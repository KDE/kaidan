// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Carson Black <uhhadd@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "QmlUtils.h"
// ICU
#include <unicode/uchar.h>
// Qt
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QMimeDatabase>
#include <QPalette>
#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QTextBoundaryFinder>
#include <QTextCursor>
// QXmpp
#include "qxmpp-exts/QXmppColorGenerator.h"
#include "qxmpp-exts/QXmppUri.h"
// Kaidan
#include "Algorithms.h"
#include "MessageModel.h"

const auto EMOJI_FONT_FAMILY = QStringLiteral("emoji");
constexpr auto URL_PREFIX = u"https://";
const auto PADDING_CHARACTER = u'⠀';
constexpr auto SINGLE_EMOJI_SIZE_FACTOR = 3;
constexpr auto MULTIPLE_EMOJIS_SIZE_FACTOR = 2;
constexpr auto MIXED_TEXT_EMOJI_SIZE_FACTOR = 1.3;

enum class TextType {
	Mixed,
	SingleEmoji,
	MultipleEmojis,
};

static TextType determineTextType(const QString &text) {
	QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
	auto emojiCounter = 0;

	for (auto start = 0, end = 0; finder.toNextBoundary() != -1; start = end) {
		end = finder.position();
		auto part = text.mid(start, end - start).toUcs4()[0];
		const auto firstCodepoint = text[start];

		if (firstCodepoint.isSpace() || firstCodepoint == PADDING_CHARACTER) {
			continue;
		}

		if (u_hasBinaryProperty(part, UCHAR_EMOJI_PRESENTATION)) {
			emojiCounter++;
		} else {
			return TextType::Mixed;
		}
	}

	switch (emojiCounter) {
	case 0:
		return TextType::Mixed;
	case 1:
		return TextType::SingleEmoji;
	default:
		return TextType::MultipleEmojis;
	}
}

static auto isTextSeparator(QChar character)
{
	return character.isSpace() || character == PADDING_CHARACTER;
}

// Formats emojis to use an appropriate emoji font that usually displays coloured emojis.
static void formatEmojis(QTextCursor &cursor, const QString &text, double emojiFontSizeFactor)
{
	QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);

	for (auto start = 0, end = 0; finder.toNextBoundary() != -1; start = end) {
		end = finder.position();
		auto firstCodepoint = QStringView(text).mid(start, end - start).toUcs4()[0];

		if (u_hasBinaryProperty(firstCodepoint, UCHAR_EMOJI_PRESENTATION)) {
			cursor.setPosition(start, QTextCursor::MoveAnchor);
			cursor.setPosition(end, QTextCursor::KeepAnchor);

			auto font = QGuiApplication::font();
			font.setFamily(EMOJI_FONT_FAMILY);
			font.setPointSize(font.pointSize() * emojiFontSizeFactor);

			QTextCharFormat format;
			format.setFont(font);
			cursor.setCharFormat(format);
		}
	}
}

// Marks and hightlights URLs to be displayed as links that can be opened.
static void formatUrls(QTextCursor &cursor, const QString &text)
{
	processTextParts(text, isTextSeparator, [&cursor](qsizetype i, QStringView part) {
		if (part.startsWith(URL_PREFIX)) {
			cursor.setPosition(i, QTextCursor::MoveAnchor);
			cursor.setPosition(i + part.size(), QTextCursor::KeepAnchor);

			QTextCharFormat format;

			format.setAnchor(true);
			format.setAnchorHref(part.toString());
			format.setForeground(QPalette().link());

			cursor.setCharFormat(format);
		}
	});
}

static QmlUtils *s_instance;

QmlUtils *QmlUtils::instance()
{
	if (!s_instance)
		return new QmlUtils(QGuiApplication::instance());

	return s_instance;
}

template<typename FormatText>
static void attachFormatting(QQuickTextDocument *document, double emojiFontSizeFactor, FormatText formatText) {
	auto textDocument = document->textDocument();

	// Avoid calling this function again and creating an infinite loop if this function modifies the
	// text document.
	QObject::disconnect(textDocument, &QTextDocument::contentsChanged, s_instance, nullptr);

	QTextCursor cursor(textDocument);
	auto text = textDocument->toRawText();

	formatEmojis(cursor, text, emojiFontSizeFactor);
	formatText(cursor, text);

	// Connect this function in order to be called each time the text document is modified.
	// That way, text changes via QML cause the formatting to be applied.
	QObject::connect(textDocument, &QTextDocument::contentsChanged, s_instance, [document, emojiFontSizeFactor, formatText]() {
		attachFormatting(document, emojiFontSizeFactor, formatText);
	});
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

void QmlUtils::attachTextFormatting(QQuickTextDocument *document)
{
	attachFormatting(document, MIXED_TEXT_EMOJI_SIZE_FACTOR, [](auto, auto) {});
}

void QmlUtils::attachEnhancedTextFormatting(QQuickTextDocument *document)
{
	double emojiFontSizeFactor;

	switch (determineTextType(document->textDocument()->toRawText())) {
	case TextType::Mixed:
		emojiFontSizeFactor = MIXED_TEXT_EMOJI_SIZE_FACTOR;
		break;
	case TextType::SingleEmoji:
		emojiFontSizeFactor = SINGLE_EMOJI_SIZE_FACTOR;
		break;
	case TextType::MultipleEmojis:
		emojiFontSizeFactor = MULTIPLE_EMOJIS_SIZE_FACTOR;
		break;
	}

	attachFormatting(document, emojiFontSizeFactor, [](QTextCursor &cursor, const QString &text) {
		formatUrls(cursor, text);
	});
}

QChar QmlUtils::paddingCharacter()
{
	return PADDING_CHARACTER;
}

QColor QmlUtils::userColor(const QString &id, const QString &name)
{
	QXmppColorGenerator::RGBColor rgbColor = QXmppColorGenerator::generateColor(id.isEmpty() ? name : id);
	return { rgbColor.red, rgbColor.green, rgbColor.blue };
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

QString QmlUtils::osmUserAgent()
{
	return u"" APPLICATION_NAME % QChar(u'/') % u"" VERSION_STRING;
}
