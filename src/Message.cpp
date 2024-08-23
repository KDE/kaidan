// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Message.h"

#include <QFileInfo>
#include <QStringBuilder>

#include <QXmppBitsOfBinaryContentId.h>
#include <QXmppBitsOfBinaryData.h>
#include <QXmppBitsOfBinaryDataList.h>
#include <QXmppE2eeMetadata.h>
#include <QXmppEncryptedFileSource.h>
#if QXMPP_VERSION >= QT_VERSION_CHECK(1, 7, 0)
#include <QXmppFallback.h>
#endif
#include <QXmppFileMetadata.h>
#include <QXmppHttpFileSource.h>
#include <QXmppMixInvitation.h>
#include <QXmppOutOfBandUrl.h>
#include <QXmppThumbnail.h>
#include <QXmppUtils.h>

#include "Algorithms.h"
#include "Globals.h"
#include "MediaUtils.h"
#include "QmlUtils.h"

QXmppHash FileHash::toQXmpp() const
{
	QXmppHash hash;
	hash.setAlgorithm(hashType);
	hash.setHash(hashValue);
	return hash;
}

QXmppHttpFileSource HttpSource::toQXmpp() const
{
	QXmppHttpFileSource source;
	source.setUrl(url);
	return source;
}

QXmppEncryptedFileSource EncryptedSource::toQXmpp() const
{
	QXmppHttpFileSource encryptedHttpSource;
	encryptedHttpSource.setUrl(url);

	QXmppEncryptedFileSource source;
	source.setHttpSources({encryptedHttpSource});
	source.setCipher(cipher);
	source.setIv(iv);
	source.setKey(key);
	source.setHashes(transform(encryptedHashes, [](const auto &fileHash) {
		return fileHash.toQXmpp();
	}));
	return source;
}

QXmppFileShare File::toQXmpp() const
{
	QXmppFileMetadata metadata;
	metadata.setFilename(name);
	metadata.setDescription(description);
	metadata.setMediaType(mimeType);
	metadata.setHashes(transform(hashes, [](const FileHash &fileHash) {
		return fileHash.toQXmpp();
	}));
	metadata.setLastModified(lastModified);
	metadata.setMediaType(mimeType);
	metadata.setSize(size);

	QXmppThumbnail thumb;
	thumb.setMediaType(QMimeDatabase().mimeTypeForData(thumbnail));
	thumb.setUri(QXmppBitsOfBinaryData::fromByteArray(thumbnail).cid().toCidUrl());
	metadata.setThumbnails({thumb});

	QXmppFileShare fs;
	fs.setDisposition(disposition);
	fs.setMetadata(metadata);
#if QXMPP_VERSION >= QT_VERSION_CHECK(1, 7, 0)
	fs.setId(externalId);
#endif
	fs.setHttpSources(transform(httpSources, [](const HttpSource &fileSource) {
		return fileSource.toQXmpp();
	}));
	fs.setEncryptedSourecs(transform(encryptedSources, [](const EncryptedSource &fileSource) {
		return fileSource.toQXmpp();
	}));
	return fs;
}

QImage File::thumbnailSquareImage() const
{
	auto image = QImage::fromData(thumbnail);
	auto length = std::min(image.width(), image.height());
	QImage croppedImage(QSize(length, length), image.format());

	auto delX = (image.width() - length) / 2;
	auto delY = (image.height() - length) / 2;

	for (int x = 0; x < length; x++) {
		for (int y = 0; y < length; y++) {
			croppedImage.setPixel(x, y, image.pixel(x + delX, y + delY));
		}
	}
	return croppedImage;
}

QUrl File::downloadUrl() const
{
	if (!httpSources.isEmpty()) {
		return httpSources.front().url;
	}
	// don't use encrypted source urls (can't be opened externally)
	return {};
}

QUrl File::localFileUrl() const
{
	return localFilePath.isEmpty() ? QUrl() : QUrl::fromLocalFile(localFilePath);
}

MessageType File::type() const
{
	return MediaUtils::messageType(mimeType);
}

QString File::details() const
{
	auto formattedSize = [this]() {
		if (size) {
			return QmlUtils::formattedDataSize(*size);
		}

		if (const QFileInfo fileInfo(localFilePath); fileInfo.exists()) {
			return QmlUtils::formattedDataSize(fileInfo.size());
		}

		return QString();
	}();
	auto formattedDateTime = [this]() {
		if (lastModified.isValid()) {
			return QLocale::system().toString(lastModified, QObject::tr("dd MMM at hh:mm"));
		}

		return QString();
	}();

	if (formattedSize.isEmpty() && formattedDateTime.isEmpty()) {
		return QObject::tr("No information");
	}

	if (formattedSize.isEmpty()) {
		return formattedDateTime;
	}

	if (formattedDateTime.isEmpty()) {
		return formattedSize;
	}

	return QStringLiteral("%1, %2").arg(formattedSize, formattedDateTime);
}

QXmppMixInvitation GroupChatInvitation::toQXmpp() const
{
	QXmppMixInvitation mixInvitation;

	mixInvitation.setInviterJid(inviterJid);
	mixInvitation.setInviteeJid(inviteeJid);
	mixInvitation.setChannelJid(groupChatJid);
	mixInvitation.setToken(token);

	return mixInvitation;
}

QXmppMessage Message::toQXmpp() const
{
	QXmppMessage msg;

	if (isGroupChatMessage()) {
		msg.setType(QXmppMessage::GroupChat);
	}

	msg.setFrom(isOwn ? accountJid : chatJid);
	msg.setTo(isOwn ? chatJid : accountJid);
	msg.setId(id);
	msg.setOriginId(originId);
	msg.setReplaceId(replaceId);

	if (reply) {
		QXmpp::Reply qxmppReply;

		qxmppReply.to = isGroupChatMessage() ? reply->toGroupChatparticipantId  : reply->toJid;
		qxmppReply.id = reply->id;

		msg.setReply(qxmppReply);

		if (!reply->quote.isEmpty()) {
			const auto quote = QmlUtils::quote(reply->quote);

			msg.setBody(quote + body);
			msg.setFallbackMarkers({ QXmppFallback {
				XMLNS_MESSAGE_REPLIES.toString(),
				{ QXmppFallback::Reference { QXmppFallback::Body, QXmppFallback::Range { 0, uint32_t(quote.size()) } } },
			}});
		} else {
			msg.setBody(body);
		}
	} else {
		msg.setBody(body);
	}

	msg.setStamp(timestamp);
	msg.setIsSpoiler(isSpoiler);
	msg.setSpoilerHint(spoilerHint);
	msg.setMarkable(true);
	msg.setMarker(marker);
	msg.setMarkerId(markerId);
	msg.setReceiptRequested(!isGroupChatMessage());

	// attached files
	msg.setSharedFiles(transform(files, [](const File &file) {
		return file.toQXmpp();
	}));

	// attach data for thumbnails
	msg.setBitsOfBinaryData(transform(files, [](const File &file) {
		return QXmppBitsOfBinaryData::fromByteArray(file.thumbnail);
	}));

	// compat for clients without Stateless File Sharing
	if (!files.empty() && includeFileFallbackInMainMessage() && !files.first().httpSources.empty()) {
		auto url = files.first().httpSources.first().url.toString();

		// body
		msg.setBody(url);

		// Out-of-band url
		QXmppOutOfBandUrl oobUrl;
		oobUrl.setUrl(url);
		oobUrl.setDescription(files.first().description);
		msg.setOutOfBandUrls({ oobUrl });

		// fallback indication for SFS
#if QXMPP_VERSION >= QT_VERSION_CHECK(1, 7, 0)
		msg.setFallbackMarkers({ QXmppFallback {
			XMLNS_SFS.toString(),
			{ QXmppFallback::Reference { QXmppFallback::Body, {} } },
		}});
#else
		msg.setIsFallback(true);
#endif
	}

	if (groupChatInvitation) {
		msg.setMixInvitation(groupChatInvitation->toQXmpp());
	}

	return msg;
}

QVector<QXmppMessage> Message::fallbackMessages() const
{
#if QXMPP_VERSION >= QT_VERSION_CHECK(1, 7, 0)
	if (files.empty() || includeFileFallbackInMainMessage()) {
		return {};
	}

	// generate fallback messages for each file
	int i = 0;
	return transformFilter(files, [&](const File &file) -> std::optional<QXmppMessage> {
		if (file.httpSources.empty()) {
			return {};
		}

		QXmppMessage m;
		m.setFrom(isOwn ? accountJid : chatJid);
		m.setTo(isOwn ? chatJid : accountJid);
		m.setId(id % u'_' % QString::number(++i));
		// if the original message was a spoiler, this should be too if not recognized as fallback
		m.setIsSpoiler(isSpoiler);
		m.setSpoilerHint(spoilerHint);

		auto url = files.first().httpSources.first().url.toString();

		// body
		m.setBody(url);

		// Out-of-band url
		QXmppOutOfBandUrl oobUrl;
		oobUrl.setUrl(url);
		oobUrl.setDescription(files.first().description);
		m.setOutOfBandUrls({ oobUrl });

		// fallback indication for SFS
		m.setFallbackMarkers({ QXmppFallback {
			XMLNS_SFS.toString(),
			{ QXmppFallback::Reference { QXmppFallback::Body, {} } },
		}});

		// reference to original message
		m.setAttachId(id);
		return m;
	});
#else
	return {};
#endif
}

QString Message::relevantId() const
{
	if (!replaceId.isEmpty()) {
		return replaceId;
	}

	if (!stanzaId.isEmpty() && (isOwn || isServerMessage() || isGroupChatMessage())) {
		return stanzaId;
	}

	if (!originId.isEmpty()) {
		return originId;
	}

	return id;
}

QString Message::senderJid() const
{
	if (isOwn) {
		return accountJid;
	}

	if (isGroupChatMessage()) {
		return groupChatSenderJid;
	}

	return chatJid;
}

bool Message::isServerMessage() const
{
	return QXmppUtils::jidToDomain(accountJid) == chatJid;
}

bool Message::isGroupChatMessage() const
{
	return !groupChatSenderId.isEmpty();
}

QString Message::text() const
{
	if (groupChatInvitation) {
		return groupChatInvitationText();
	}

	if (files.isEmpty()) {
		return body.isEmpty() ? QString() : body;
	}

	return {};
}

QString Message::previewText() const
{
	if (isSpoiler) {
		if (spoilerHint.isEmpty()) {
			return tr("Spoiler");
		}
		return spoilerHint;
	}

	if (groupChatInvitation) {
		return groupChatInvitationText();
	}

	if (files.isEmpty()) {
		return body;
	}

	// Use first file for detection (could be improved with more complex logic)
	auto mediaType = MediaUtils::messageType(files.front().mimeType);
	auto mediaPreviewText = MediaUtils::mediaTypeName(mediaType);

	if (!body.isEmpty()) {
		return mediaPreviewText % QStringLiteral(": ") % body;
	}

	return mediaPreviewText;
}

Message::TrustLevel Message::trustLevel() const
{
	if (preciseTrustLevel == QXmpp::TrustLevel::Authenticated) {
		return Message::TrustLevel::Authenticated;
	}

	if ((QXmpp::TrustLevel::AutomaticallyTrusted | QXmpp::TrustLevel::ManuallyTrusted).testFlag(preciseTrustLevel)) {
		return Message::TrustLevel::Trusted;
	}

	return Message::TrustLevel::Untrusted;
}

bool Message::includeFileFallbackInMainMessage() const
{
	return files.size() == 1 && body.isEmpty();
}

QString Message::groupChatInvitationText() const
{
	return tr("Invitation to group %1").arg(groupChatInvitation->groupChatJid);
}

bool Message::Reply::operator==(const Reply &other) const
{
	return
		toJid == other.toJid &&
		toGroupChatparticipantId == other.toGroupChatparticipantId &&
		id == other.id &&
		quote == other.quote;
}
