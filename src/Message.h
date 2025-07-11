// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QCoreApplication>
#include <QImage>
#include <QMimeType>
#include <QUrl>
// QXmpp
#include <QXmppHash.h>
#include <QXmppMessage.h>
// Kaidan
#include "Encryption.h"
#include "Enums.h"

class QGeoCoordinate;

using namespace Enums;

struct FileHash {
    qint64 dataId;
    QXmpp::HashAlgorithm hashType;
    QByteArray hashValue;

    QXmppHash toQXmpp() const;

    bool operator==(const FileHash &other) const = default;
};

struct HttpSource {
    qint64 fileId;
    QUrl url;

    QXmppHttpFileSource toQXmpp() const;

    bool operator==(const HttpSource &other) const = default;
};

struct EncryptedSource {
    qint64 fileId;
    QUrl url;
    QXmpp::Cipher cipher;
    QByteArray key;
    QByteArray iv;
    std::optional<qint64> encryptedDataId;
    QList<FileHash> encryptedHashes;

    QXmppEncryptedFileSource toQXmpp() const;

    bool operator==(const EncryptedSource &other) const = default;
};

struct File {
    Q_GADGET
    Q_PROPERTY(QString fileId READ fileId CONSTANT)
    Q_PROPERTY(qint64 fileGroupId MEMBER fileGroupId)
    Q_PROPERTY(QString name READ _name CONSTANT)
    Q_PROPERTY(QString description READ _description CONSTANT)
    Q_PROPERTY(QString mimeTypeName READ mimeTypeName CONSTANT)
    Q_PROPERTY(QString mimeTypeIcon READ mimeTypeIcon CONSTANT)
    Q_PROPERTY(QString formattedSize READ formattedSize CONSTANT)
    Q_PROPERTY(QDateTime lastModified MEMBER lastModified)
    Q_PROPERTY(bool displayInline READ displayInline CONSTANT)
    Q_PROPERTY(QUrl downloadUrl READ downloadUrl CONSTANT)
    Q_PROPERTY(QString localFilePath MEMBER localFilePath)
    Q_PROPERTY(QUrl localFileUrl READ localFileUrl CONSTANT)
    Q_PROPERTY(QString externalId MEMBER externalId)
    Q_PROPERTY(QImage previewImage READ previewImage CONSTANT)
    Q_PROPERTY(Enums::MessageType type READ type CONSTANT)
    Q_PROPERTY(QString details READ details CONSTANT)
    Q_PROPERTY(bool isNew MEMBER isNew)

public:
    qint64 id = 0;
    qint64 fileGroupId = 0;
    std::optional<QString> name;
    std::optional<QString> description;
    QMimeType mimeType;
    std::optional<qint64> size;
    uint32_t width;
    uint32_t height;
    QDateTime lastModified;
    QXmppFileShare::Disposition disposition = QXmppFileShare::Attachment;
    QString localFilePath;
    QString externalId;
    QList<FileHash> hashes;
    QByteArray thumbnail;
    QList<HttpSource> httpSources;
    QList<EncryptedSource> encryptedSources;
    bool isNew;

    [[nodiscard]] QXmppFileShare toQXmpp() const;

    bool operator==(const File &other) const = default;

    [[nodiscard]] QString _name() const
    {
        return name.value_or(QString());
    }
    [[nodiscard]] QString _description() const
    {
        return description.value_or(QString());
    }
    [[nodiscard]] QString mimeTypeName() const
    {
        return mimeType.name();
    }
    [[nodiscard]] QString mimeTypeIcon() const
    {
        return mimeType.iconName();
    }
    [[nodiscard]] qint64 _size() const
    {
        return size.value_or(-1);
    }
    [[nodiscard]] bool displayInline() const
    {
        return disposition == QXmppFileShare::Inline;
    }
    [[nodiscard]] QImage previewImage() const;
    [[nodiscard]] QUrl downloadUrl() const;
    [[nodiscard]] QUrl localFileUrl() const;
    [[nodiscard]] Enums::MessageType type() const;
    [[nodiscard]] QString fileId() const
    {
        return QString::number(id);
    }
    [[nodiscard]] QString details() const;
    [[nodiscard]] QString formattedSize() const;
    [[nodiscard]] QString formattedDateTime() const;

private:
    QImage createPreviewImage() const;
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

struct MessageReaction {
    Q_GADGET
    Q_PROPERTY(QString emoji MEMBER emoji)
    Q_PROPERTY(MessageReactionDeliveryState::Enum deliveryState MEMBER deliveryState)

public:
    QString emoji;
    MessageReactionDeliveryState::Enum deliveryState = MessageReactionDeliveryState::Delivered;

    bool operator==(const MessageReaction &other) const = default;
};

Q_DECLARE_METATYPE(MessageReaction)

struct MessageReactionSender {
    QDateTime latestTimestamp;
    QList<MessageReaction> reactions;

    bool operator==(const MessageReactionSender &other) const = default;
};

struct GroupChatInvitation {
    QString inviterJid;
    QString inviteeJid;
    QString groupChatJid;
    QString token;

    [[nodiscard]] QXmppMixInvitation toQXmpp() const;

    bool operator==(const GroupChatInvitation &other) const = default;
};

Q_DECLARE_METATYPE(GroupChatInvitation)

class Message
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(Message)

public:
    enum class TrustLevel {
        Untrusted,
        Trusted,
        Authenticated,
    };
    Q_ENUM(TrustLevel)

    struct Reply {
        // Empty for a reply to an own message in a normal chat or for a reply in an anonymous group
        // chat.
        QString toJid;
        // Empty in a normal chat or for a reply to an own message in a group chat.
        QString toGroupChatParticipantId;
        // Empty for a reply to an own message in a group chat.
        QString toGroupChatParticipantName;
        QString id;
        QString quote;

        bool operator==(const Reply &other) const;
    };

    /**
     * Compares another @c Message with this. Only attributes that are saved in the
     * database are checked.
     */
    bool operator==(const Message &m) const = default;
    bool operator!=(const Message &m) const = default;

    QString accountJid;
    QString chatJid;
    bool isOwn;
    QString groupChatSenderId;
    QString groupChatSenderJid;
    QString groupChatSenderName;
    QString id;
    QString originId;
    QString stanzaId;
    QString replaceId;
    std::optional<Reply> reply;
    QDateTime timestamp;
    // End-to-end encryption used for this message.
    Encryption::Enum encryption = Encryption::NoEncryption;
    // ID of this message's sender's public long-term end-to-end encryption key.
    QByteArray senderKey;
    QXmpp::TrustLevel preciseTrustLevel;
    // Delivery state of the message, like if it was sent successfully or if it was already delivered
    DeliveryState deliveryState = DeliveryState::Sent;
    bool isSpoiler = false;
    QString spoilerHint;
    std::optional<qint64> fileGroupId;
    QList<File> files;
    QXmppMessage::Marker marker = QXmppMessage::NoMarker;
    QString markerId;
    bool receiptRequested = false;
    // IDs (JIDs or group chat participant IDs) of senders mapped to the senders
    QMap<QString, MessageReactionSender> reactionSenders;
    std::optional<GroupChatInvitation> groupChatInvitation;

    // Text description of an error if it ever happened to the message
    QString errorText;
    // True if the message's content and related data such as files have been removed locally.
    bool removed = false;

    [[nodiscard]] QXmppMessage toQXmpp() const;
    [[nodiscard]] QList<QXmppMessage> fileFallbackMessages() const;

    QString relevantId() const;
    QString senderJid() const;
    bool isServerMessage() const;
    bool isGroupChatMessage() const;

    QString body() const;
    void setPreparedBody(const QString &preparedBody);
    void setUnpreparedBody(const QString &unpreparedBody);

    // Text to be shown for the user depending on the message's content
    QString text() const;

    // Preview of the message in pure text form (used in the contact list for the
    // last message for example)
    QString previewText() const;

    QGeoCoordinate geoCoordinate() const;

    TrustLevel trustLevel() const;

private:
    void toQXmppBase(QXmppMessage &message) const;
    bool fileFallbackIncludedInMainMessage() const;
    static bool addFileFallbackMessageBase(QXmppMessage &message, const File &file);
    QString groupChatInvitationText() const;

    QString m_body;
};

Q_DECLARE_METATYPE(Message)
Q_DECLARE_METATYPE(Message::TrustLevel)

enum class MessageOrigin : quint8 {
    Stream,
    UserInput,
    MamInitial,
    MamCatchUp,
    MamBacklog,
};

Q_DECLARE_METATYPE(MessageOrigin);
