// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "QmlUtils.h"
// Qt
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringBuilder>
// QXmpp
#include "qxmpp-exts/QXmppColorGenerator.h"
#include "qxmpp-exts/QXmppUri.h"
// Kaidan
#include "MessageModel.h"

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
#ifdef UBUNTU_TOUCH
	pathList << QString("./share/") + QString(APPLICATION_NAME);
#endif
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
	qWarning() << "[main] Could NOT find media file:" << name;
	return QString();
}

QUrl QmlUtils::applicationWebsiteUrl()
{
	return QUrl(QStringLiteral(APPLICATION_WEBSITE_URL));
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

QUrl QmlUtils::trustMessageUri(const QString &jid)
{
	return QUrl(trustMessageUriString(jid));
}

QString QmlUtils::trustMessageUriString(const QString &jid)
{
	const auto keys = MessageModel::instance()->keys().value(jid);
	QList<QString> authenticatedKeys;
	QList<QString> distrustedKeys;

	for (auto itr = keys.constBegin(); itr != keys.constEnd(); ++itr) {
		const auto key = QString::fromUtf8(itr.key().toHex());
		const auto trustLevel = itr.value();

		if (trustLevel == QXmpp::TrustLevel::Authenticated) {
			authenticatedKeys.append(key);
		} else if (trustLevel == QXmpp::TrustLevel::ManuallyDistrusted) {
			distrustedKeys.append(key);
		}
	}

	QXmppUri uri;
	uri.setJid(jid);

	// Create a Trust Message URI only if there are keys for it.
	if (!authenticatedKeys.isEmpty() || !distrustedKeys.isEmpty()) {
		uri.setAction(QXmppUri::TrustMessage);
		// TODO: Find solution to pass enum to "uri.setEncryption()" instead of string (see QXmppGlobal::encryptionToString())
		uri.setEncryption(QStringLiteral("urn:xmpp:omemo:2"));
		uri.setTrustedKeysIds(authenticatedKeys);
		uri.setDistrustedKeysIds(distrustedKeys);
	}

	return uri.toString();
}

QUrl QmlUtils::groupChatUri(const QString &groupChatJid)
{
	QXmppUri uri;
	uri.setJid(groupChatJid);
	uri.setAction(QXmppUri::Join);
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

bool QmlUtils::isImageFile(const QUrl &fileUrl)
{
	QMimeType type = QMimeDatabase().mimeTypeForUrl(fileUrl);
	return type.inherits(QStringLiteral("image/jpeg")) || type.inherits(QStringLiteral("image/png"));
}

void QmlUtils::copyToClipboard(const QString &text)
{
	QGuiApplication::clipboard()->setText(text);
}

QString QmlUtils::fileNameFromUrl(const QUrl &url)
{
	return QUrl(url).fileName();
}

QString QmlUtils::formattedDataSize(qint64 fileSize)
{
	if (fileSize < 0) {
		return tr("Unknown size");
	}

	return QLocale::system().formattedDataSize(fileSize);
}

QString QmlUtils::formatMessage(const QString &message)
{
	// escape all special XML chars (like '<' and '>')
	// and spilt into words for processing
	return processMsgFormatting(message.toHtmlEscaped().split(QStringLiteral(" ")));
}

QColor QmlUtils::getUserColor(const QString &nickName)
{
	QXmppColorGenerator::RGBColor color = QXmppColorGenerator::generateColor(nickName);
	return {color.red, color.green, color.blue};
}

QUrl QmlUtils::pasteImage()
{
	const auto image = QGuiApplication::clipboard()->image();
	if (image.isNull())
		return {};

	// create absolute file path
	const auto path = downloadPath(u"image-" % timestampForFileName() % u".jpg");

	// encode JPEG image
	if (!image.save(path, "JPG", JPEG_EXPORT_QUALITY))
		return {};
	return QUrl::fromLocalFile(path);
}

QString QmlUtils::downloadPath(const QString &filename)
{
	// Kaidan download directory
	const QDir directory(
			QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) %
				QDir::separator() %
				QStringLiteral(APPLICATION_DISPLAY_NAME));
	// create directory if it doesn't exist
	if (!directory.mkpath(QStringLiteral(".")))
		return {};

	// check whether a file with this name already exists
	QFileInfo info(directory, filename);
	if (!info.exists())
		return info.absoluteFilePath();

	// find a new filename that doesn't exist
	const auto baseName = info.baseName();
	const auto nameSuffix = info.suffix();
	for (uint i = 1; info.exists(); i++) {
		// try to insert '-<i>' before the file extension
		info = QFileInfo(directory, baseName % u'-' % QString::number(i) % nameSuffix);
	}
	return info.absoluteFilePath();
}

QString QmlUtils::timestampForFileName()
{
	return timestampForFileName(QDateTime::currentDateTime());
}

QString QmlUtils::timestampForFileName(const QDateTime &dateTime)
{
	return dateTime.toString(u"yyyyMMdd_hhmmss");
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

QString QmlUtils::processMsgFormatting(const QStringList &list, bool isFirst)
{
	if (list.isEmpty())
		return QString();

	// link highlighting
	if (list.first().startsWith(QStringLiteral("https://")) || list.first().startsWith(QStringLiteral("http://")))
		return (isFirst ? QString() : QStringLiteral(" ")) + QStringLiteral("<a href='%1'>%1</a>").arg(list.first())
		       + processMsgFormatting(list.mid(1), false);

	// preserve newlines
	if (list.first().contains(QStringLiteral("\n")))
		return (isFirst ? QString() : QStringLiteral(" ")) + QString(list.first()).replace(QStringLiteral("\n"), QStringLiteral("<br>"))
		       + processMsgFormatting(list.mid(1), false);

	return (isFirst ? QString() : QStringLiteral(" ")) + list.first() + processMsgFormatting(list.mid(1), false);
}

QString QmlUtils::osmUserAgent()
{
	return u"" APPLICATION_NAME % QChar(u'/') % u"" VERSION_STRING;
}
