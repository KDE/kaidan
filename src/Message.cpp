// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Message.h"

// Qt
#include <QFileInfo>
#include <QStringBuilder>
// QXmpp
#include <QXmppBitsOfBinaryContentId.h>
#include <QXmppBitsOfBinaryData.h>
#include <QXmppBitsOfBinaryDataList.h>
#include <QXmppE2eeMetadata.h>
#include <QXmppEncryptedFileSource.h>
#include <QXmppFallback.h>
#include <QXmppFileMetadata.h>
#include <QXmppHttpFileSource.h>
#include <QXmppMixInvitation.h>
#include <QXmppOutOfBandUrl.h>
#include <QXmppThumbnail.h>
#include <QXmppUtils.h>
// Kaidan
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
    fs.setId(externalId);
    fs.setHttpSources(transform(httpSources, [](const HttpSource &fileSource) {
        return fileSource.toQXmpp();
    }));
    fs.setEncryptedSourecs(transform(encryptedSources, [](const EncryptedSource &fileSource) {
        return fileSource.toQXmpp();
    }));
    return fs;
}

QImage File::previewImage() const
{
    const auto image = createPreviewImage();

    if (image.isNull()) {
        return {};
    }

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
    if (MediaUtils::localFileAvailable(localFilePath)) {
        return MediaUtils::localFileUrl(localFilePath);
    }

    return QUrl();
}

MessageType File::type() const
{
    return MediaUtils::messageType(mimeType);
}

QString File::details() const
{
    const auto size = formattedSize();
    const auto dateTime = formattedDateTime();

    if (size.isEmpty() && dateTime.isEmpty()) {
        return QObject::tr("No information");
    }

    if (size.isEmpty()) {
        return dateTime;
    }

    if (dateTime.isEmpty()) {
        return size;
    }

    return QStringLiteral("%1, %2").arg(size, dateTime);
}

QString File::formattedSize() const
{
    if (size) {
        return QmlUtils::formattedDataSize(*size);
    }

    if (const QFileInfo fileInfo(localFilePath); fileInfo.exists()) {
        return QmlUtils::formattedDataSize(fileInfo.size());
    }

    return QString();
}

QString File::formattedDateTime() const
{
    if (lastModified.isValid()) {
        return QLocale::system().toString(lastModified, QObject::tr("dd MMM at hh:mm"));
    }

    return QString();
}

QImage File::createPreviewImage() const
{
    switch (type()) {
    case Enums::MessageType::MessageImage:
        if (localFileUrl().isValid()) {
            return QImage{localFilePath};
        } else if (!thumbnail.isEmpty()) {
            return QImage::fromData(thumbnail);
        }
        break;
    case Enums::MessageType::MessageVideo:
        if (!thumbnail.isEmpty()) {
            return QImage::fromData(thumbnail);
        }
        break;
    default:
        break;
    }

    return {};
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
    QXmppMessage message;
    toQXmppBase(message);

    message.setId(id);
    message.setOriginId(originId);
    message.setReplaceId(replaceId);

    if (reply) {
        QXmpp::Reply qxmppReply;

        qxmppReply.to = isGroupChatMessage() ? reply->toGroupChatparticipantId : reply->toJid;
        qxmppReply.id = reply->id;

        message.setReply(qxmppReply);

        if (!reply->quote.isEmpty()) {
            const auto quote = QmlUtils::quote(reply->quote);

            message.setBody(quote + body);
            message.setFallbackMarkers({QXmppFallback{
                XMLNS_MESSAGE_REPLIES.toString(),
                {QXmppFallback::Reference{QXmppFallback::Body, QXmppFallback::Range{0, uint32_t(quote.size())}}},
            }});
        } else {
            message.setBody(body);
        }
    } else {
        message.setBody(body);
    }

    message.setMarkable(true);
    message.setMarker(marker);
    message.setMarkerId(markerId);
    message.setReceiptRequested(!isGroupChatMessage());

    // attached files
    message.setSharedFiles(transform(files, [](const File &file) {
        return file.toQXmpp();
    }));

    // attach data for thumbnails
    message.setBitsOfBinaryData(transform(files, [](const File &file) {
        return QXmppBitsOfBinaryData::fromByteArray(file.thumbnail);
    }));

    // compat for clients without Stateless File Sharing
    if (fileFallbackIncludedInMainMessage()) {
        addFileFallbackMessageBase(message, files.first());
    }

    if (groupChatInvitation) {
        message.setMixInvitation(groupChatInvitation->toQXmpp());
    }

    return message;
}

QVector<QXmppMessage> Message::fileFallbackMessages() const
{
    if (files.isEmpty() || fileFallbackIncludedInMainMessage()) {
        return {};
    }

    int i = 0;
    return transformFilter(files, [this, &i](const File &file) -> std::optional<QXmppMessage> {
        QXmppMessage message;

        if (addFileFallbackMessageBase(message, file)) {
            toQXmppBase(message);

            const auto extendedId = id + u'_' + QString::number(++i);

            message.setId(extendedId);
            message.setOriginId(extendedId);
            message.setAttachId(id);

            return message;
        }

        return {};
    });
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

    return body.isEmpty() ? QString() : body;
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

    if (geoCoordinate().isValid()) {
        return tr("Location");
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

QGeoCoordinate Message::geoCoordinate() const
{
    return QmlUtils::geoCoordinate(body);
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

void Message::toQXmppBase(QXmppMessage &message) const
{
    if (isGroupChatMessage()) {
        message.setType(QXmppMessage::GroupChat);
    }

    message.setFrom(isOwn ? accountJid : chatJid);
    message.setTo(isOwn ? chatJid : accountJid);
    message.setStamp(timestamp);
    message.setIsSpoiler(isSpoiler);
    message.setSpoilerHint(spoilerHint);
}

bool Message::fileFallbackIncludedInMainMessage() const
{
    return files.size() == 1 && body.isEmpty();
}

bool Message::addFileFallbackMessageBase(QXmppMessage &message, const File &file)
{
    const auto httpSources = file.httpSources;
    const auto encryptedSources = file.encryptedSources;

    if (httpSources.isEmpty() && encryptedSources.isEmpty()) {
        return false;
    }

    QString url;

    if (encryptedSources.isEmpty()) {
        url = httpSources.first().url.toString();
    } else {
        url = encryptedSources.first().url.toString();
    }

    QXmppOutOfBandUrl oobUrl;
    oobUrl.setUrl(url);
    oobUrl.setDescription(file.description);

    message.setBody(url);
    message.setOutOfBandUrls({oobUrl});
    message.setFallbackMarkers({QXmppFallback{
        XMLNS_SFS.toString(),
        {QXmppFallback::Reference{QXmppFallback::Body, {}}},
    }});

    return true;
}

QString Message::groupChatInvitationText() const
{
    return tr("Invitation to group %1").arg(groupChatInvitation->groupChatJid);
}

bool Message::Reply::operator==(const Reply &other) const
{
    return toJid == other.toJid && toGroupChatparticipantId == other.toGroupChatparticipantId && id == other.id && quote == other.quote;
}

#include "moc_Message.cpp"
