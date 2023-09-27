// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Algorithms.h"
#include "MediaUtils.h"
#include "Message.h"
#include <QXmppBitsOfBinaryContentId.h>
#include <QXmppBitsOfBinaryData.h>
#include <QXmppBitsOfBinaryDataList.h>
#include <QXmppE2eeMetadata.h>
#include <QXmppFileMetadata.h>
#include <QXmppOutOfBandUrl.h>
#include <QXmppThumbnail.h>

#include <QFileInfo>
#include <QStringBuilder>

#include <QXmppHttpFileSource.h>
#include <QXmppEncryptedFileSource.h>

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
	const auto formattedSize = [this]() {
		if (size) {
			return QLocale::system().formattedDataSize(*size);
		}

		if (const QFileInfo fileInfo(localFilePath); fileInfo.exists()) {
			return QLocale::system().formattedDataSize(fileInfo.size());
		}

		return QString();
	}();
	const auto formattedDateTime = [this]() {
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

QXmppMessage Message::toQXmpp() const
{
	QXmppMessage msg;
	msg.setFrom(isOwn() ? accountJid : chatJid);
	msg.setTo(isOwn() ? chatJid : accountJid);
	msg.setId(id);
	msg.setOriginId(originId);
	msg.setStanzaId(stanzaId);
	msg.setReplaceId(replaceId);
	msg.setStamp(timestamp);
	msg.setBody(body);
	msg.setIsSpoiler(isSpoiler);
	msg.setSpoilerHint(spoilerHint);
	msg.setMarkable(true);
	msg.setMarker(marker);
	msg.setMarkerId(markerId);
	msg.setReceiptRequested(receiptRequested);

	// attached files
	msg.setSharedFiles(transform(files, [](const File &file) {
		return file.toQXmpp();
	}));

	// attach data for thumbnails
	msg.setBitsOfBinaryData(transform(files, [](const File &file) {
		return QXmppBitsOfBinaryData::fromByteArray(file.thumbnail);
	}));

	// compat for clients without Stateless File Sharing
	msg.setOutOfBandUrls(transformFilter(files, [](const File &file) -> std::optional<QXmppOutOfBandUrl> {
		if (file.httpSources.empty()) {
			return {};
		}

		QXmppOutOfBandUrl data;
		data.setUrl(file.httpSources.front().url.toString());
		data.setDescription(file.description.value_or(QString()));
		return data;
	}));

	return msg;
}

bool Message::isOwn() const
{
	return accountJid == senderId;
}

QString Message::previewText() const
{
	if (isSpoiler) {
		if (spoilerHint.isEmpty()) {
			return tr("Spoiler");
		}
		return spoilerHint;
	}

	if (files.empty()) {
		return body;
	}

	// Use first file for detection (could be improved with more complex logic)
	auto mediaType = MediaUtils::messageType(files.front().mimeType);
	auto text = MediaUtils::mediaTypeName(mediaType);

	if (!body.isEmpty()) {
		return text % QStringLiteral(": ") % body;
	}
	return text;
}
