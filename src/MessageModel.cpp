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
#include "Account.h"
#include "AtmController.h"
#include "ChatController.h"
#include "EncryptionController.h"
#include "Globals.h"
#include "KaidanCoreLog.h"
#include "MainController.h"
#include "MediaUtils.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "NotificationController.h"
#include "RosterDb.h"
#include "RosterModel.h"

auto DisplayedMessageReaction::operator<=>(const DisplayedMessageReaction &other) const
{
    return emoji <=> other.emoji;
}

bool DetailedMessageReaction::operator<(const DetailedMessageReaction &other) const
{
    return senderName < other.senderName;
}

MessageModel::MessageModel(AccountSettings *accountSettings,
                           Connection *connection,
                           AtmController *atmController,
                           ChatController *chatController,
                           EncryptionController *encryptionController,
                           MessageController *messageController,
                           NotificationController *notificationController,
                           QObject *parent)
    : QAbstractListModel(parent)
    , m_accountSettings(accountSettings)
    , m_connection(connection)
    , m_atmController(atmController)
    , m_chatController(chatController)
    , m_messageController(messageController)
    , m_notificationController(notificationController)
{
    connect(MessageDb::instance(), &MessageDb::messageAdded, this, &MessageModel::handleMessage);
    connect(MessageDb::instance(), &MessageDb::messageUpdated, this, &MessageModel::handleMessageUpdated);
    connect(MessageDb::instance(), &MessageDb::messagesRemoved, this, &MessageModel::removeMessages);

    connect(m_chatController, &ChatController::rosterItemChanged, this, [this]() {
        if (m_lastReadOwnMessageId != m_chatController->rosterItem().lastReadOwnMessageId) {
            updateLastReadOwnMessageId();
        }
    });

    connect(encryptionController, &EncryptionController::devicesChanged, this, &MessageModel::handleDevicesChanged);
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
    roles[Marked] = QByteArrayLiteral("marked");
    roles[ErrorText] = QByteArrayLiteral("errorText");
    return roles;
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();

    if (!hasIndex(row, index.column(), index.parent())) {
        qCWarning(KAIDAN_CORE_LOG) << "Could not get data from message model." << index << role;
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
            return msg.groupChatSenderName;
        }

        const auto rosterItem = RosterModel::instance()->item(msg.accountJid, msg.chatJid);

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

        // Return a default-constructed string for a reply to an own message.
        if (msg.isGroupChatMessage() && reply->toGroupChatParticipantId.isEmpty()) {
            return QString();
        }

        return reply->toJid;
    }
    case ReplyToGroupChatParticipantId: {
        if (!msg.isGroupChatMessage()) {
            return QString();
        }

        const auto reply = msg.reply;

        // Return a default-constructed string for no reply or a reply to an own message.
        if (!reply || reply->toGroupChatParticipantId.isEmpty()) {
            return QString();
        }

        return reply->toGroupChatParticipantId;
    }
    case ReplyToName: {
        const auto reply = msg.reply;

        if (!reply) {
            return QString();
        }

        if (msg.isGroupChatMessage()) {
            // Return a default-constructed string for a reply to an own message.
            if (reply->toGroupChatParticipantId.isEmpty()) {
                return QString();
            }

            return determineReplyToName(*reply);
        }

        // Return a default-constructed string for a reply to an own message.
        if (reply->toJid.isEmpty()) {
            return QString();
        }

        const auto rosterItem = RosterModel::instance()->item(msg.accountJid, msg.chatJid);

        // On the first received message from a stranger, a new roster item is added.
        Q_ASSERT(rosterItem);

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
        for (auto itr = reactionSenders.cbegin(); itr != reactionSenders.cend(); ++itr) {
            const auto ownReactionsIterated = itr.key() == m_accountSettings->jid();

            for (const auto &reaction : std::as_const(itr->reactions)) {
                auto reactionItr = std::ranges::find_if(displayedMessageReactions, [=](const DisplayedMessageReaction &displayedMessageReaction) {
                    return displayedMessageReaction.emoji == reaction.emoji;
                });

                if (ownReactionsIterated) {
                    if (reactionItr == displayedMessageReactions.end()) {
                        displayedMessageReactions.append({reaction.emoji, 1, true, reaction.deliveryState});
                    } else {
                        reactionItr->count++;
                        reactionItr->ownReactionIncluded = true;
                        reactionItr->deliveryState = reaction.deliveryState;
                    }
                } else {
                    if (reactionItr == displayedMessageReactions.end()) {
                        displayedMessageReactions.append({reaction.emoji, 1, false, {}});
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
                if (itr.key() != m_accountSettings->jid()) {
                    QStringList emojis;

                    for (const auto &reaction : std::as_const(itr->reactions)) {
                        emojis.append(reaction.emoji);
                    }

                    std::ranges::sort(emojis);

                    detailedMessageReactions.append({itr.key(), {}, {}, emojis});
                }
            }

            return QVariant::fromValue(detailedMessageReactions);
        }

        return {};
    }
    case OwnReactionsFailed: {
        const auto &ownReactions = msg.reactionSenders.value(m_accountSettings->jid()).reactions;
        return std::ranges::any_of(ownReactions, [](const MessageReaction &reaction) {
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
    case Marked:
        return msg.marked;
    case ErrorText:
        return msg.errorText;
    }

    return {};
}

void MessageModel::fetchMore(const QModelIndex &)
{
    if (!m_fetchedAllFromDb) {
        const auto accountJid = m_accountSettings->jid();
        if (m_messages.isEmpty()) {
            // If there are unread messages, all messages until the first unread message are
            // fetched.
            // Otherwise, the messages are fetched by their regular limit.
            if (m_chatController->rosterItem().unreadMessageCount) {
                const auto lastReadContactMessageId = m_chatController->rosterItem().lastReadContactMessageId;

                // lastReadContactMessageId can be empty if there is no contact message stored or
                // the oldest stored contact message is marked as first unread.
                if (lastReadContactMessageId.isEmpty()) {
                    MessageDb::instance()
                        ->fetchMessagesUntilFirstContactMessage(accountJid, m_chatController->jid(), 0)
                        .then(this, [this](QList<Message> &&messages) {
                            handleMessagesFetched(messages);
                        });
                } else {
                    MessageDb::instance()
                        ->fetchMessagesUntilId(accountJid, m_chatController->jid(), 0, lastReadContactMessageId)
                        .then(this, [this](MessageDb::MessageResult &&result) {
                            handleMessagesFetched(result.messages);
                        });
                }
            } else {
                MessageDb::instance()->fetchMessages(accountJid, m_chatController->jid(), 0).then(this, [this](QList<Message> &&messages) {
                    handleMessagesFetched(messages);
                });
            }
        } else {
            MessageDb::instance()->fetchMessages(accountJid, m_chatController->jid(), m_messages.size()).then(this, [this](QList<Message> &&messages) {
                handleMessagesFetched(messages);
            });
        }
    } else if (!m_fetchedAllFromMam && m_connection->state() == Enums::ConnectionState::StateConnected) {
        // Skip unneeded steps when 'canFetchMore()' has not been called before calling
        // 'fetchMore()'.
        if (!m_mamLoading) {
            setMamLoading(true);

            const auto chatJid = m_chatController->jid();
            const auto isGroupChat = m_chatController->rosterItem().isGroupChat();

            if (m_messages.isEmpty()) {
                m_messageController->retrieveBacklogMessages(chatJid, isGroupChat).then([this](bool complete) {
                    handleMamBacklogRetrieved(complete);
                });
            } else {
                m_messageController->retrieveBacklogMessages(chatJid, isGroupChat, m_messages.constLast().stanzaId).then([this](bool complete) {
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

void MessageModel::resendMessage(int index)
{
    if (index < 0 || index >= m_messages.size()) {
        return;
    }

    const auto message = m_messages.at(index);

    MessageDb::instance()->updateMessage(m_accountSettings->jid(), m_chatController->jid(), message.relevantId(), [](Message &message) {
        message.deliveryState = DeliveryState::Pending;
    });

    m_messageController->sendPendingMessage(std::move(message));
}

void MessageModel::handleMessageRead(int readMessageIndex)
{
    // Check the index validity.
    if (readMessageIndex < 0 || readMessageIndex >= m_messages.size()) {
        return;
    }

    const auto &lastReadContactMessageId = m_chatController->rosterItem().lastReadContactMessageId;

    // Skip messages that are read but older than the last read message.
    for (int i = 0; i != m_messages.size(); ++i) {
        if (m_messages.at(i).id == lastReadContactMessageId && i <= readMessageIndex) {
            return;
        }
    }

    Message readContactMessage;
    const auto readMessage = m_messages.at(readMessageIndex);

    // Search for the last read contact message if it is at the top of the chat page list view but
    // readMessageIndex is of an own message.
    if (readMessage.isOwn) {
        auto isContactMessageRead = false;

        for (int i = readMessageIndex + 1; i != m_messages.size(); ++i) {
            if (const auto &message = m_messages.at(i); !message.isOwn) {
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
        readContactMessage = readMessage;
    }

    const auto readMessageId = readContactMessage.id;
    const auto isApplicationActive = QGuiApplication::applicationState() == Qt::ApplicationActive;

    if (lastReadContactMessageId != readMessageId && isApplicationActive) {
        m_notificationController->closeMessageNotification(m_chatController->jid());

        bool readMarkerPending = true;
        if (Enums::ConnectionState(m_connection->state()) == Enums::ConnectionState::StateConnected) {
            if (m_chatController->rosterItem().readMarkerSendingEnabled) {
                m_messageController->sendReadMarker(m_chatController->jid(), readMessageId);
            }

            readMarkerPending = false;
        }

        RosterDb::instance()->updateItem(m_accountSettings->jid(), m_chatController->jid(), [=](RosterItem &item) {
            item.lastReadContactMessageId = readMessageId;
            item.readMarkerPending = readMarkerPending;
        });
    }
}

int MessageModel::firstUnreadContactMessageIndex() const
{
    return m_firstUnreadContactMessageIndex;
}

void MessageModel::setMessageMarked(int index, bool marked)
{
    MessageDb::instance()->updateMessage(m_accountSettings->jid(), m_chatController->jid(), m_messages.at(index).id, [marked](Message &message) {
        message.marked = marked;
    });
}

void MessageModel::addMessageReaction(const QString &messageId, const QString &emoji)
{
    const auto itr = std::ranges::find_if(m_messages, [&](const Message &message) {
        return message.relevantId() == messageId;
    });

    if (itr != m_messages.cend()) {
        const auto rosterItem = m_chatController->rosterItem();
        const auto senderId = m_accountSettings->jid();
        const auto reactions = itr->reactionSenders.value(senderId).reactions;

        if (undoMessageReactionRemoval(messageId, senderId, emoji, reactions)) {
            return;
        }

        auto addReaction = [this, messageId, senderId, emoji](MessageReactionDeliveryState::Enum deliveryState) -> QFuture<void> {
            return MessageDb::instance()->updateMessage(m_chatController->account()->settings()->jid(),
                                                        m_chatController->jid(),
                                                        messageId,
                                                        [senderId, emoji, deliveryState](Message &message) {
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
        future.then(this, [=, this, chatJid = m_chatController->jid()]() {
            if (ConnectionState(m_connection->state()) == Enums::ConnectionState::StateConnected) {
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

                m_messageController
                    ->sendMessageReaction(chatJid,
                                          messageId,
                                          m_chatController->rosterItem().isGroupChat(),
                                          emojis,
                                          m_chatController->activeEncryption(),
                                          m_chatController->groupChatUserJids())
                    .then([this, addReaction, messageId, senderId, emoji](QXmpp::SendResult &&result) {
                        if (const auto error = std::get_if<QXmppError>(&result)) {
                            Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Reaction could not be sent: %1").arg(error->description));

                            MessageDb::instance()->updateMessage(m_chatController->account()->settings()->jid(),
                                                                 m_chatController->jid(),
                                                                 messageId,
                                                                 [senderId, emoji](Message &message) {
                                                                     auto &reactionSender = message.reactionSenders[senderId];
                                                                     reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();
                                                                     auto &reactions = reactionSender.reactions;

                                                                     auto itr = std::ranges::find_if(reactions, [emoji](const MessageReaction &reaction) {
                                                                         return reaction.emoji == emoji;
                                                                     });

                                                                     if (itr != reactions.end()) {
                                                                         itr->deliveryState = MessageReactionDeliveryState::ErrorOnAddition;
                                                                     }
                                                                 });
                        } else {
                            m_messageController->updateMessageReactionsAfterSending(m_chatController->jid(), messageId, senderId);
                        }
                    });
            }
        });
    }
}

void MessageModel::removeMessageReaction(const QString &messageId, const QString &emoji)
{
    const auto itr = std::ranges::find_if(m_messages, [&](const Message &message) {
        return message.relevantId() == messageId;
    });

    if (itr != m_messages.cend()) {
        const auto rosterItem = m_chatController->rosterItem();
        const auto senderId = m_accountSettings->jid();
        const auto &reactions = itr->reactionSenders.value(senderId).reactions;

        if (undoMessageReactionAddition(messageId, senderId, emoji, reactions)) {
            return;
        }

        auto future = MessageDb::instance()->updateMessage(m_chatController->account()->settings()->jid(),
                                                           m_chatController->jid(),
                                                           messageId,
                                                           [senderId, emoji](Message &message) {
                                                               auto &reactionSenders = message.reactionSenders;
                                                               auto &reactions = reactionSenders[senderId].reactions;

                                                               const auto itr = std::ranges::find_if(reactions, [&](const MessageReaction &reaction) {
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

        future.then(this, [this, messageId, senderId, emoji, itr, chatJid = m_chatController->jid()]() {
            if (ConnectionState(m_connection->state()) == Enums::ConnectionState::StateConnected) {
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

                m_messageController
                    ->sendMessageReaction(chatJid,
                                          messageId,
                                          m_chatController->rosterItem().isGroupChat(),
                                          emojis,
                                          m_chatController->activeEncryption(),
                                          m_chatController->groupChatUserJids())
                    .then([this, messageId, senderId, emoji](QXmpp::SendResult &&result) {
                        if (const auto error = std::get_if<QXmppError>(&result)) {
                            Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Reaction could not be sent: %1").arg(error->description));

                            MessageDb::instance()->updateMessage(m_chatController->account()->settings()->jid(),
                                                                 m_chatController->jid(),
                                                                 messageId,
                                                                 [senderId, emoji](Message &message) {
                                                                     auto &reactions = message.reactionSenders[senderId].reactions;

                                                                     const auto itr = std::ranges::find_if(reactions, [&](const MessageReaction &reaction) {
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
                            m_messageController->updateMessageReactionsAfterSending(m_chatController->jid(), messageId, senderId);
                        }
                    });
            }
        });
    }
}

void MessageModel::resendMessageReactions(const QString &messageId)
{
    const auto itr = std::ranges::find_if(m_messages, [&](const Message &message) {
        return message.relevantId() == messageId;
    });

    if (itr != m_messages.cend()) {
        const auto rosterItem = m_chatController->rosterItem();
        const auto senderId = m_accountSettings->jid();

        MessageDb::instance()->updateMessage(m_chatController->account()->settings()->jid(), m_chatController->jid(), messageId, [senderId](Message &message) {
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

        if (ConnectionState(m_connection->state()) == Enums::ConnectionState::StateConnected) {
            QList<QString> emojis;

            for (const auto &reaction : itr->reactionSenders.value(senderId).reactions) {
                if (const auto deliveryState = reaction.deliveryState; deliveryState != MessageReactionDeliveryState::PendingRemovalAfterSent
                    && deliveryState != MessageReactionDeliveryState::PendingRemovalAfterDelivered
                    && deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterSent
                    && deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered) {
                    emojis.append(reaction.emoji);
                }
            }

            m_messageController
                ->sendMessageReaction(m_chatController->jid(),
                                      messageId,
                                      m_chatController->rosterItem().isGroupChat(),
                                      emojis,
                                      m_chatController->activeEncryption(),
                                      m_chatController->groupChatUserJids())
                .then([this, messageId, senderId](QXmpp::SendResult &&result) {
                    if (const auto error = std::get_if<QXmppError>(&result)) {
                        Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Reactions could not be sent: %1").arg(error->description));

                        MessageDb::instance()->updateMessage(m_chatController->account()->settings()->jid(),
                                                             m_chatController->jid(),
                                                             messageId,
                                                             [senderId](Message &message) {
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
                        m_messageController->updateMessageReactionsAfterSending(m_chatController->jid(), messageId, senderId);
                    }
                });
        }
    }
}

int MessageModel::nextCorrectableMessageIndex(int indexOffset) const
{
    for (int i = indexOffset; i < m_messages.size(); i++) {
        if (canCorrectMessage(i)) {
            return i;
        }
    }

    return -1;
}

int MessageModel::previousCorrectableMessageIndex(int indexOffset) const
{
    if (indexOffset >= m_messages.size()) {
        return -1;
    }

    for (int i = indexOffset; i >= 0; i--) {
        if (canCorrectMessage(i)) {
            return i;
        }
    }

    return -1;
}

bool MessageModel::canCorrectMessage(int index) const
{
    // The message must be loaded.
    if (index < 0 || index >= m_messages.size()) {
        return false;
    }

    const auto &message = m_messages.at(index);

    if (!message.isOwn || message.groupChatInvitation || message.deliveryState == Enums::DeliveryState::Error
        || RosterModel::instance()->item(message.accountJid, message.chatJid)->isDeletedGroupChat()) {
        return false;
    }

    // The message must not be too old.
    const auto timeThreshold = QDateTime::currentDateTimeUtc().addDays(-MAX_MESSAGE_CORRECTION_DAYS);
    if (message.timestamp < timeThreshold) {
        return false;
    }

    // There must not be too many more recent messages.
    if (index >= MAX_MESSAGE_CORRECTION_COUNT) {
        return false;
    }

    return true;
}

void MessageModel::deleteFile(int index, const File &file)
{
    MessageDb::instance()->updateMessage(m_accountSettings->jid(), m_chatController->jid(), m_messages.at(index).id, [fileId = file.id](Message &message) {
        auto it = std::ranges::find_if(message.files, [fileId](const auto &file) {
            return file.id == fileId;
        });
        if (it != message.files.end()) {
            it->localFilePath.clear();
        }
    });

    MediaUtils::deleteDownloadedFile(file.localFilePath);
}

void MessageModel::removeMessage(const QString &messageId)
{
    const auto hasCorrectId = [&messageId](const Message &message) {
        return message.relevantId() == messageId;
    };

    const auto itr = std::ranges::find_if(m_messages, hasCorrectId);

    // Update the roster item of the current chat.
    if (itr != m_messages.cend()) {
        int readMessageIndex = std::ranges::distance(m_messages.cbegin(), itr);

        const QString &lastReadContactMessageId = m_chatController->rosterItem().lastReadContactMessageId;
        const QString &lastReadOwnMessageId = m_chatController->rosterItem().lastReadOwnMessageId;

        if (lastReadContactMessageId == messageId || lastReadOwnMessageId == messageId) {
            handleMessageRead(readMessageIndex);

            // Get the previous message ID if possible.
            const int newLastReadMessageIndex = readMessageIndex + 1;
            const bool isNewLastReadMessageIdValid = m_messages.size() >= newLastReadMessageIndex;

            const QString newLastReadMessageId{
                isNewLastReadMessageIdValid && newLastReadMessageIndex < m_messages.size() ? m_messages.at(newLastReadMessageIndex).id : QString()};

            if (newLastReadMessageId.isEmpty()) {
                RosterDb::instance()->updateItem(m_accountSettings->jid(), m_chatController->jid(), [=](RosterItem &item) {
                    item.lastReadContactMessageId.clear();
                    item.lastReadOwnMessageId.clear();
                    item.lastMessage.clear();
                    item.lastMessageGroupChatSenderName.clear();
                });
            } else {
                RosterDb::instance()->updateItem(m_accountSettings->jid(), m_chatController->jid(), [=](RosterItem &item) {
                    if (itr->isOwn) {
                        item.lastReadOwnMessageId = newLastReadMessageId;
                    } else {
                        item.lastReadContactMessageId = newLastReadMessageId;
                    }
                });
            }
        }

        // Remove the message from the database/model and delete included files.

        MessageDb::instance()->removeMessage(itr->accountJid, itr->chatJid, messageId);

        for (auto &file : itr->files) {
            MediaUtils::deleteDownloadedFile(file.localFilePath);
        }

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

    MessageDb::instance()
        ->fetchMessagesUntilId(m_accountSettings->jid(), m_chatController->jid(), i, messageId, 0)
        .then(this, [this](MessageDb::MessageResult &&result) {
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
            if (foundIndex > -1 && m_messages.at(foundIndex).body().contains(searchString, Qt::CaseInsensitive)) {
                return foundIndex;
            }
        }

        MessageDb::instance()
            ->fetchMessagesUntilQueryString(m_accountSettings->jid(), m_chatController->jid(), foundIndex, searchString)
            .then(this, [this](MessageDb::MessageResult &&result) {
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
            if (m_messages.at(foundIndex).body().contains(searchString, Qt::CaseInsensitive))
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
    if (msgs.size() < DB_QUERY_LIMIT_MESSAGES) {
        m_fetchedAllFromDb = true;
    }

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

    m_messages.append(msgs);

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

    if (m_chatController->rosterItem().lastReadContactMessageId.isEmpty()) {
        for (const auto &message : std::as_const(m_messages)) {
            if (!message.isOwn) {
                RosterDb::instance()->updateItem(m_accountSettings->jid(), m_chatController->jid(), [=, messageId = message.id](RosterItem &item) {
                    item.lastReadContactMessageId = messageId;
                });
                break;
            }
        }
    }

    Q_EMIT messageFetchingFinished();
}

void MessageModel::handleMessage(Message msg, MessageOrigin)
{
    if (msg.accountJid == m_accountSettings->jid() && msg.chatJid == m_chatController->jid()) {
        addMessage(std::move(msg));
    }
}

void MessageModel::handleMessageUpdated(Message message)
{
    if (message.accountJid != m_accountSettings->jid()) {
        return;
    }

    for (int i = 0; i < m_messages.size(); i++) {
        const auto &oldMessage = m_messages.at(i);
        const auto oldId = oldMessage.id;
        const auto oldReplaceId = oldMessage.replaceId;

        // The updated message can be either a normal message, a first message correction, an own
        // reflected group chat message or a subsequent message correction.
        if ((!oldId.isEmpty() && (oldId == message.id || oldId == message.replaceId || oldId == message.originId))
            || (!oldReplaceId.isEmpty() && oldReplaceId == message.replaceId)) {
            m_messages.replace(i, message);
            const auto modelIndex = index(i);
            Q_EMIT dataChanged(modelIndex, modelIndex);
            break;
        }
    }
}

void MessageModel::handleDevicesChanged(QList<QString> jids)
{
    // TODO: Search through all messages and only fetch trust levels for relevant keys, set those collected trust levels afterwards via another loop through all
    // messages

    for (auto itr = m_messages.begin(); itr != m_messages.end(); ++itr) {
        if (const auto senderJid = itr->senderJid(); jids.contains(senderJid)) {
            // Do not retrieve the trust level of this device and for unencrypted messages.
            if (const auto senderKey = itr->senderKey; senderKey.isEmpty()) {
                continue;
            }

            m_atmController->trustLevel(XMLNS_OMEMO_2, senderJid, itr->senderKey).then([this, itr](QXmpp::TrustLevel trustLevel) {
                // TODO: Search message again here because itr may not be up-to-date

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
}

void MessageModel::removeMessages(const QString &accountJid, const QString &chatJid)
{
    if (accountJid == m_accountSettings->jid() && chatJid == m_chatController->jid()) {
        removeAllMessages();
    }
}

void MessageModel::updateLastReadOwnMessageId()
{
    const auto formerLastReadOwnMessageId = m_lastReadOwnMessageId;
    m_lastReadOwnMessageId = m_chatController->rosterItem().lastReadOwnMessageId;
    emitMessagesUpdated({formerLastReadOwnMessageId, m_lastReadOwnMessageId}, IsLastReadOwnMessage);
}

void MessageModel::updateFirstUnreadContactMessageIndex()
{
    int lastReadContactMessageIndex = -1;

    for (auto i = 0; i < m_messages.size(); ++i) {
        const auto &message = m_messages.at(i);
        const auto &messageId = message.id;
        const auto lastReadContactMessageId = m_chatController->rosterItem().lastReadContactMessageId;

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

void MessageModel::emitMessagesUpdated(const QList<QString> &messageIds, MessageRoles role)
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

bool MessageModel::undoMessageReactionRemoval(const QString &messageId, const QString &senderJid, const QString &emoji, const QList<MessageReaction> &reactions)
{
    const auto reactionItr = std::ranges::find_if(reactions, [&](const MessageReaction &reaction) {
        return reaction.emoji == emoji;
    });

    if (reactionItr != reactions.end()) {
        MessageDb::instance()->updateMessage(m_chatController->account()->settings()->jid(),
                                             m_chatController->jid(),
                                             messageId,
                                             [senderJid, emoji](Message &message) {
                                                 auto &reactions = message.reactionSenders[senderJid].reactions;

                                                 const auto itr = std::ranges::find_if(reactions, [&](const MessageReaction &reaction) {
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
    const auto reactionItr = std::ranges::find_if(reactions, [&](const MessageReaction &reaction) {
        return reaction.emoji == emoji
            && (reaction.deliveryState == MessageReactionDeliveryState::PendingAddition
                || reaction.deliveryState == MessageReactionDeliveryState::ErrorOnAddition);
    });

    if (reactionItr != reactions.end()) {
        MessageDb::instance()->updateMessage(m_chatController->account()->settings()->jid(),
                                             m_chatController->jid(),
                                             messageId,
                                             [senderJid, emoji](Message &message) {
                                                 auto &reactionSenders = message.reactionSenders;
                                                 auto &reactions = reactionSenders[senderJid].reactions;

                                                 const auto itr = std::ranges::find_if(reactions, [&](const MessageReaction &reaction) {
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

QString MessageModel::determineReplyToName(const Message::Reply &reply) const
{
    if (const auto replyToJid = reply.toJid; !replyToJid.isEmpty()) {
        if (const auto rosterItem = RosterModel::instance()->item(m_accountSettings->jid(), replyToJid)) {
            return rosterItem->displayName();
        }
    }

    return reply.toGroupChatParticipantName;
}

#include "moc_MessageModel.cpp"
