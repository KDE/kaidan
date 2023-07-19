// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QCoreApplication>
#include <QUrl>
#include <QImage>
#include <QMimeType>
// QXmpp
#include <QXmppMessage.h>
#include <QXmppHash.h>
// Kaidan
#include "Encryption.h"
#include "Enums.h"

class QMimeType;

using namespace Enums;

struct FileHash
{
	qint64 dataId;
	QXmpp::HashAlgorithm hashType;
	QByteArray hashValue;

	QXmppHash toQXmpp() const;

	bool operator==(const FileHash &other) const = default;
};

struct HttpSource
{
	qint64 fileId;
	QUrl url;

	QXmppHttpFileSource toQXmpp() const;

	bool operator==(const HttpSource &other) const = default;
};

struct EncryptedSource
{
	qint64 fileId;
	QUrl url;
	QXmpp::Cipher cipher;
	QByteArray key;
	QByteArray iv;
	std::optional<qint64> encryptedDataId;
	QVector<FileHash> encryptedHashes;

	QXmppEncryptedFileSource toQXmpp() const;

	bool operator==(const EncryptedSource &other) const = default;
};

struct File
{
	Q_GADGET
	Q_PROPERTY(QString fileId READ fileId CONSTANT)
	Q_PROPERTY(qint64 fileGroupId MEMBER fileGroupId)
	Q_PROPERTY(QString name READ _name CONSTANT)
	Q_PROPERTY(QString description READ _description CONSTANT)
	Q_PROPERTY(QString mimeTypeName READ mimeTypeName CONSTANT)
	Q_PROPERTY(QString mimeTypeIcon READ mimeTypeIcon CONSTANT)
	Q_PROPERTY(qint64 size READ _size CONSTANT)
	Q_PROPERTY(QDateTime lastModified MEMBER lastModified)
	Q_PROPERTY(bool displayInline READ displayInline CONSTANT)
	Q_PROPERTY(QString localFilePath MEMBER localFilePath)
	Q_PROPERTY(QUrl localFileUrl READ localFileUrl CONSTANT)
	Q_PROPERTY(bool hasThumbnail READ hasThumbnail CONSTANT)
	Q_PROPERTY(QImage thumbnail READ thumbnailImage CONSTANT)
	Q_PROPERTY(QImage thumbnailSquare READ thumbnailSquareImage CONSTANT)
	Q_PROPERTY(QUrl downloadUrl READ downloadUrl CONSTANT)
	Q_PROPERTY(Enums::MessageType type READ type CONSTANT)
	Q_PROPERTY(QString details READ details CONSTANT)

public:
	qint64 id = 0;
	qint64 fileGroupId = 0;
	std::optional<QString> name;
	std::optional<QString> description;
	QMimeType mimeType;
	std::optional<qint64> size;
	QDateTime lastModified;
	QXmppFileShare::Disposition disposition = QXmppFileShare::Attachment;
	QString localFilePath;
	QVector<FileHash> hashes;
	QByteArray thumbnail;
	QVector<HttpSource> httpSources;
	QVector<EncryptedSource> encryptedSources;

	[[nodiscard]] QXmppFileShare toQXmpp() const;

	bool operator==(const File &other) const = default;

private:
	[[nodiscard]] QString _name() const { return name.value_or(QString()); }
	[[nodiscard]] QString _description() const { return description.value_or(QString()); }
	[[nodiscard]] QString mimeTypeName() const { return mimeType.name(); }
	[[nodiscard]] QString mimeTypeIcon() const { return mimeType.iconName(); }
	[[nodiscard]] qint64 _size() const { return size.value_or(-1); }
	[[nodiscard]] bool displayInline() const { return disposition == QXmppFileShare::Inline; }
	[[nodiscard]] bool hasThumbnail() const { return !thumbnail.isEmpty(); }
	[[nodiscard]] QImage thumbnailImage() const { return QImage::fromData(thumbnail); }
	[[nodiscard]] QImage thumbnailSquareImage() const;
	[[nodiscard]] QUrl downloadUrl() const;
	[[nodiscard]] QUrl localFileUrl() const;
	[[nodiscard]] Enums::MessageType type() const;
	[[nodiscard]] QString fileId() const { return QString::number(id); }
	[[nodiscard]] QString details() const;
};

class MessageReactionDeliveryState
{
	Q_GADGET

public:
	enum Enum {
		PendingAddition,
		PendingRemovalAfterSent,
		PendingRemovalAfterDelivered,
		ErrorOnAddition,
		ErrorOnRemovalAfterSent,
		ErrorOnRemovalAfterDelivered,
		Sent,
		Delivered,
	};
	Q_ENUM(Enum)
};

struct MessageReaction
{
	Q_GADGET
	Q_PROPERTY(QString emoji MEMBER emoji)
	Q_PROPERTY(MessageReactionDeliveryState::Enum deliveryState MEMBER deliveryState)

public:
	QString emoji;
	MessageReactionDeliveryState::Enum deliveryState = MessageReactionDeliveryState::Delivered;

	bool operator==(const MessageReaction &other) const = default;
};

Q_DECLARE_METATYPE(MessageReaction)

struct MessageReactionSender
{
	QDateTime latestTimestamp;
	QVector<MessageReaction> reactions;

	bool operator==(const MessageReactionSender &other) const = default;
};

/**
 * @brief This class is used to load messages from the database and use them in
 * the @c MessageModel. The class inherits from @c QXmppMessage and most
 * properties are shared.
 */
struct Message
{
	Q_DECLARE_TR_FUNCTIONS(Message)

public:
	/**
	 * Compares another @c Message with this. Only attributes that are saved in the
	 * database are checked.
	 */
	bool operator==(const Message &m) const = default;
	bool operator!=(const Message &m) const = default;

	QString id;
	QString to;
	QString from;
	QString body;
	QDateTime stamp;
	bool isSpoiler = false;
	QString spoilerHint;
	QXmppMessage::Marker marker = QXmppMessage::NoMarker;
	QString markerId;
	QString replaceId;
	QString originId;
	QString stanzaId;
	std::optional<qint64> fileGroupId;
	QVector<File> files;
	bool receiptRequested = false;
	// JIDs of senders mapped to the senders
	QMap<QString, MessageReactionSender> reactionSenders;
	// End-to-end encryption used for this message.
	Encryption::Enum encryption = Encryption::NoEncryption;
	// ID of this message's sender's public long-term end-to-end encryption key.
	QByteArray senderKey;
	// True if the message is an own one.
	bool isOwn = true;
	// Delivery state of the message, like if it was sent successfully or if it was already delivered
	DeliveryState deliveryState = DeliveryState::Delivered;
	// Text description of an error if it ever happened to the message
	QString errorText;
	// True if the message's content and related data such as files have been removed locally.
	bool removed = false;

	[[nodiscard]] QXmppMessage toQXmpp() const;

	// Preview of the message in pure text form (used in the contact list for the
	// last message for example)
	QString previewText() const;
};

Q_DECLARE_METATYPE(Message)

enum class MessageOrigin : quint8 {
	Stream,
	UserInput,
	MamInitial,
	MamCatchUp,
	MamBacklog,
};

Q_DECLARE_METATYPE(MessageOrigin);
