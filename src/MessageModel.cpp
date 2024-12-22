// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <dev@taibsu.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageModel.h"

// Qt
#include <QGeoCoordinate>
#include <QGuiApplication>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "AccountManager.h"
#include "ChatController.h"
#include "EncryptionController.h"
#include "FutureUtils.h"
#include "Globals.h"
#include "GroupChatUser.h"
#include "GroupChatUserDb.h"
#include "Kaidan.h"
#include "MediaUtils.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "Notifications.h"
#include "RosterDb.h"
#include "RosterModel.h"
#include "TrustDb.h"

// defines that the message is suitable for correction only if it is among the N latest messages
constexpr int MAX_CORRECTION_MESSAGE_COUNT_DEPTH = 20;
// defines that the message is suitable for correction only if it has ben sent not earlier than N days ago
constexpr int MAX_CORRECTION_MESSAGE_DAYS_DEPTH = 2;

bool DisplayedMessageReaction::operator<(const DisplayedMessageReaction &other) const
{
    return emoji < other.emoji;
}

MessageModel *MessageModel::s_instance = nullptr;

MessageModel *MessageModel::instance()
{
    return s_instance;
}

MessageModel::MessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    connect(MessageDb::instance(), &MessageDb::messageAdded, this, &MessageModel::handleMessage);
    connect(MessageDb::instance(), &MessageDb::messageUpdated, this, &MessageModel::handleMessageUpdated);
    connect(MessageDb::instance(), &MessageDb::allMessagesRemovedFromChat, this, &MessageModel::removeMessages);

    connect(ChatController::instance(), &ChatController::rosterItemChanged, this, [this]() {
        if (m_lastReadOwnMessageId != ChatController::instance()->rosterItem().lastReadOwnMessageId) {
            updateLastReadOwnMessageId();
        }
    });

    connect(EncryptionController::instance(), &EncryptionController::devicesChanged, this, &MessageModel::handleDevicesChanged);
}

MessageModel::~MessageModel()
{
    s_instance = nullptr;
}

int MessageModel::rowCount(const QModelIndex &) const
{
    return m_messages.size();
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[SenderJid] = QByteArrayLiteral("senderJid");
    roles[GroupChatSenderId] = QByteArrayLiteral("groupChatSenderId");
    roles[SenderName] = QByteArrayLiteral("senderName");
    roles[Id] = QByteArrayLiteral("id");
    roles[IsLastReadOwnMessage] = QByteArrayLiteral("isLastReadOwnMessage");
    roles[IsLatestOldMessage] = QByteArrayLiteral("isLatestOldMessage");
    roles[IsEdited] = QByteArrayLiteral("isEdited");
    roles[ReplyToJid] = QByteArrayLiteral("replyToJid");
    roles[ReplyToGroupChatParticipantId] = QByteArrayLiteral("replyToGroupChatParticipantId");
    roles[ReplyToName] = QByteArrayLiteral("replyToName");
    roles[ReplyId] = QByteArrayLiteral("replyId");
    roles[ReplyQuote] = QByteArrayLiteral("replyQuote");
    roles[Date] = QByteArrayLiteral("date");
    roles[NextDate] = QByteArrayLiteral("nextDate");
    roles[Time] = QByteArrayLiteral("time");
    roles[Body] = QByteArrayLiteral("body");
    roles[Encryption] = QByteArrayLiteral("encryption");
    roles[TrustLevel] = QByteArrayLiteral("trustLevel");
    roles[DeliveryState] = QByteArrayLiteral("deliveryState");
    roles[DeliveryStateIcon] = QByteArrayLiteral("deliveryStateIcon");
    roles[DeliveryStateName] = QByteArrayLiteral("deliveryStateName");
    roles[IsSpoiler] = QByteArrayLiteral("isSpoiler");
    roles[SpoilerHint] = QByteArrayLiteral("spoilerHint");
    roles[IsOwn] = QByteArrayLiteral("isOwn");
    roles[Files] = QByteArrayLiteral("files");
    roles[DisplayedReactions] = QByteArrayLiteral("displayedReactions");
    roles[DetailedReactions] = QByteArrayLiteral("detailedReactions");
    roles[OwnReactionsFailed] = QByteArrayLiteral("ownReactionsFailed");
    roles[GroupChatInvitationJid] = QByteArrayLiteral("groupChatInvitationJid");
    roles[GeoCoordinate] = QByteArrayLiteral("geoCoordinate");
    roles[ErrorText] = QByteArrayLiteral("errorText");
    return roles;
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();

    if (!hasIndex(row, index.column(), index.parent())) {
        qWarning() << "Could not get data from message model." << index << role;
        return {};
    }
    const Message &msg = m_messages.at(row);

    switch (role) {
    case SenderJid: {
        // Return a default-constructed string for an own message.
        if (msg.isOwn) {
            return QString();
        }

        return msg.isGroupChatMessage() ? msg.groupChatSenderJid : msg.chatJid;
    }
    case GroupChatSenderId:
        // Return a default-constructed string for an own message.
        if (msg.isOwn) {
            return QString();
        }

        return msg.groupChatSenderId;
    case SenderName: {
        // Return a default-constructed string for an own message.
        if (msg.isOwn) {
            return QString();
        }

        if (msg.isGroupChatMessage()) {
            return determineGroupChatSenderName(msg);
        }

        const auto rosterItem = RosterModel::instance()->findItem(msg.chatJid);

        // On the first received message from a stranger, a new roster item is added.
        Q_ASSERT(rosterItem);

        return rosterItem->displayName();
    }
    case Id:
        return msg.relevantId();
    case IsLastReadOwnMessage:
        // A read marker text is only displayed if the message is the last read own message and
        // there is no more recent message from the contact.
        if (msg.id == m_lastReadOwnMessageId) {
            for (auto i = row; i >= 0; --i) {
                if (!m_messages.at(i).isOwn) {
                    return false;
                }
            }
            return true;
        }
        return false;
    case IsLatestOldMessage: {
        // A marker for unread messages is only displayed if the chat is not with oneself and the
        // message is the message prior to the first unread contact message.
        return msg.accountJid != msg.chatJid && row != 0 && row == m_firstUnreadContactMessageIndex + 1;
    }
    case IsEdited:
        return !msg.replaceId.isEmpty();
    case ReplyToJid: {
        const auto reply = msg.reply;

        if (!reply) {
            return QString();
        }

        const auto rosterItem = RosterModel::instance()->findItem(msg.chatJid);

        // On the first received message from a stranger, a new roster item is added.
        Q_ASSERT(rosterItem);

        if (msg.isGroupChatMessage()) {
            // Return a default-constructed string for a reply to an own message.
            if (reply->toGroupChatparticipantId == rosterItem->groupChatParticipantId) {
                return QString();
            }

            return reply->toJid;
        }

        // Return a default-constructed string for a reply to an own message.
        if (reply->toJid == rosterItem->accountJid) {
            return QString();
        }

        return reply->toJid;
    }
    case ReplyToGroupChatParticipantId: {
        if (!msg.isGroupChatMessage()) {
            return QString();
        }

        const auto reply = msg.reply;

        if (!reply) {
            return QString();
        }

        const auto rosterItem = RosterModel::instance()->findItem(msg.chatJid);

        // On the first received message from a stranger, a new roster item is added.
        Q_ASSERT(rosterItem);

        // Return a default-constructed string for a reply to an own message.
        if (reply->toGroupChatparticipantId == rosterItem->groupChatParticipantId) {
            return QString();
        }

        return reply->toGroupChatparticipantId;
    }
    case ReplyToName: {
        const auto reply = msg.reply;

        if (!reply) {
            return QString();
        }

        const auto rosterItem = RosterModel::instance()->findItem(msg.chatJid);

        // On the first received message from a stranger, a new roster item is added.
        Q_ASSERT(rosterItem);

        if (msg.isGroupChatMessage()) {
            // Return a default-constructed string for a reply to an own message.
            if (reply->toGroupChatparticipantId == rosterItem->groupChatParticipantId) {
                return QString();
            }

            return determineReplyToName(*reply);
        }

        // Return a default-constructed string for a reply to an own message.
        if (reply->toJid == rosterItem->accountJid) {
            return QString();
        }

        return rosterItem->displayName();
    }
    case ReplyId: {
        const auto reply = msg.reply;
        return reply ? reply->id : QString();
    }
    case ReplyQuote: {
        const auto reply = msg.reply;
        return reply ? reply->quote : QString();
    }
    case Date:
        return formatDate(msg.timestamp.toLocalTime().date());
    case NextDate:
        return formatDate(searchNextDate(row));
    case Time:
        return QLocale::system().toString(msg.timestamp.toLocalTime().time(), QLocale::ShortFormat);
    case Body:
        return msg.text();
    case Encryption:
        return msg.encryption;
    case TrustLevel:
        return QVariant::fromValue(msg.trustLevel());
    case DeliveryState:
        return QVariant::fromValue(msg.deliveryState);
    case DeliveryStateIcon:
        switch (msg.deliveryState) {
        case DeliveryState::Pending:
            return QStringLiteral("content-loading-symbolic");
        case DeliveryState::Sent:
            return QString();
        case DeliveryState::Delivered:
            return QStringLiteral("emblem-ok-symbolic");
        case DeliveryState::Error:
            return QStringLiteral("dialog-error-symbolic");
        case DeliveryState::Draft:
            Q_UNREACHABLE();
        }
        return {};
    case DeliveryStateName:
        switch (msg.deliveryState) {
        case DeliveryState::Pending:
            return tr("Pending");
        case DeliveryState::Sent:
            return tr("Sent");
        case DeliveryState::Delivered:
            return tr("Delivered");
        case DeliveryState::Error:
            return tr("Error");
        case DeliveryState::Draft:
            Q_UNREACHABLE();
        }
        return {};
    case IsSpoiler:
        return msg.isSpoiler;
    case SpoilerHint:
        return msg.spoilerHint;
    case IsOwn:
        return msg.isOwn;
    case Files:
        return QVariant::fromValue(msg.files);
    case DisplayedReactions: {
        QList<DisplayedMessageReaction> displayedMessageReactions;

        const auto &reactionSenders = msg.reactionSenders;
        for (auto itr = reactionSenders.begin(); itr != reactionSenders.end(); ++itr) {
            const auto ownReactionsIterated = itr.key() == ChatController::instance()->accountJid();

            for (const auto &reaction : std::as_const(itr->reactions)) {
                auto reactionItr = std::find_if(displayedMessageReactions.begin(),
                                                displayedMessageReactions.end(),
                                                [=](const DisplayedMessageReaction &displayedMessageReaction) {
                                                    return displayedMessageReaction.emoji == reaction.emoji;
                                                });

                if (ownReactionsIterated) {
                    if (reactionItr == displayedMessageReactions.end()) {
                        displayedMessageReactions.append({reaction.emoji, 1, ownReactionsIterated, reaction.deliveryState});
                    } else {
                        reactionItr->count++;
                        reactionItr->ownReactionIncluded = ownReactionsIterated;
                        reactionItr->deliveryState = reaction.deliveryState;
                    }
                } else {
                    if (reactionItr == displayedMessageReactions.end()) {
                        displayedMessageReactions.append({reaction.emoji, 1, ownReactionsIterated, {}});
                    } else {
                        reactionItr->count++;
                    }
                }
            }
        }

        std::sort(displayedMessageReactions.begin(), displayedMessageReactions.end());

        return QVariant::fromValue(displayedMessageReactions);
    }
    case DetailedReactions: {
        if (msg.isGroupChatMessage()) {
            QList<DetailedMessageReaction> detailedMessageReactions;

            const auto &reactionSenders = msg.reactionSenders;
            for (auto itr = reactionSenders.begin(); itr != reactionSenders.end(); ++itr) {
                // Skip own reactions.
                if (itr.key() != ChatController::instance()->accountJid()) {
                    QStringList emojis;

                    for (const auto &reaction : std::as_const(itr->reactions)) {
                        emojis.append(reaction.emoji);
                    }

                    std::sort(emojis.begin(), emojis.end());

                    detailedMessageReactions.append({itr.key(), {}, {}, emojis});
                }
            }

            return QVariant::fromValue(detailedMessageReactions);
        }

        return {};
    }
    case OwnReactionsFailed: {
        const auto &ownReactions = msg.reactionSenders.value(ChatController::instance()->accountJid()).reactions;
        return std::any_of(ownReactions.cbegin(), ownReactions.cend(), [](const MessageReaction &reaction) {
            switch (reaction.deliveryState) {
            case MessageReactionDeliveryState::ErrorOnAddition:
            case MessageReactionDeliveryState::ErrorOnRemovalAfterSent:
            case MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered:
                return true;
            default:
                return false;
            }
        });
    }
    case GroupChatInvitationJid: {
        const auto groupChatInvitation = msg.groupChatInvitation;
        return groupChatInvitation ? groupChatInvitation->groupChatJid : QString();
    }
    case GeoCoordinate:
        return QVariant::fromValue(msg.geoCoordinate());
    case ErrorText:
        return msg.errorText;
    }

    return {};
}

void MessageModel::fetchMore(const QModelIndex &)
{
    if (!m_fetchedAllFromDb) {
        if (m_messages.isEmpty()) {
            // If there are unread messages, all messages until the first unread message are
            // fetched.
            // Otherwise, the messages are fetched by their regular limit.
            if (ChatController::instance()->rosterItem().unreadMessages > 0) {
                const auto lastReadContactMessageId = ChatController::instance()->rosterItem().lastReadContactMessageId;

                // lastReadContactMessageId can be empty if there is no contact message stored or
                // the oldest stored contact message is marked as first unread.
                if (lastReadContactMessageId.isEmpty()) {
                    await(MessageDb::instance()->fetchMessagesUntilFirstContactMessage(AccountManager::instance()->jid(),
                                                                                       ChatController::instance()->chatJid(),
                                                                                       0),
                          this,
                          [this](QList<Message> &&messages) {
                              handleMessagesFetched(messages);
                          });
                } else {
                    await(MessageDb::instance()->fetchMessagesUntilId(AccountManager::instance()->jid(),
                                                                      ChatController::instance()->chatJid(),
                                                                      0,
                                                                      lastReadContactMessageId),
                          this,
                          [this](MessageDb::MessageResult &&result) {
                              handleMessagesFetched(result.messages);
                          });
                }
            } else {
                await(MessageDb::instance()->fetchMessages(AccountManager::instance()->jid(), ChatController::instance()->chatJid(), 0),
                      this,
                      [this](QList<Message> &&messages) {
                          handleMessagesFetched(messages);
                      });
            }
        } else {
            await(MessageDb::instance()->fetchMessages(AccountManager::instance()->jid(), ChatController::instance()->chatJid(), m_messages.size()),
                  this,
                  [this](QList<Message> &&messages) {
                      handleMessagesFetched(messages);
                  });
        }
    } else if (!m_fetchedAllFromMam) {
        // Skip unneeded steps when 'canFetchMore()' has not been called before calling
        // 'fetchMore()'.
        if (!m_mamLoading) {
            setMamLoading(true);

            const auto chatJid = ChatController::instance()->chatJid();
            const auto isGroupChat = ChatController::instance()->rosterItem().isGroupChat();

            if (m_messages.isEmpty()) {
                await(MessageController::instance()->retrieveBacklogMessages(chatJid, isGroupChat), this, [this](bool complete) {
                    handleMamBacklogRetrieved(complete);
                });
            } else {
                await(MessageController::instance()->retrieveBacklogMessages(chatJid, isGroupChat, m_messages.constLast().stanzaId),
                      this,
                      [this](bool complete) {
                          handleMamBacklogRetrieved(complete);
                      });
            }
        }
    }

    // At this point, all messages are fetched from the DB and via MAM.
}

bool MessageModel::canFetchMore(const QModelIndex &) const
{
    return !m_fetchedAllFromDb || (!m_fetchedAllFromMam && !m_mamLoading);
}

void MessageModel::handleMessageRead(int readMessageIndex)
{
    // Check the index validity.
    if (readMessageIndex < 0 || readMessageIndex >= m_messages.size()) {
        return;
    }

    const auto &lastReadContactMessageId = ChatController::instance()->rosterItem().lastReadContactMessageId;

    // Skip messages that are read but older than the last read message.
    for (int i = 0; i != m_messages.size(); ++i) {
        if (m_messages.at(i).id == lastReadContactMessageId && i <= readMessageIndex) {
            return;
        }
    }

    Message readContactMessage;
    int readContactMessageIndex;
    const auto readMessage = m_messages.at(readMessageIndex);

    // Search for the last read contact message if it is at the top of the chat page list view but
    // readMessageIndex is of an own message.
    if (readMessage.isOwn) {
        auto isContactMessageRead = false;

        for (int i = readMessageIndex + 1; i != m_messages.size(); ++i) {
            if (const auto &message = m_messages.at(i); !message.isOwn) {
                readContactMessageIndex = i;
                readContactMessage = message;
                isContactMessageRead = true;
                break;
            }
        }

        // Skip the remaining processing if no contact message is read.
        if (!isContactMessageRead) {
            return;
        }
    } else {
        readContactMessageIndex = readMessageIndex;
        readContactMessage = readMessage;
    }

    const auto readMessageId = readContactMessage.id;
    const auto isApplicationActive = QGuiApplication::applicationState() == Qt::ApplicationActive;

    if (lastReadContactMessageId != readMessageId && isApplicationActive) {
        Notifications::instance()->closeMessageNotification(ChatController::instance()->accountJid(), ChatController::instance()->chatJid());

        bool readMarkerPending = true;
        if (Enums::ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
            if (ChatController::instance()->rosterItem().readMarkerSendingEnabled) {
                MessageController::instance()->sendReadMarker(ChatController::instance()->chatJid(), readMessageId);
            }

            readMarkerPending = false;
        }

        RosterDb::instance()->updateItem(ChatController::instance()->chatJid(), [=, this](RosterItem &item) {
            item.lastReadContactMessageId = readMessageId;
            item.readMarkerPending = readMarkerPending;

            // If the read message is the latest one, lastReadContactMessageId is empty or the read
            // message is an own message, reset the counter for unread messages.
            // lastReadContactMessageId can be empty if there is no contact message stored or the
            // oldest stored contact message is marked as first unread.
            // Otherwise, decrease it by the number of contact messages between the read contact
            // message and the last read contact message.
            if (readContactMessageIndex == 0 || lastReadContactMessageId.isEmpty() || readMessage.isOwn) {
                item.unreadMessages = 0;
            } else {
                int readMessageCount = 1;
                for (int i = readContactMessageIndex + 1; i < m_messages.size(); ++i) {
                    if (const auto &message = m_messages.at(i); message.id == lastReadContactMessageId) {
                        break;
                    } else if (!message.isOwn) {
                        ++readMessageCount;
                    }
                }

                item.unreadMessages = item.unreadMessages - readMessageCount;
            }
        });
    }
}

int MessageModel::firstUnreadContactMessageIndex() const
{
    return m_firstUnreadContactMessageIndex;
}

void MessageModel::markMessageAsFirstUnread(int index)
{
    int unreadMessageCount = 1;
    QString lastReadContactMessageId;

    // Determine the count of unread contact messages before the marked one.
    for (int i = 0; i != index; ++i) {
        if (!m_messages.at(i).isOwn) {
            ++unreadMessageCount;
        }
    }

    // Search for the first contact message after the marked one.
    if (index < m_messages.size() - 1) {
        for (int i = index + 1; i != m_messages.size(); ++i) {
            const auto &message = m_messages.at(i);
            if (!message.isOwn) {
                lastReadContactMessageId = message.id;
                break;
            }
        }
    }

    // Find the last read contact message in the database in order to update the last read contact
    // message ID.
    // That is needed if a message is marked as the first unread message while the previous message
    // of the contact is not fetched from the database and thus not in m_messages.
    if (lastReadContactMessageId.isEmpty()) {
        auto future =
            MessageDb::instance()->firstContactMessageId(ChatController::instance()->accountJid(), ChatController::instance()->chatJid(), unreadMessageCount);
        await(future, this, [=, currentChatJid = ChatController::instance()->chatJid()](QString firstContactMessageId) {
            RosterDb::instance()->updateItem(currentChatJid, [=](RosterItem &item) {
                item.unreadMessages = unreadMessageCount;
                item.lastReadContactMessageId = firstContactMessageId;
            });
        });
    } else {
        RosterDb::instance()->updateItem(ChatController::instance()->chatJid(), [=](RosterItem &item) {
            item.unreadMessages = unreadMessageCount;
            item.lastReadContactMessageId = lastReadContactMessageId;
        });
    }
}

void MessageModel::addMessageReaction(const QString &messageId, const QString &emoji)
{
    const auto itr = std::find_if(m_messages.cbegin(), m_messages.cend(), [&](const Message &message) {
        return message.relevantId() == messageId;
    });

    if (itr != m_messages.cend()) {
        const auto rosterItem = ChatController::instance()->rosterItem();
        const auto senderId = ChatController::instance()->accountJid();
        const auto reactions = itr->reactionSenders.value(senderId).reactions;

        if (undoMessageReactionRemoval(messageId, senderId, emoji, reactions)) {
            return;
        }

        auto addReaction = [messageId, senderId, emoji](MessageReactionDeliveryState::Enum deliveryState) -> QFuture<void> {
            return MessageDb::instance()->updateMessage(messageId, [senderId, emoji, deliveryState](Message &message) {
                auto &reactionSender = message.reactionSenders[senderId];
                reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();

                MessageReaction reaction;
                reaction.emoji = emoji;
                reaction.deliveryState = deliveryState;

                auto &reactions = reactionSender.reactions;
                reactions.append(reaction);
            });
        };

        auto future = addReaction(MessageReactionDeliveryState::PendingAddition);
        await(future, this, [=, this, chatJid = ChatController::instance()->chatJid()]() {
            if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
                QList<QString> emojis;

                for (const auto &reaction : reactions) {
                    switch (reaction.deliveryState) {
                    case MessageReactionDeliveryState::PendingAddition:
                    case MessageReactionDeliveryState::ErrorOnAddition:
                    case MessageReactionDeliveryState::Sent:
                    case MessageReactionDeliveryState::Delivered:
                        emojis.append(reaction.emoji);
                        break;
                    default:
                        break;
                    }
                }

                emojis.append(emoji);

                await(MessageController::instance()->sendMessageReaction(chatJid, messageId, ChatController::instance()->rosterItem().isGroupChat(), emojis),
                      this,
                      [addReaction, messageId, senderId](QXmpp::SendResult &&result) {
                          if (const auto error = std::get_if<QXmppError>(&result)) {
                              Q_EMIT Kaidan::instance() -> passiveNotificationRequested(tr("Reaction could not be sent: %1").arg(error->description));
                              addReaction(MessageReactionDeliveryState::ErrorOnAddition);
                          } else {
                              MessageController::instance()->updateMessageReactionsAfterSending(messageId, senderId);
                          }
                      });
            }
        });
    }
}

void MessageModel::removeMessageReaction(const QString &messageId, const QString &emoji)
{
    const auto itr = std::find_if(m_messages.cbegin(), m_messages.cend(), [&](const Message &message) {
        return message.relevantId() == messageId;
    });

    if (itr != m_messages.cend()) {
        const auto rosterItem = ChatController::instance()->rosterItem();
        const auto senderId = ChatController::instance()->accountJid();
        const auto &reactions = itr->reactionSenders.value(senderId).reactions;

        if (undoMessageReactionAddition(messageId, senderId, emoji, reactions)) {
            return;
        }

        auto future = MessageDb::instance()->updateMessage(messageId, [senderId, emoji](Message &message) {
            auto &reactionSenders = message.reactionSenders;
            auto &reactions = reactionSenders[senderId].reactions;

            const auto itr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
                return reaction.emoji == emoji;
            });

            switch (auto &deliveryState = itr->deliveryState) {
            case MessageReactionDeliveryState::Sent:
                deliveryState = MessageReactionDeliveryState::PendingRemovalAfterSent;
                break;
            case MessageReactionDeliveryState::Delivered:
                deliveryState = MessageReactionDeliveryState::PendingRemovalAfterDelivered;
                break;
            default:
                break;
            }
        });

        await(future, this, [=, this, chatJid = ChatController::instance()->chatJid()]() {
            if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
                const auto &reactionSenders = itr->reactionSenders;
                const auto &reactions = reactionSenders[senderId].reactions;
                QList<QString> emojis;

                for (const auto &reaction : reactions) {
                    const auto &storedEmoji = reaction.emoji;
                    const auto deliveryState = reaction.deliveryState;

                    switch (deliveryState) {
                    case MessageReactionDeliveryState::PendingAddition:
                    case MessageReactionDeliveryState::ErrorOnAddition:
                    case MessageReactionDeliveryState::Sent:
                    case MessageReactionDeliveryState::Delivered:
                        emojis.append(storedEmoji);
                        break;
                    default:
                        break;
                    }
                }

                await(MessageController::instance()->sendMessageReaction(chatJid, messageId, ChatController::instance()->rosterItem().isGroupChat(), emojis),
                      this,
                      [messageId, senderId, emoji](QXmpp::SendResult &&result) {
                          if (const auto error = std::get_if<QXmppError>(&result)) {
                              Q_EMIT Kaidan::instance() -> passiveNotificationRequested(tr("Reaction could not be sent: %1").arg(error->description));

                              MessageDb::instance()->updateMessage(messageId, [senderId, emoji](Message &message) {
                                  auto &reactions = message.reactionSenders[senderId].reactions;

                                  const auto itr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
                                      return reaction.emoji == emoji;
                                  });

                                  switch (auto &deliveryState = itr->deliveryState) {
                                  case MessageReactionDeliveryState::Sent:
                                      deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterSent;
                                      break;
                                  case MessageReactionDeliveryState::Delivered:
                                      deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered;
                                      break;
                                  default:
                                      break;
                                  }
                              });
                          } else {
                              MessageController::instance()->updateMessageReactionsAfterSending(messageId, senderId);
                          }
                      });
            }
        });
    }
}

void MessageModel::resendMessageReactions(const QString &messageId)
{
    const auto itr = std::find_if(m_messages.cbegin(), m_messages.cend(), [&](const Message &message) {
        return message.relevantId() == messageId;
    });

    if (itr != m_messages.cend()) {
        const auto rosterItem = ChatController::instance()->rosterItem();
        const auto senderId = ChatController::instance()->accountJid();

        MessageDb::instance()->updateMessage(messageId, [senderId](Message &message) {
            auto &reactionSender = message.reactionSenders[senderId];
            reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();

            for (auto &reaction : reactionSender.reactions) {
                switch (auto &deliveryState = reaction.deliveryState) {
                case MessageReactionDeliveryState::ErrorOnAddition:
                    deliveryState = MessageReactionDeliveryState::PendingAddition;
                    break;
                case MessageReactionDeliveryState::ErrorOnRemovalAfterSent:
                    deliveryState = MessageReactionDeliveryState::PendingRemovalAfterSent;
                    break;
                case MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered:
                    deliveryState = MessageReactionDeliveryState::PendingRemovalAfterDelivered;
                    break;
                default:
                    break;
                }
            }
        });

        if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
            QList<QString> emojis;

            for (const auto &reaction : itr->reactionSenders.value(senderId).reactions) {
                if (const auto deliveryState = reaction.deliveryState; deliveryState != MessageReactionDeliveryState::PendingRemovalAfterSent
                    && deliveryState != MessageReactionDeliveryState::PendingRemovalAfterDelivered
                    && deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterSent
                    && deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered) {
                    emojis.append(reaction.emoji);
                }
            }

            await(MessageController::instance()->sendMessageReaction(ChatController::instance()->chatJid(),
                                                                     messageId,
                                                                     ChatController::instance()->rosterItem().isGroupChat(),
                                                                     emojis),
                  this,
                  [messageId, senderId](QXmpp::SendResult &&result) {
                      if (const auto error = std::get_if<QXmppError>(&result)) {
                          Q_EMIT Kaidan::instance() -> passiveNotificationRequested(tr("Reactions could not be sent: %1").arg(error->description));

                          MessageDb::instance()->updateMessage(messageId, [senderId](Message &message) {
                              auto &reactionSender = message.reactionSenders[senderId];
                              reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();

                              for (auto &reaction : reactionSender.reactions) {
                                  switch (auto &deliveryState = reaction.deliveryState) {
                                  case MessageReactionDeliveryState::PendingAddition:
                                      deliveryState = MessageReactionDeliveryState::ErrorOnAddition;
                                      break;
                                  case MessageReactionDeliveryState::PendingRemovalAfterSent:
                                      deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterSent;
                                      break;
                                  case MessageReactionDeliveryState::PendingRemovalAfterDelivered:
                                      deliveryState = MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered;
                                      break;
                                  default:
                                      break;
                                  }
                              }
                          });
                      } else {
                          MessageController::instance()->updateMessageReactionsAfterSending(messageId, senderId);
                      }
                  });
        }
    }
}

bool MessageModel::canCorrectMessage(int index) const
{
    // check index validity
    if (index < 0 || index >= m_messages.size())
        return false;

    // message needs to be sent by us and needs to be no error message
    const auto &msg = m_messages.at(index);
    if (!msg.isOwn || msg.deliveryState == Enums::DeliveryState::Error)
        return false;

    // check time limit
    const auto timeThreshold = QDateTime::currentDateTimeUtc().addDays(-MAX_CORRECTION_MESSAGE_DAYS_DEPTH);
    if (msg.timestamp < timeThreshold)
        return false;

    // check messages count limit
    for (int i = 0, count = 0; i < index; i++) {
        if (m_messages.at(i).isOwn && ++count == MAX_CORRECTION_MESSAGE_COUNT_DEPTH)
            return false;
    }

    return true;
}

void MessageModel::removeMessage(const QString &messageId)
{
    const auto hasCorrectId = [&messageId](const Message &message) {
        return message.relevantId() == messageId;
    };

    const auto itr = std::find_if(m_messages.cbegin(), m_messages.cend(), hasCorrectId);

    // Update the roster item of the current chat.
    if (itr != m_messages.cend()) {
        int readMessageIndex = std::distance(m_messages.cbegin(), itr);

        const QString &lastReadContactMessageId = ChatController::instance()->rosterItem().lastReadContactMessageId;
        const QString &lastReadOwnMessageId = ChatController::instance()->rosterItem().lastReadOwnMessageId;

        if (lastReadContactMessageId == messageId || lastReadOwnMessageId == messageId) {
            handleMessageRead(readMessageIndex);

            // Get the previous message ID if possible.
            const int newLastReadMessageIndex = readMessageIndex + 1;
            const bool isNewLastReadMessageIdValid = m_messages.size() >= newLastReadMessageIndex;

            const QString newLastReadMessageId{
                isNewLastReadMessageIdValid && newLastReadMessageIndex < m_messages.size() ? m_messages.at(newLastReadMessageIndex).id : QString()};

            if (newLastReadMessageId.isEmpty()) {
                RosterDb::instance()->updateItem(ChatController::instance()->chatJid(), [=](RosterItem &item) {
                    item.lastReadContactMessageId.clear();
                    item.lastReadOwnMessageId.clear();
                    item.lastMessage.clear();
                    item.lastMessageGroupChatSenderName.clear();
                    item.unreadMessages = 0;
                });
            } else {
                RosterDb::instance()->updateItem(ChatController::instance()->chatJid(), [=](RosterItem &item) {
                    if (itr->isOwn) {
                        item.lastReadOwnMessageId = newLastReadMessageId;
                    } else {
                        item.lastReadContactMessageId = newLastReadMessageId;
                    }
                });
            }
        }

        // Remove the message from the database and model.

        MessageDb::instance()->removeMessage(itr->accountJid, itr->chatJid, messageId);
        updateLastReadOwnMessageId();

        QModelIndex index = createIndex(readMessageIndex, 0);

        beginRemoveRows(QModelIndex(), readMessageIndex, readMessageIndex);
        m_messages.removeAt(readMessageIndex);
        endRemoveRows();

        Q_EMIT dataChanged(index, index);
    }
}

void MessageModel::removeAllMessages()
{
    if (!m_messages.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_messages.clear();
        endRemoveRows();
    }

    m_fetchedAllFromDb = false;
    m_fetchedAllFromMam = false;
    setMamLoading(false);
}

int MessageModel::searchMessageById(const QString &messageId)
{
    int i = 0;

    for (; i < m_messages.size(); i++) {
        if (m_messages.at(i).relevantId() == messageId) {
            return i;
        }
    }

    await(MessageDb::instance()->fetchMessagesUntilId(AccountManager::instance()->jid(), ChatController::instance()->chatJid(), i, messageId, 0),
          this,
          [this](MessageDb::MessageResult &&result) {
              if (const auto messages = result.messages; !messages.isEmpty()) {
                  handleMessagesFetched(messages);
              }

              Q_EMIT messageSearchByIdInDbFinished(result.queryIndex);
          });

    return -1;
}

int MessageModel::searchForMessageFromNewToOld(const QString &searchString, int startIndex)
{
    int foundIndex = startIndex;

    if (foundIndex < m_messages.size()) {
        for (; foundIndex < m_messages.size(); foundIndex++) {
            if (foundIndex > -1 && m_messages.at(foundIndex).body.contains(searchString, Qt::CaseInsensitive)) {
                return foundIndex;
            }
        }

        await(MessageDb::instance()->fetchMessagesUntilQueryString(AccountManager::instance()->jid(),
                                                                   ChatController::instance()->chatJid(),
                                                                   foundIndex,
                                                                   searchString),
              this,
              [this](MessageDb::MessageResult &&result) {
                  handleMessagesFetched(result.messages);
                  Q_EMIT messageSearchFinished(result.queryIndex);
              });
    }

    return -1;
}

int MessageModel::searchForMessageFromOldToNew(const QString &searchString, int startIndex)
{
    int foundIndex = startIndex;

    if (foundIndex >= 0) {
        for (; foundIndex >= 0; foundIndex--) {
            if (m_messages.at(foundIndex).body.contains(searchString, Qt::CaseInsensitive))
                break;
        }
    }

    return foundIndex;
}

bool MessageModel::mamLoading() const
{
    return m_mamLoading;
}

void MessageModel::setMamLoading(bool mamLoading)
{
    if (m_mamLoading != mamLoading) {
        m_mamLoading = mamLoading;
        Q_EMIT mamLoadingChanged();
    }
}

void MessageModel::handleMessagesFetched(const QList<Message> &msgs)
{
    if (msgs.size() < DB_QUERY_LIMIT_MESSAGES)
        m_fetchedAllFromDb = true;

    if (msgs.empty()) {
        // If nothing can be retrieved from the DB, directly try MAM instead.
        if (m_fetchedAllFromDb) {
            fetchMore({});
        } else {
            Q_EMIT messageFetchingFinished();
        }

        return;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount() + msgs.size() - 1);

    for (auto msg : msgs) {
        // Skip messages that were not fetched for the current chat.
        if (msg.accountJid != ChatController::instance()->accountJid() || msg.chatJid != ChatController::instance()->chatJid()) {
            continue;
        }

        processMessage(msg);
        m_messages << msg;
        addVideoThumbnails(msg);
    }

    updateLastReadOwnMessageId();
    updateFirstUnreadContactMessageIndex();

    endInsertRows();

    Q_EMIT messageFetchingFinished();
}

void MessageModel::handleMamBacklogRetrieved(bool complete)
{
    setMamLoading(false);

    if (complete) {
        m_fetchedAllFromMam = true;
    }

    if (ChatController::instance()->rosterItem().lastReadContactMessageId.isEmpty()) {
        for (const auto &message : std::as_const(m_messages)) {
            if (!message.isOwn) {
                RosterDb::instance()->updateItem(ChatController::instance()->chatJid(), [=, messageId = message.id](RosterItem &item) {
                    item.lastReadContactMessageId = messageId;
                });
                break;
            }
        }
    }

    Q_EMIT messageFetchingFinished();
}

void MessageModel::handleMessage(Message msg, MessageOrigin origin)
{
    processMessage(msg);

    showMessageNotification(msg, origin);

    if (msg.accountJid == ChatController::instance()->accountJid() && msg.chatJid == ChatController::instance()->chatJid()) {
        addMessage(std::move(msg));
    }
}

void MessageModel::handleMessageUpdated(Message message)
{
    for (int i = 0; i < m_messages.size(); i++) {
        const auto oldMessage = m_messages.at(i);
        const auto oldId = oldMessage.id;
        const auto oldReplaceId = oldMessage.replaceId;

        // The updated message can be either a normal message, a first message correction, an own
        // reflected group chat message or a subsequent messsage correction.
        if ((!oldId.isEmpty() && (oldId == message.id || oldId == message.replaceId || oldId == message.originId))
            || (!oldReplaceId.isEmpty() && oldReplaceId == message.replaceId)) {
            const auto oldMessageTimestamp = m_messages.at(i).timestamp;

            // Keep the variables that are only stored in memory.
            // Otherwise, they would be removed by the updated message.
            message.groupChatSenderJid = oldMessage.groupChatSenderJid;
            message.groupChatSenderName = oldMessage.groupChatSenderName;
            message.preciseTrustLevel = oldMessage.preciseTrustLevel;

            beginRemoveRows(QModelIndex(), i, i);
            m_messages.removeAt(i);
            endRemoveRows();

            // Insert the message at its original position if the date is unchanged.
            // Otherwise, move it to its new position.
            if (!m_messages.isEmpty() && (message.timestamp == oldMessageTimestamp)) {
                insertMessage(i, message);
            } else {
                addMessage(message);
            }

            showMessageNotification(message, MessageOrigin::Stream);

            break;
        }
    }
}

void MessageModel::handleDevicesChanged(const QString &accountJid, QList<QString> jids)
{
    for (auto itr = m_messages.begin(); itr != m_messages.end(); ++itr) {
        if (const auto senderJid = itr->senderJid(); itr->accountJid == accountJid && jids.contains(senderJid)) {
            // Do not retrieve the trust level of this device and for unencrypted messages.
            if (const auto senderKey = itr->senderKey; senderKey.isEmpty()) {
                continue;
            }

            TrustDb::instance()->trustLevel(XMLNS_OMEMO_2, senderJid, itr->senderKey).then(this, [this, itr](QXmpp::TrustLevel trustLevel) {
                itr->preciseTrustLevel = trustLevel;

                const auto i = std::distance(m_messages.begin(), itr);
                const auto modelIndex = index(i);
                Q_EMIT dataChanged(modelIndex, modelIndex, {TrustLevel});
            });
        }
    }
}

void MessageModel::addMessage(const Message &msg)
{
    // index where to add the new message
    int i = 0;
    for (const auto &message : std::as_const(m_messages)) {
        if (msg.timestamp > message.timestamp) {
            insertMessage(i, msg);
            return;
        }
        i++;
    }
    // add message to the end of the list
    insertMessage(i, msg);
}

void MessageModel::insertMessage(int idx, const Message &msg)
{
    beginInsertRows(QModelIndex(), idx, idx);
    m_messages.insert(idx, msg);
    endInsertRows();

    updateLastReadOwnMessageId();
    addVideoThumbnails(msg);
}

void MessageModel::removeMessages(const QString &accountJid, const QString &chatJid)
{
    if (accountJid == ChatController::instance()->accountJid() && chatJid == ChatController::instance()->chatJid()) {
        removeAllMessages();
    }
}

void MessageModel::processMessage(Message &message)
{
    if (message.body.size() > MESSAGE_MAX_CHARS) {
        auto body = message.body;
        body.truncate(MESSAGE_MAX_CHARS);
        message.body = body;
    }
}

void MessageModel::addVideoThumbnails(const Message &message)
{
    auto &files = message.files;
    std::for_each(files.begin(), files.end(), [this, messageId = message.id](const File &file) {
        if (const auto localFileUrl = file.localFileUrl(); file.type() == MessageType::MessageVideo && file.localFileUrl().isValid()) {
            await(MediaUtils::generateThumbnail(localFileUrl, file.mimeTypeName(), VIDEO_THUMBNAIL_EDGE_PIXEL_COUNT),
                  this,
                  [this, fileId = file.fileId(), messageId](const QByteArray &thumbnail) mutable {
                      auto itr = std::find_if(m_messages.begin(), m_messages.end(), [messageId](const Message &message) {
                          return message.relevantId() == messageId;
                      });

                      if (itr != m_messages.cend()) {
                          auto &files = itr->files;

                          const auto fileItr = std::find_if(files.begin(), files.end(), [fileId, messageId](const File &queriedFile) {
                              return queriedFile.fileId() == fileId;
                          });

                          if (fileItr != files.cend()) {
                              fileItr->thumbnail = thumbnail;
                              const auto modelIndex = index(std::distance(m_messages.begin(), itr));
                              Q_EMIT dataChanged(modelIndex, modelIndex, {Files});
                          }
                      }
                  });
        }
    });
}

void MessageModel::updateLastReadOwnMessageId()
{
    const auto formerLastReadOwnMessageId = m_lastReadOwnMessageId;
    m_lastReadOwnMessageId = ChatController::instance()->rosterItem().lastReadOwnMessageId;
    emitMessagesUpdated({formerLastReadOwnMessageId, m_lastReadOwnMessageId}, IsLastReadOwnMessage);
}

void MessageModel::updateFirstUnreadContactMessageIndex()
{
    int lastReadContactMessageIndex = -1;

    for (auto i = 0; i < m_messages.size(); ++i) {
        const auto &message = m_messages.at(i);
        const auto &messageId = message.id;
        const auto lastReadContactMessageId = ChatController::instance()->rosterItem().lastReadContactMessageId;

        // lastReadContactMessageId can be empty if there is no contact message stored or the oldest
        // stored contact message is marked as first unread.
        if (!message.isOwn && (messageId == lastReadContactMessageId || lastReadContactMessageId.isEmpty())) {
            lastReadContactMessageIndex = i;
            break;
        }
    }

    if (lastReadContactMessageIndex > 0) {
        for (auto i = lastReadContactMessageIndex - 1; i >= 0; --i) {
            if (!m_messages.at(i).isOwn) {
                m_firstUnreadContactMessageIndex = i;
                return;
            }
        }
    }

    m_firstUnreadContactMessageIndex = -1;
}

void MessageModel::emitMessagesUpdated(const QVector<QString> &messageIds, MessageRoles role)
{
    const int messageIdCount = messageIds.size();
    int foundMessageCount = 0;

    for (int i = 0; i != m_messages.size(); ++i) {
        const auto &messageId = m_messages.at(i).id;

        if (messageIds.contains(messageId)) {
            const auto modelIndex = index(i);
            Q_EMIT dataChanged(modelIndex, modelIndex, {role});

            if (++foundMessageCount == messageIdCount) {
                break;
            }
        }
    }
}

void MessageModel::showMessageNotification(const Message &message, MessageOrigin origin)
{
    // Send a notification in the following cases:
    // * The message was not sent by the user from another resource and
    //   received via Message Carbons.
    // * Notifications are allowed according to the set rule.
    // * The corresponding chat is not opened while the application window
    //   is active.

    switch (origin) {
    case MessageOrigin::UserInput:
    case MessageOrigin::MamInitial:
    case MessageOrigin::MamBacklog:
        // no notifications
        return;
    case MessageOrigin::Stream:
    case MessageOrigin::MamCatchUp:
        break;
    }

    if (!message.isOwn) {
        const auto accountJid = AccountManager::instance()->jid();
        const auto chatJid = message.chatJid;

        const auto rosterItem = RosterModel::instance()->findItem(chatJid).value_or(RosterItem{});

        // auto notificationsAllowedByRule = true;

        auto sendNotification = [this, accountJid, chatJid, message]() {
            const auto chatActive =
                ChatController::instance()->isChatCurrentChat(accountJid, chatJid) && QGuiApplication::applicationState() == Qt::ApplicationActive;
            const auto previewText = message.previewText();
            const auto notificationBody =
                message.isGroupChatMessage() ? determineGroupChatSenderName(message) + QStringLiteral(": ") + previewText : previewText;

            if (!chatActive) {
                Notifications::instance()->sendMessageNotification(accountJid, chatJid, message.id, notificationBody);
            }
        };

        auto sendNotificationOnMention = [this, accountJid, chatJid, rosterItem, message, sendNotification]() {
            await(GroupChatUserDb::instance()->user(accountJid, chatJid, rosterItem.groupChatParticipantId),
                  this,
                  [body = message.body, sendNotification](const std::optional<GroupChatUser> user) {
                      if (user && (body.contains(user->id) || body.contains(user->jid) || body.contains(user->name))) {
                          sendNotification();
                      }
                  });
        };

        if (const auto notificationRule = rosterItem.notificationRule; notificationRule == RosterItem::NotificationRule::Account) {
            if (message.isGroupChatMessage()) {
                switch (AccountManager::instance()->account().groupChatNotificationRule) {
                case Account::GroupChatNotificationRule::Mentioned:
                    sendNotificationOnMention();
                    break;
                case Account::GroupChatNotificationRule::Always:
                    sendNotification();
                    break;
                default:
                    break;
                }
            } else {
                switch (AccountManager::instance()->account().contactNotificationRule) {
                case Account::ContactNotificationRule::PresenceOnly:
                    if (rosterItem.isReceivingPresence()) {
                        sendNotification();
                    }
                    break;
                case Account::ContactNotificationRule::Always:
                    sendNotification();
                    break;
                default:
                    break;
                }
            }
        } else {
            switch (notificationRule) {
            case RosterItem::NotificationRule::PresenceOnly:
                if (rosterItem.isReceivingPresence()) {
                    sendNotification();
                }
                break;
            case RosterItem::NotificationRule::Mentioned:
                sendNotificationOnMention();
                break;
            case RosterItem::NotificationRule::Always:
                sendNotification();
                break;
            default:
                break;
            }
        }
    }
}

bool MessageModel::undoMessageReactionRemoval(const QString &messageId, const QString &senderJid, const QString &emoji, const QList<MessageReaction> &reactions)
{
    const auto reactionItr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
        return reaction.emoji == emoji;
    });

    if (reactionItr != reactions.end()) {
        MessageDb::instance()->updateMessage(messageId, [senderJid, emoji](Message &message) {
            auto &reactions = message.reactionSenders[senderJid].reactions;

            const auto itr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
                return reaction.emoji == emoji;
            });

            switch (auto &deliveryState = itr->deliveryState) {
            case MessageReactionDeliveryState::PendingRemovalAfterSent:
            case MessageReactionDeliveryState::ErrorOnRemovalAfterSent:
                deliveryState = MessageReactionDeliveryState::Sent;
                break;
            case MessageReactionDeliveryState::PendingRemovalAfterDelivered:
            case MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered:
                deliveryState = MessageReactionDeliveryState::Delivered;
                break;
            default:
                break;
            }
        });

        return true;
    }

    return false;
}

bool MessageModel::undoMessageReactionAddition(const QString &messageId,
                                               const QString &senderJid,
                                               const QString &emoji,
                                               const QList<MessageReaction> &reactions)
{
    const auto reactionItr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
        return reaction.emoji == emoji
            && (reaction.deliveryState == MessageReactionDeliveryState::PendingAddition
                || reaction.deliveryState == MessageReactionDeliveryState::ErrorOnAddition);
    });

    if (reactionItr != reactions.end()) {
        MessageDb::instance()->updateMessage(messageId, [senderJid, emoji](Message &message) {
            auto &reactionSenders = message.reactionSenders;
            auto &reactions = reactionSenders[senderJid].reactions;

            const auto itr = std::find_if(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
                return reaction.emoji == emoji;
            });

            switch (itr->deliveryState) {
            case MessageReactionDeliveryState::PendingAddition:
            case MessageReactionDeliveryState::ErrorOnAddition:
                reactions.erase(itr);

                // Remove the reaction sender if it has no reactions anymore.
                if (reactions.isEmpty()) {
                    reactionSenders.remove(senderJid);
                }

                break;
            default:
                break;
            }
        });

        return true;
    }

    return false;
}

QDate MessageModel::searchNextDate(int messageStartIndex) const
{
    const auto startDate = m_messages.at(messageStartIndex).timestamp.toLocalTime().date();

    for (int i = messageStartIndex; i >= 0; i--) {
        if (const auto date = m_messages.at(i).timestamp.toLocalTime().date(); date > startDate) {
            return date;
        }
    }

    return {};
}

QString MessageModel::formatDate(QDate localDate) const
{
    if (localDate.isNull()) {
        // Unset date: Return a default-constructed string.
        return {};
    } else if (const auto elapsedNightCount = localDate.daysTo(QDateTime::currentDateTime().date()); elapsedNightCount == 0) {
        // Today: Return that term.
        return tr("Today");
    } else if (elapsedNightCount == 1) {
        // Yesterday: Return that term.
        return tr("Yesterday");
    } else if (elapsedNightCount <= 7) {
        // Between yesterday and seven days before today: Return the day of the week.
        return QLocale::system().dayName(localDate.dayOfWeek(), QLocale::LongFormat);
    } else {
        // Older than seven days before today: Return the date.
        return QLocale::system().toString(localDate, QLocale::LongFormat);
    }
}

QString MessageModel::determineGroupChatSenderName(const Message &message) const
{
    const auto groupChatSenderJid = message.groupChatSenderJid;

    if (!groupChatSenderJid.isEmpty()) {
        if (const auto rosterItem = RosterModel::instance()->findItem(groupChatSenderJid)) {
            return rosterItem->displayName();
        }
    }

    return message.groupChatSenderName;
}

QString MessageModel::determineReplyToName(const Message::Reply &reply) const
{
    if (const auto replyToJid = reply.toJid; !replyToJid.isEmpty()) {
        if (const auto rosterItem = RosterModel::instance()->findItem(replyToJid)) {
            return rosterItem->displayName();
        }
    }

    return reply.toGroupChatParticipantName;
}

#include "moc_MessageModel.cpp"
