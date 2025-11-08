// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Account.h"

// Qt
#include <QDesktopServices>
#include <QFile>
#include <QGeoCoordinate>
#include <QThread>
#include <QWaitCondition>
// QXmpp
#include "QXmppSasl2UserAgent.h"
#include <QXmppUri.h>
#include <QXmppUtils.h>
// Kaidan
#include "AccountDb.h"
#include "AtmController.h"
#include "Blocking.h"
#include "ChatController.h"
#include "CredentialsValidator.h"
#include "DiscoveryController.h"
#include "EncryptionController.h"
#include "FileSharingController.h"
#include "FutureUtils.h"
#include "GroupChatController.h"
#include "LogHandler.h"
#include "MessageController.h"
#include "NotificationController.h"
#include "PresenceCache.h"
#include "QmlUtils.h"
#include "RegistrationController.h"
#include "RosterController.h"
#include "RosterModel.h"
#include "SystemUtils.h"
#include "VCardController.h"
#include "VersionController.h"

constexpr QStringView GEO_LOCATION_WEB_URL = u"https://osmand.net/map/?pin=%1,%2#16/%1/%2";

/**
 * Allows to run a function and waits for the thread to be started before returning to the calling
 * thread.
 *
 * This is needed because QXmppMigrationManager expects the classes of the import/export functions
 * to be registered while they are running in the right/final thread.
 */
class XmppThread : public QThread
{
public:
    explicit XmppThread(std::future<void> &&future)
        : m_future(std::move(future))
    {
    }

    ~XmppThread() override
    {
        requestInterruption();
        quit();
        wait();
    }

    void waitForStarted(QThread::Priority priority = InheritPriority)
    {
        if (isRunning()) {
            return;
        }

        QMutexLocker locker(&m_mutex);
        QThread::start(priority);
        m_condition.wait(&m_mutex);
    }

    template<typename Function, typename... Args>
    static XmppThread *create(Function &&f, Args &&...args)
    {
        using DecayedFunction = typename std::decay<Function>::type;
        auto threadFunction = [f = static_cast<DecayedFunction>(std::forward<Function>(f))](auto &&...largs) mutable -> void {
            (void)std::invoke(std::move(f), std::forward<decltype(largs)>(largs)...);
        };
        return new XmppThread(std::move(std::async(std::launch::deferred, std::move(threadFunction), std::forward<Args>(args)...)));
    }

protected:
    void run() override
    {
        m_future.get();
        m_condition.wakeOne();
        QThread::exec();
    }

private:
    QWaitCondition m_condition;
    QMutex m_mutex;
    std::future<void> m_future;
};

AccountSettings::AccountSettings(Data data, QObject *parent)
    : QObject(parent)
    , m_data(data)
{
    generateJidResource();
    generateUserAgentDeviceId();
}

bool AccountSettings::initialized() const
{
    return m_data.initialized;
}

bool AccountSettings::initialMessagesRetrieved() const
{
    return m_data.initialMessagesRetrieved;
}

QString AccountSettings::jid() const
{
    return m_data.jid;
}

void AccountSettings::setJid(const QString &jid)
{
    if (m_data.jid != jid) {
        m_data.jid = jid;
        Q_EMIT jidChanged();
    }
}

QString AccountSettings::jidResource() const
{
    return m_data.jidResource;
}

QString AccountSettings::password() const
{
    return m_data.password;
}

void AccountSettings::setPassword(const QString &password)
{
    if (m_data.password != password) {
        m_data.password = password;
        Q_EMIT passwordChanged();
    }
}

QXmppCredentials AccountSettings::credentials() const
{
    return m_data.credentials;
}

void AccountSettings::setCredentials(const QXmppCredentials &credentials)
{
    m_data.credentials = credentials;
    AccountDb::instance()->updateCredentials(m_data.jid, credentials);
}

QXmppSasl2UserAgent AccountSettings::userAgent() const
{
    return QXmppSasl2UserAgent(m_data.userAgentDeviceId, QStringLiteral(APPLICATION_DISPLAY_NAME), SystemUtils::productName());
}

bool AccountSettings::tlsErrorsIgnored() const
{
    return m_data.tlsErrorsIgnored;
}

QXmppConfiguration::StreamSecurityMode AccountSettings::tlsRequirement() const
{
    return m_data.tlsRequirement;
}

QString AccountSettings::host() const
{
    return m_data.host;
}

void AccountSettings::setHost(const QString &host)
{
    if (m_data.host != host) {
        m_data.host = host;
        Q_EMIT hostChanged();
    }
}

quint16 AccountSettings::port() const
{
    return m_data.port;
}

void AccountSettings::setPort(quint16 port)
{
    if (m_data.port != port) {
        m_data.port = port;
        Q_EMIT portChanged();
    }
}

quint16 AccountSettings::autoDetectPort() const
{
    return AUTO_DETECT_PORT;
}

QString AccountSettings::name() const
{
    return m_data.name;
}

void AccountSettings::setName(const QString &name)
{
    if (m_data.name != name) {
        m_data.name = name;
        Q_EMIT nameChanged();
    }
}

QString AccountSettings::displayName() const
{
    if (!m_data.name.isEmpty()) {
        return m_data.name;
    }

    return QXmppUtils::jidToUser(m_data.jid);
}

bool AccountSettings::enabled() const
{
    return m_data.enabled;
}

void AccountSettings::setEnabled(bool enabled)
{
    if (m_data.enabled != enabled) {
        m_data.enabled = enabled;
        AccountDb::instance()->updateEnabled(m_data.jid, enabled);
        Q_EMIT enabledChanged();
    }
}

bool AccountSettings::inBandRegistrationFeaturesSupported() const
{
    return m_data.inBandRegistrationFeaturesSupported;
}

void AccountSettings::setInBandRegistrationFeaturesSupported(bool inBandRegistrationFeaturesSupported)
{
    if (m_data.inBandRegistrationFeaturesSupported != inBandRegistrationFeaturesSupported) {
        m_data.inBandRegistrationFeaturesSupported = inBandRegistrationFeaturesSupported;
        Q_EMIT inBandRegistrationFeaturesSupportedChanged();
    }
}

qint64 AccountSettings::httpUploadLimit() const
{
    return m_data.httpUploadLimit;
}

void AccountSettings::setHttpUploadLimit(qint64 httpUploadLimit)
{
    if (m_data.httpUploadLimit != httpUploadLimit) {
        m_data.httpUploadLimit = httpUploadLimit;
        AccountDb::instance()->updateHttpUploadLimit(m_data.jid, httpUploadLimit);
        Q_EMIT httpUploadLimitChanged();
    }
}

QString AccountSettings::httpUploadLimitText() const
{
    return QmlUtils::formattedDataSize(m_data.httpUploadLimit);
}

void AccountSettings::setChatSupportAddresses(const QList<QString> &chatSupportAddresses)
{
    if (m_data.chatSupportAddresses != chatSupportAddresses) {
        m_data.chatSupportAddresses = chatSupportAddresses;
        Q_EMIT chatSupportAddressesChanged();
    }
}

QList<QString> AccountSettings::chatSupportAddresses() const
{
    return m_data.chatSupportAddresses;
}

void AccountSettings::setGroupChatSupportAddresses(const QList<QString> &groupChatSupportAddresses)
{
    if (m_data.groupChatSupportAddresses != groupChatSupportAddresses) {
        m_data.groupChatSupportAddresses = groupChatSupportAddresses;
        Q_EMIT groupChatSupportAddressesChanged();
    }
}

QList<QString> AccountSettings::groupChatSupportAddresses() const
{
    return m_data.groupChatSupportAddresses;
}

AccountSettings::PasswordVisibility AccountSettings::passwordVisibility() const
{
    return m_data.passwordVisibility;
}

void AccountSettings::setPasswordVisibility(PasswordVisibility passwordVisibility)
{
    if (m_data.passwordVisibility != passwordVisibility) {
        m_data.passwordVisibility = passwordVisibility;
        AccountDb::instance()->updatePasswordVisibility(m_data.jid, passwordVisibility);
        Q_EMIT passwordVisibilityChanged();
    }
}

Encryption::Enum AccountSettings::encryption() const
{
    return m_data.encryption;
}

void AccountSettings::setEncryption(Encryption::Enum encryption)
{
    if (m_data.encryption != encryption) {
        m_data.encryption = encryption;
        AccountDb::instance()->updateEncryption(m_data.jid, encryption);
        Q_EMIT encryptionChanged();
    }
}

AccountSettings::AutomaticMediaDownloadsRule AccountSettings::automaticMediaDownloadsRule() const
{
    return m_data.automaticMediaDownloadsRule;
}

void AccountSettings::setAutomaticMediaDownloadsRule(AutomaticMediaDownloadsRule automaticMediaDownloadsRule)
{
    if (m_data.automaticMediaDownloadsRule != automaticMediaDownloadsRule) {
        m_data.automaticMediaDownloadsRule = automaticMediaDownloadsRule;
        AccountDb::instance()->updateAutomaticMediaDownloadsRule(m_data.jid, automaticMediaDownloadsRule);
        Q_EMIT automaticMediaDownloadsRuleChanged();
    }
}

AccountSettings::ContactNotificationRule AccountSettings::contactNotificationRule() const
{
    return m_data.contactNotificationRule;
}

void AccountSettings::setContactNotificationRule(ContactNotificationRule contactNotificationRule)
{
    if (m_data.contactNotificationRule != contactNotificationRule) {
        m_data.contactNotificationRule = contactNotificationRule;
        AccountDb::instance()->updateContactNotificationRule(m_data.jid, contactNotificationRule);
        Q_EMIT contactNotificationRuleChanged();
    }
}

AccountSettings::GroupChatNotificationRule AccountSettings::groupChatNotificationRule() const
{
    return m_data.groupChatNotificationRule;
}

void AccountSettings::setGroupChatNotificationRule(GroupChatNotificationRule groupChatNotificationRule)
{
    if (m_data.groupChatNotificationRule != groupChatNotificationRule) {
        m_data.groupChatNotificationRule = groupChatNotificationRule;
        AccountDb::instance()->updateGroupChatNotificationRule(m_data.jid, groupChatNotificationRule);
        Q_EMIT groupChatNotificationRuleChanged();
    }
}

bool AccountSettings::geoLocationMapPreviewEnabled() const
{
    return m_data.geoLocationMapPreviewEnabled;
}

void AccountSettings::setGeoLocationMapPreviewEnabled(bool geoLocationMapPreviewEnabled)
{
    if (m_data.geoLocationMapPreviewEnabled != geoLocationMapPreviewEnabled) {
        m_data.geoLocationMapPreviewEnabled = geoLocationMapPreviewEnabled;
        AccountDb::instance()->updateGeoLocationMapPreviewEnabled(m_data.jid, geoLocationMapPreviewEnabled);
        Q_EMIT geoLocationMapPreviewEnabledChanged();
    }
}

AccountSettings::GeoLocationMapService AccountSettings::geoLocationMapService() const
{
    return m_data.geoLocationMapService;
}

void AccountSettings::setGeoLocationMapService(GeoLocationMapService geoLocationMapService)
{
    if (m_data.geoLocationMapService != geoLocationMapService) {
        m_data.geoLocationMapService = geoLocationMapService;
        AccountDb::instance()->updateGeoLocationMapService(m_data.jid, geoLocationMapService);
        Q_EMIT geoLocationMapServiceChanged();
    }
}

void AccountSettings::resetCustomConnectionSettings()
{
    setHost({});
    setPort(AUTO_DETECT_PORT);
}

void AccountSettings::storeTemporaryData()
{
    if (m_data.initialized) {
        AccountDb::instance()->updateAccount(m_data.jid, [data = m_data](AccountSettings::Data &account) {
            account.password = data.password;
            account.userAgentDeviceId = data.userAgentDeviceId;
            account.host = data.host;
            account.port = data.port;
            account.name = data.name;
        });
    } else {
        m_data.enabled = true;
        m_data.initialized = true;
        AccountDb::instance()->addAccount(m_data);
    }
}

QString AccountSettings::loginUriString() const
{
    QXmpp::Uri::Login loginQuery;

    if (passwordVisibility() != AccountSettings::PasswordVisibility::Invisible) {
        loginQuery.password = password();
    }

    QXmppUri uri;

    uri.setJid(jid());
    uri.setQuery(std::move(loginQuery));

    return uri.toString();
}

void AccountSettings::generateJidResource()
{
    m_data.jidResource = QXmppUtils::generateStanzaHash(4);
}

void AccountSettings::generateUserAgentDeviceId()
{
    if (auto &deviceId = m_data.userAgentDeviceId; deviceId.isNull()) {
        deviceId = QUuid::createUuid();
    }
}

Connection::Connection(ClientWorker *clientWorker, QObject *parent)
    : QObject(parent)
    , m_clientWorker(clientWorker)
{
    connect(m_clientWorker, &ClientWorker::connectionStateChanged, this, &Connection::setState);
    connect(m_clientWorker, &ClientWorker::connectionErrorChanged, this, &Connection::setError);
}

void Connection::logIn()
{
    runOnThread(m_clientWorker, [this]() {
        m_clientWorker->logIn();
    });
}

void Connection::logOut(bool isApplicationBeingClosed)
{
    runOnThread(m_clientWorker, [this, isApplicationBeingClosed]() {
        m_clientWorker->logOut(isApplicationBeingClosed);
    });
}

Enums::ConnectionState Connection::state() const
{
    return m_state;
}

QString Connection::stateText() const
{
    switch (m_state) {
    case Enums::ConnectionState::StateConnected:
        return tr("Connected");
    case Enums::ConnectionState::StateConnecting:
        return tr("Connecting…");
    case Enums::ConnectionState::StateDisconnected:
        return tr("Disconnected");
    }
    return {};
}

QString Connection::errorText() const
{
    switch (m_error) {
    case ClientWorker::NoError:
        return {};
    case ClientWorker::AuthenticationFailed:
        return tr("Invalid username or password.");
    case ClientWorker::NotConnected:
        return tr("Cannot connect to the server. Please check your internet connection.");
    case ClientWorker::TlsFailed:
        return tr("Error while trying to connect securely.");
    case ClientWorker::TlsNotAvailable:
        return tr("The server doesn't support secure connections.");
    case ClientWorker::DnsError:
        return tr("Could not connect to the server. Please check your internet connection or your server name.");
    case ClientWorker::ConnectionRefused:
        return tr("The server is offline or blocked by a firewall.");
    case ClientWorker::NoSupportedAuth:
        return tr("Authentication protocol not supported by the server.");
    case ClientWorker::KeepAliveError:
        return tr("The connection could not be refreshed.");
    case ClientWorker::NoNetworkPermission:
        return tr("The internet access is not permitted. Please check your system's internet access configuration.");
    case ClientWorker::RegistrationUnsupported:
        return tr("This server does not support registration.");
    case ClientWorker::EmailConfirmationRequired:
        return tr("Could not log in. Confirm the email message you received first.");
    case ClientWorker::UnknownError:
        return tr("Could not connect to the server.");
    }
    Q_UNREACHABLE();
}

ClientWorker::ConnectionError Connection::error() const
{
    return m_error;
}

void Connection::setState(Enums::ConnectionState state)
{
    if (m_state != state) {
        m_state = state;
        Q_EMIT stateChanged();

        if (state == Enums::ConnectionState::StateConnected) {
            Q_EMIT connected();
        }
    }
}

void Connection::setError(ClientWorker::ConnectionError error)
{
    // For displaying errors to the user, every new error (even if it is the same as before) must be
    // emitted.
    // Thus, the error is not checked for a change but simply emitted.
    m_error = error;
    Q_EMIT errorChanged();
}

Account::Account(QObject *parent)
    : Account({}, parent)
{
}

Account::Account(AccountSettings::Data accountSettingsData, QObject *parent)
    : QObject(parent)
    , m_settings(new AccountSettings(accountSettingsData, this))
    , m_client([this] {
        ClientWorker *worker = nullptr;
        auto thread = XmppThread::create([&, this] {
            worker = new ClientWorker(m_settings);
        });

        thread->setObjectName(u"XMPP");
        thread->waitForStarted();

        new LogHandler(m_settings, worker->xmppClient(), this);

        return XmppClient{thread, worker};
    }())
    , m_connection(new Connection(m_client.worker, this))
    , m_presenceCache(new PresenceCache(this))
    , m_atmController(new AtmController(m_client.worker->atmManager(), this))
    , m_blockingController(new BlockingController(m_settings, m_connection, m_client.worker, this))
    , m_encryptionController(new EncryptionController(m_settings, m_presenceCache, m_client.worker->omemoManager(), this))
    , m_rosterController(new RosterController(m_settings, m_connection, m_encryptionController, m_client.worker->rosterManager(), this))
    , m_messageController(new MessageController(m_settings,
                                                m_connection,
                                                m_encryptionController,
                                                m_rosterController,
                                                m_client.worker->xmppClient(),
                                                m_client.worker->mamManager(),
                                                m_client.worker->messageReceiptManager(),
                                                this))
    , m_fileSharingController(new FileSharingController(m_settings, m_connection, m_messageController, m_client.worker, this))
    , m_groupChatController(new GroupChatController(m_settings, m_messageController, m_client.worker->mixManager(), this))
    , m_notificationController(new NotificationController(m_settings, m_messageController, this))
    , m_vCardController(new VCardController(m_settings, m_connection, m_presenceCache, m_client.worker->xmppClient(), m_client.worker->vCardManager(), this))
    , m_registrationController(new RegistrationController(m_settings, m_connection, m_encryptionController, m_vCardController, m_client.worker, this))
    , m_versionController(new VersionController(m_presenceCache, m_client.worker->versionManager()))
{
    new DiscoveryController(m_settings, m_connection, m_client.worker->discoveryManager(), this);

    runOnThread(m_client.worker, [this]() {
        m_client.worker->initialize(m_atmController, m_encryptionController, m_messageController, m_registrationController, m_presenceCache);
    });
}

Account::~Account()
{
    m_client.worker->deleteLater();

    m_client.thread->quit();
    m_client.thread->wait();
}

AccountSettings *Account::settings() const
{
    return m_settings;
}

Connection *Account::connection() const
{
    return m_connection;
}

ClientWorker *Account::clientWorker() const
{
    return m_client.worker;
}

AtmController *Account::atmController() const
{
    return m_atmController;
}

BlockingController *Account::blockingController() const
{
    return m_blockingController;
}

EncryptionController *Account::encryptionController() const
{
    return m_encryptionController;
}

FileSharingController *Account::fileSharingController() const
{
    return m_fileSharingController;
}

GroupChatController *Account::groupChatController() const
{
    return m_groupChatController;
}

MessageController *Account::messageController() const
{
    return m_messageController;
}

NotificationController *Account::notificationController() const
{
    return m_notificationController;
}

RegistrationController *Account::registrationController() const
{
    return m_registrationController;
}

RosterController *Account::rosterController() const
{
    return m_rosterController;
}

VCardController *Account::vCardController() const
{
    return m_vCardController;
}

VersionController *Account::versionController() const
{
    return m_versionController;
}

PresenceCache *Account::presenceCache() const
{
    return m_presenceCache;
}

void Account::enable()
{
    m_settings->setEnabled(true);
    m_connection->logIn();
}

void Account::disable()
{
    m_settings->setEnabled(false);
    m_connection->logOut();
}

void Account::restoreState()
{
    if (m_settings->enabled()) {
        m_connection->logIn();
    }
}

Account::LoginWithUriResult Account::logInWithUri(const QString &uriString)
{
    if (const auto uriParsingResult = QXmppUri::fromString(uriString); std::holds_alternative<QXmppUri>(uriParsingResult)) {
        const auto uri = std::get<QXmppUri>(uriParsingResult);
        const auto jid = uri.jid();
        const auto query = uri.query();

        if (query.type() != typeid(QXmpp::Uri::Login) || !CredentialsValidator::isUserJidValid(jid)) {
            return LoginWithUriResult::InvalidLoginUri;
        }

        const auto password = std::any_cast<QXmpp::Uri::Login>(query).password;

        if (!CredentialsValidator::isPasswordValid(password)) {
            return LoginWithUriResult::PasswordNeeded;
        }

        m_settings->setJid(jid);
        m_settings->setPassword(password);

        // Connect with the extracted credentials.
        m_connection->logIn();

        return LoginWithUriResult::Connecting;
    }

    return LoginWithUriResult::InvalidLoginUri;
}

bool Account::openGeoLocation(const QGeoCoordinate &geoCoordinate)
{
    switch (m_settings->geoLocationMapService()) {
    case AccountSettings::GeoLocationMapService::System:
        QDesktopServices::openUrl(QUrl(QmlUtils::geoUri(geoCoordinate)));
        break;
    case AccountSettings::GeoLocationMapService::InApp:
        return true;
    case AccountSettings::GeoLocationMapService::Web:
        QDesktopServices::openUrl(QUrl(GEO_LOCATION_WEB_URL.arg(QString::number(geoCoordinate.latitude()), QString::number(geoCoordinate.longitude()))));
        break;
    }

    return false;
}

#include "moc_Account.cpp"
