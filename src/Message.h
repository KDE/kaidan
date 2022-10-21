/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

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

	bool operator==(const FileHash &other) const;
};

struct HttpSource
{
	qint64 fileId;
	QUrl url;

	QXmppHttpFileSource toQXmpp() const;

	bool operator==(const HttpSource &other) const;
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

	bool operator==(const EncryptedSource &other) const;
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
	Q_PROPERTY(bool hasThumbnail READ hasThumbnail CONSTANT)
	Q_PROPERTY(QImage thumbnail READ thumbnailImage CONSTANT)
	Q_PROPERTY(QImage thumbnailSquare READ thumbnailSquareImage CONSTANT)
	Q_PROPERTY(QUrl downloadUrl READ downloadUrl CONSTANT)
	Q_PROPERTY(Enums::MessageType type READ type CONSTANT)

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

	bool operator==(const File &other) const;

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
	[[nodiscard]] Enums::MessageType type() const;
	[[nodiscard]] QString fileId() const { return QString::number(id); }
};

struct MessageReaction
{
	QDateTime latestTimestamp;
	QVector<QString> emojis;

	bool operator==(const MessageReaction &other) const;
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
	bool operator==(const Message &m) const;
	bool operator!=(const Message &m) const;

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
	// JIDs of senders mapped to their reactions
	QMap<QString, MessageReaction> reactions;
	// End-to-end encryption used for this message.
	Encryption::Enum encryption = Encryption::NoEncryption;
	// ID of this message's sender's public long-term end-to-end encryption key.
	QByteArray senderKey;
	// True if the message is an own one.
	bool isOwn = true;
	// True if the orginal message was edited.
	bool isEdited = false;
	// Delivery state of the message, like if it was sent successfully or if it was already delivered
	DeliveryState deliveryState = DeliveryState::Delivered;
	// Text description of an error if it ever happened to the message
	QString errorText;

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
