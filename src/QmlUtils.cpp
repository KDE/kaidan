// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "QmlUtils.h"

// Qt
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QGeoCoordinate>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QStandardPaths>
// QXmpp
#include <QXmppColorGeneration.h>
#include <QXmppUri.h>
// Kaidan
#include "AccountManager.h"
#include "Globals.h"
#include "KaidanCoreLog.h"
#include "SystemUtils.h"

const auto NEW_LINE = QStringLiteral("\n");
const auto QUOTE_PREFIX = QStringLiteral("> ");

constexpr QStringView GEO_URI_SCHEME = u"geo";
constexpr QStringView GEO_URI_COORDINATE_SEPARATOR = u",";
constexpr QStringView GEO_LOCATION_WEB_URL = u"https://osmand.net/map/?pin=%1,%2#16/%1/%2";

static QmlUtils *s_instance;

QmlUtils *QmlUtils::instance()
{
    if (!s_instance)
        return new QmlUtils(QGuiApplication::instance());

    return s_instance;
}

QmlUtils::QmlUtils(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

QmlUtils::~QmlUtils()
{
    s_instance = nullptr;
}

QChar QmlUtils::messageBubblePaddingCharacter()
{
    return MESSAGE_BUBBLE_PADDING_CHARACTER;
}

QChar QmlUtils::groupChatUserMentionPrefix()
{
    return GROUP_CHAT_USER_MENTION_PREFIX;
}

QChar QmlUtils::groupChatUserMentionSeparator()
{
    return GROUP_CHAT_USER_MENTION_SEPARATOR;
}

QString QmlUtils::systemCountryCode()
{
    return SystemUtils::systemLocaleCodes().countryCode;
}

QString QmlUtils::connectionErrorMessage(ClientWorker::ConnectionError error)
{
    switch (error) {
    case ClientWorker::NoError:
        break;
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
        return tr("Authentification protocol not supported by the server.");
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

QString QmlUtils::getResourcePath(const QString &name)
{
    // We generally prefer to first search for files in application resources
    if (QFile::exists(QStringLiteral(":/") + name))
        return QStringLiteral("qrc:/") + name;

    // list of file paths where to search for the resource file
    QStringList pathList;
    // add relative path from binary (only works if installed)
    pathList << QCoreApplication::applicationDirPath() + QStringLiteral("/../share/") + QStringLiteral(APPLICATION_NAME);
    // get the standard app data locations for current platform
    pathList << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
#ifndef NDEBUG
#ifdef DEBUG_SOURCE_PATH
    // add source directory (only for debug builds)
    pathList << QStringLiteral(DEBUG_SOURCE_PATH) + QStringLiteral("/data");
#endif
#endif

    // search for file in directories
    for (int i = 0; i < pathList.size(); i++) {
        // open directory
        QDir directory(pathList.at(i));
        // look up the file
        if (directory.exists(name)) {
            // found the file, return the path
            return QUrl::fromLocalFile(directory.absoluteFilePath(name)).toString();
        }
    }

    // no file found
    qCWarning(KAIDAN_CORE_LOG) << "Could not find media file:" << name;
    return QString();
}

QString QmlUtils::versionString()
{
    return QStringLiteral(VERSION_STRING);
}

QString QmlUtils::applicationDisplayName()
{
    return QStringLiteral(APPLICATION_DISPLAY_NAME);
}

QUrl QmlUtils::applicationWebsiteUrl()
{
    return QUrl(QStringLiteral(APPLICATION_WEBSITE_URL));
}

QUrl QmlUtils::applicationSourceCodeUrl()
{
    return QUrl(QStringLiteral(APPLICATION_SOURCE_CODE_URL));
}

QUrl QmlUtils::issueTrackingUrl()
{
    return QUrl(QStringLiteral(ISSUE_TRACKING_URL));
}

QUrl QmlUtils::donationUrl()
{
    return QUrl(QStringLiteral(DONATION_URL));
}

QUrl QmlUtils::mastodonUrl()
{
    return QUrl(QStringLiteral(MASTODON_URL));
}

QUrl QmlUtils::providerDetailsUrl(const QString &providerJid)
{
    return QUrl(PROVIDER_DETAILS_URL.arg(providerJid));
}

QUrl QmlUtils::invitationUrl(const QString &uri)
{
    // "uri.mid(5)" removes "xmpp:".
    return QUrl(QStringLiteral(INVITATION_URL) + uri.mid(5));
}

QUrl QmlUtils::groupChatUri(const QString &groupChatJid)
{
    QXmppUri uri;
    uri.setJid(groupChatJid);
    uri.setQuery(QXmpp::Uri::Join());
    return QUrl(uri.toString());
}

bool QmlUtils::validateEncryptionKeyId(const QString &keyId)
{
    static QRegularExpression re(QStringLiteral("^[0-9A-F]{64}$"), QRegularExpression::CaseInsensitiveOption);
    return re.match(keyId).hasMatch();
}

QString QmlUtils::displayableEncryptionKeyId(QString keyId)
{
    keyId.remove(ENCRYPTION_KEY_ID_CHARACTER_GROUP_SEPARATOR.toString());

    QStringList keyIdParts;

    for (int i = 0; i * ENCRYPTION_KEY_ID_CHARACTER_GROUP_SIZE < keyId.size(); i++) {
        keyIdParts.append(keyId.mid(i * ENCRYPTION_KEY_ID_CHARACTER_GROUP_SIZE, ENCRYPTION_KEY_ID_CHARACTER_GROUP_SIZE));
    }

    return keyIdParts.join(ENCRYPTION_KEY_ID_CHARACTER_GROUP_SEPARATOR);
}

QString QmlUtils::removeNewLinesFromString(const QString &input)
{
    return input.simplified();
}

QString QmlUtils::quote(QString text)
{
    return QUOTE_PREFIX + text.replace(NEW_LINE, NEW_LINE + QUOTE_PREFIX) + NEW_LINE;
}

void QmlUtils::copyToClipboard(const QUrl &url)
{
    copyToClipboard(url.toString());
}

void QmlUtils::copyToClipboard(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

void QmlUtils::copyToClipboard(const QImage &image)
{
    QGuiApplication::clipboard()->setImage(image);
}

QString QmlUtils::formattedDataSize(qint64 fileSize)
{
    if (fileSize < 0) {
        return tr("Unknown size");
    }

    return QLocale::system().formattedDataSize(fileSize, 0, QLocale::DataSizeSIFormat);
}

QColor QmlUtils::userColor(const QString &id, const QString &name)
{
    return QXmppColorGeneration::generateColor(id.isEmpty() ? name : id);
}

QString QmlUtils::chatStateDescription(const QString &displayName, const QXmppMessage::State state)
{
    switch (state) {
    case QXmppMessage::State::Active:
        return tr("%1 is online").arg(displayName);
    case QXmppMessage::State::Composing:
        return tr("%1 is typing…").arg(displayName);
    case QXmppMessage::State::Paused:
        return tr("%1 paused typing").arg(displayName);
    // Not helpful for the user, so don't display
    case QXmppMessage::State::Gone:
    case QXmppMessage::State::Inactive:
    case QXmppMessage::State::None:
        break;
    };

    return {};
}

QString QmlUtils::osmUserAgent()
{
    return u"" APPLICATION_NAME % QChar(u'/') % u"" VERSION_STRING;
}

QString QmlUtils::geoUri(const QGeoCoordinate &geoCoordinate)
{
    QUrl uri;
    uri.setScheme(GEO_URI_SCHEME.toString());
    uri.setPath(QString::number(geoCoordinate.latitude()) + GEO_URI_COORDINATE_SEPARATOR.toString() + QString::number(geoCoordinate.longitude()));

    return uri.toString();
}

QGeoCoordinate QmlUtils::geoCoordinate(const QString &geoUri)
{
    QUrl uri(geoUri);

    if (uri.scheme() == GEO_URI_SCHEME) {
        const auto coordinateParts = uri.path().split(GEO_URI_COORDINATE_SEPARATOR.toString());

        if (coordinateParts.size() == 2) {
            return {coordinateParts.at(0).toDouble(), coordinateParts.at(1).toDouble()};
        }
    }

    return {};
}

bool QmlUtils::openGeoLocation(const QGeoCoordinate &geoCoordinate)
{
    switch (AccountManager::instance()->account().geoLocationMapService) {
    case Account::GeoLocationMapService::System:
        QDesktopServices::openUrl(QUrl(geoUri(geoCoordinate)));
        break;
    case Account::GeoLocationMapService::InApp:
        return true;
    case Account::GeoLocationMapService::Web:
        QDesktopServices::openUrl(QUrl(GEO_LOCATION_WEB_URL.arg(QString::number(geoCoordinate.latitude()), QString::number(geoCoordinate.longitude()))));
        break;
    }

    return false;
}

#include "moc_QmlUtils.cpp"
