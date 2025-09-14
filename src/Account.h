// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QUuid>
// QXmpp
#include <QXmppConfiguration.h>
#include <QXmppCredentials.h>
// Kaidan
#include "ClientWorker.h"
#include "Encryption.h"
#include "Globals.h"

constexpr quint16 AUTO_DETECT_PORT = 0;

class AtmController;
class AvatarCache;
class BlockingController;
class ChatController;
class ClientWorker;
class EncryptionController;
class FileSharingController;
class GroupChatController;
class MessageController;
class NotificationController;
class PresenceCache;
class QGeoCoordinate;
class RegistrationController;
class RosterController;
class VCardController;
class VersionController;
class XmppThread;

class AccountSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(quint16 autoDetectPort READ autoDetectPort CONSTANT FINAL)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY nameChanged)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
    Q_PROPERTY(bool inBandRegistrationFeaturesSupported READ inBandRegistrationFeaturesSupported NOTIFY inBandRegistrationFeaturesSupportedChanged)
    Q_PROPERTY(qint64 httpUploadLimit READ httpUploadLimit NOTIFY httpUploadLimitChanged)
    Q_PROPERTY(QString httpUploadLimitText READ httpUploadLimitText NOTIFY httpUploadLimitChanged)
    Q_PROPERTY(AccountSettings::PasswordVisibility passwordVisibility READ passwordVisibility WRITE setPasswordVisibility NOTIFY passwordVisibilityChanged)
    Q_PROPERTY(Encryption::Enum encryption READ encryption WRITE setEncryption NOTIFY encryptionChanged)
    Q_PROPERTY(AccountSettings::AutomaticMediaDownloadsRule automaticMediaDownloadsRule READ automaticMediaDownloadsRule WRITE setAutomaticMediaDownloadsRule
                   NOTIFY automaticMediaDownloadsRuleChanged)
    Q_PROPERTY(AccountSettings::ContactNotificationRule contactNotificationRule READ contactNotificationRule WRITE setContactNotificationRule NOTIFY
                   contactNotificationRuleChanged)
    Q_PROPERTY(AccountSettings::GroupChatNotificationRule groupChatNotificationRule READ groupChatNotificationRule WRITE setGroupChatNotificationRule NOTIFY
                   groupChatNotificationRuleChanged)
    Q_PROPERTY(
        bool geoLocationMapPreviewEnabled READ geoLocationMapPreviewEnabled WRITE setGeoLocationMapPreviewEnabled NOTIFY geoLocationMapPreviewEnabledChanged)
    Q_PROPERTY(AccountSettings::GeoLocationMapService geoLocationMapService READ geoLocationMapService WRITE setGeoLocationMapService NOTIFY
                   geoLocationMapServiceChanged)

public:
    /**
     * State specifying in which way a password is displayed.
     */
    enum class PasswordVisibility {
        Visible, ///< The password is included in the QR code and shown as plain text.
        OnlyVisibleViaQrCode, ///< The password is included in the QR code but not shown as plain text.
        Invisible ///< The password is neither included in the QR code nor shown as plain text.
    };
    Q_ENUM(PasswordVisibility)

    /**
     * Default rule to automatically download media for all roster items of an account.
     */
    enum class AutomaticMediaDownloadsRule {
        Never, ///< Never automatically download files.
        PresenceOnly, ///< Automatically download files only for contacts receiving presence.
        Always, ///< Always automatically download files.
    };
    Q_ENUM(AutomaticMediaDownloadsRule)

    /**
     * Default rule to inform the user about incoming messages from contacts.
     */
    enum class ContactNotificationRule {
        Never, ///< Never notify.
        PresenceOnly, ///< Notify only for contacts receiving presence.
        Always, ///< Always notify.
    };
    Q_ENUM(ContactNotificationRule)

    /**
     * Default rule to inform the user about incoming messages from group chats.
     */
    enum class GroupChatNotificationRule {
        Never, ///< Never notify.
        Mentioned, ///< Notify only if the user is mentioned.
        Always, ///< Always notify.
    };
    Q_ENUM(GroupChatNotificationRule)

    /**
     * Map service for opening geo locations.
     */
    enum class GeoLocationMapService {
        System, ///< Let the system decide how to open geo locations.
        InApp, ///< Open geo locations in Kaidan.
        Web, ///< Open geo locations in a web browser.
    };
    Q_ENUM(GeoLocationMapService)

    struct Data {
        bool initialized = false;
        bool initialMessagesRetrieved = false;
        QString jid;
        QString jidResource;
        QString password;
        QXmppCredentials credentials;
        QUuid userAgentDeviceId;
        bool tlsErrorsIgnored = false;
        QXmppConfiguration::StreamSecurityMode tlsRequirement = QXmppConfiguration::TLSRequired;
        QString host;
        quint16 port = AUTO_DETECT_PORT;
        QString name;
        bool enabled = true;
        QString latestMessageStanzaId;
        QDateTime latestMessageStanzaTimestamp;
        bool inBandRegistrationFeaturesSupported = false;
        qint64 httpUploadLimit = 0;
        PasswordVisibility passwordVisibility = PasswordVisibility::Visible;
        Encryption::Enum encryption = Encryption::Omemo2;
        AutomaticMediaDownloadsRule automaticMediaDownloadsRule = AutomaticMediaDownloadsRule::PresenceOnly;
        ContactNotificationRule contactNotificationRule = ContactNotificationRule::Always;
        GroupChatNotificationRule groupChatNotificationRule = GroupChatNotificationRule::Always;
        bool geoLocationMapPreviewEnabled = true;
        GeoLocationMapService geoLocationMapService = GeoLocationMapService::System;

        bool operator==(const Data &other) const = default;
        bool operator!=(const Data &other) const = default;
    };

    explicit AccountSettings(Data data, QObject *parent = nullptr);

    bool initialized() const;
    bool initialMessagesRetrieved() const;

    QString jid() const;
    void setJid(const QString &jid);
    Q_SIGNAL void jidChanged();

    QString jidResource() const;

    QString password() const;
    void setPassword(const QString &password);
    Q_SIGNAL void passwordChanged();

    QXmppCredentials credentials() const;
    void setCredentials(const QXmppCredentials &credentials);

    QXmppSasl2UserAgent userAgent() const;

    bool tlsErrorsIgnored() const;

    QXmppConfiguration::StreamSecurityMode tlsRequirement() const;

    QString host() const;
    void setHost(const QString &host);
    Q_SIGNAL void hostChanged();

    quint16 port() const;
    void setPort(quint16 port);
    Q_SIGNAL void portChanged();

    quint16 autoDetectPort() const;

    QString name() const;
    void setName(const QString &name);
    Q_SIGNAL void nameChanged();

    QString displayName() const;

    bool enabled() const;
    void setEnabled(bool enabled);
    Q_SIGNAL void enabledChanged();

    bool inBandRegistrationFeaturesSupported() const;
    void setInBandRegistrationFeaturesSupported(bool inBandRegistrationFeaturesSupported);
    Q_SIGNAL void inBandRegistrationFeaturesSupportedChanged();

    qint64 httpUploadLimit() const;
    void setHttpUploadLimit(qint64 httpUploadLimit);
    Q_SIGNAL void httpUploadLimitChanged();

    QString httpUploadLimitText() const;

    PasswordVisibility passwordVisibility() const;
    void setPasswordVisibility(PasswordVisibility passwordVisibility);
    Q_SIGNAL void passwordVisibilityChanged();

    Encryption::Enum encryption() const;
    void setEncryption(Encryption::Enum encryption);
    Q_SIGNAL void encryptionChanged();

    AutomaticMediaDownloadsRule automaticMediaDownloadsRule() const;
    void setAutomaticMediaDownloadsRule(AutomaticMediaDownloadsRule automaticMediaDownloadsRule);
    Q_SIGNAL void automaticMediaDownloadsRuleChanged();

    ContactNotificationRule contactNotificationRule() const;
    void setContactNotificationRule(ContactNotificationRule contactNotificationRule);
    Q_SIGNAL void contactNotificationRuleChanged();

    GroupChatNotificationRule groupChatNotificationRule() const;
    void setGroupChatNotificationRule(GroupChatNotificationRule groupChatNotificationRule);
    Q_SIGNAL void groupChatNotificationRuleChanged();

    bool geoLocationMapPreviewEnabled() const;
    void setGeoLocationMapPreviewEnabled(bool geoLocationMapPreviewEnabled);
    Q_SIGNAL void geoLocationMapPreviewEnabledChanged();

    GeoLocationMapService geoLocationMapService() const;
    void setGeoLocationMapService(GeoLocationMapService geoLocationMapService);
    Q_SIGNAL void geoLocationMapServiceChanged();

    Q_INVOKABLE void resetCustomConnectionSettings();

    void storeTemporaryData();

private:
    void generateJidResource();
    void generateUserAgentDeviceId();

    Data m_data;
};

class Connection : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Enums::ConnectionState state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
    Q_PROPERTY(ClientWorker::ConnectionError error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorChanged)

public:
    explicit Connection(ClientWorker *clientWorker, QObject *parent = nullptr);

    Q_INVOKABLE void logIn();
    Q_INVOKABLE void logOut(bool isApplicationBeingClosed = false);

    Enums::ConnectionState state() const;
    Q_SIGNAL void stateChanged();
    Q_SIGNAL void connected();

    QString stateText() const;
    QString errorText() const;

    /**
     * Returns the last connection error.
     */
    ClientWorker::ConnectionError error() const;
    Q_SIGNAL void errorChanged();

private:
    void setState(Enums::ConnectionState connectionState);
    void setError(ClientWorker::ConnectionError error);

    ClientWorker *const m_clientWorker;

    Enums::ConnectionState m_state = Enums::ConnectionState::StateDisconnected;
    ClientWorker::ConnectionError m_error = ClientWorker::NoError;
};

class Account : public QObject
{
    Q_OBJECT

    Q_PROPERTY(AccountSettings *settings MEMBER m_settings CONSTANT)
    Q_PROPERTY(Connection *connection MEMBER m_connection CONSTANT)

    Q_PROPERTY(AtmController *atmController READ atmController CONSTANT)
    Q_PROPERTY(BlockingController *blockingController READ blockingController CONSTANT)
    Q_PROPERTY(EncryptionController *encryptionController READ encryptionController CONSTANT)
    Q_PROPERTY(FileSharingController *fileSharingController READ fileSharingController CONSTANT)
    Q_PROPERTY(GroupChatController *groupChatController READ groupChatController CONSTANT)
    Q_PROPERTY(MessageController *messageController READ messageController CONSTANT)
    Q_PROPERTY(RegistrationController *registrationController READ registrationController CONSTANT)
    Q_PROPERTY(RosterController *rosterController READ rosterController CONSTANT)
    Q_PROPERTY(VCardController *vCardController READ vCardController CONSTANT)
    Q_PROPERTY(VersionController *versionController READ versionController CONSTANT)

    Q_PROPERTY(AvatarCache *avatarCache READ avatarCache NOTIFY avatarCacheChanged)
    Q_PROPERTY(PresenceCache *presenceCache READ presenceCache CONSTANT)

public:
    /**
     * State specifying how an XMPP login URI is used.
     */
    enum class LoginWithUriResult {
        Connecting, ///< The JID and password are included in the URI and the client is connecting.
        PasswordNeeded, ///< The JID is included in the URI but not the password.
        InvalidLoginUri ///< The URI cannot be used to log in.
    };
    Q_ENUM(LoginWithUriResult)

    explicit Account(QObject *parent = nullptr);
    explicit Account(AccountSettings::Data accountSettingsData, QObject *parent = nullptr);
    ~Account() override;

    AccountSettings *settings() const;
    Connection *connection() const;

    ClientWorker *clientWorker() const;

    AtmController *atmController() const;
    BlockingController *blockingController() const;
    EncryptionController *encryptionController() const;
    FileSharingController *fileSharingController() const;
    GroupChatController *groupChatController() const;
    MessageController *messageController() const;
    NotificationController *notificationController() const;
    RegistrationController *registrationController() const;
    RosterController *rosterController() const;
    VCardController *vCardController() const;
    VersionController *versionController() const;

    AvatarCache *avatarCache() const;
    Q_SIGNAL void avatarCacheChanged();
    PresenceCache *presenceCache() const;

    Q_INVOKABLE void enable();
    Q_INVOKABLE void disable();
    void restoreState();

    /**
     * Connects to the server with the parsed credentials (bare JID and password) of an XMPP URI.
     *
     * The URI (e.g., "xmpp:user@example.org?login;password=abc") is used as follows.
     *
     * Login attempt (LoginWithUriResult::Connecting is returned):
     *	xmpp:user@example.org?login;password=abc
     *
     * Pre-fill of JID for opening login page (LoginWithUriResult::PasswordNeeded is returned):
     *	xmpp:user@example.org?login;password=
     *	xmpp:user@example.org?login;password
     *	xmpp:user@example.org?login;
     *	xmpp:user@example.org?login
     *	xmpp:user@example.org?
     *	xmpp:user@example.org
     *
     * In all other cases, LoginWithUriResult::InvalidLoginUri is returned.
     *
     * @param uriString string which can be an XMPP login URI
     *
     * @return how the XMPP login URI is used
     */
    Q_INVOKABLE LoginWithUriResult logInWithUri(const QString &uriString);

    /**
     * Opens a geo location as preferred by the user.
     *
     * @return whether the location should be opened within Kaidan
     */
    Q_INVOKABLE bool openGeoLocation(const QGeoCoordinate &geoCoordinate);

private:
    AccountSettings *const m_settings;

    struct XmppClient {
        XmppThread *thread;
        ClientWorker *worker;
    } m_client;

    Connection *const m_connection;

    AvatarCache *const m_avatarCache;
    PresenceCache *const m_presenceCache;

    AtmController *const m_atmController;
    BlockingController *const m_blockingController;
    EncryptionController *const m_encryptionController;
    FileSharingController *const m_fileSharingController;
    RosterController *const m_rosterController;
    MessageController *const m_messageController;
    GroupChatController *const m_groupChatController;
    NotificationController *const m_notificationController;
    VCardController *const m_vCardController;
    RegistrationController *const m_registrationController;
    VersionController *const m_versionController;
};

Q_DECLARE_METATYPE(Account)
Q_DECLARE_METATYPE(AccountSettings::PasswordVisibility)
Q_DECLARE_METATYPE(AccountSettings::ContactNotificationRule)
Q_DECLARE_METATYPE(AccountSettings::GroupChatNotificationRule)
Q_DECLARE_METATYPE(AccountSettings::AutomaticMediaDownloadsRule)
Q_DECLARE_METATYPE(Connection)
Q_DECLARE_METATYPE(QXmppConfiguration::StreamSecurityMode)
