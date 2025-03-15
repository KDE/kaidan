// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageController.h"

// Qt
#include <QUrl>
// QXmpp
#include <QXmppBitsOfBinaryContentId.h>
#include <QXmppBitsOfBinaryDataList.h>
#include <QXmppE2eeMetadata.h>
#include <QXmppEncryptedFileSource.h>
#include <QXmppFallback.h>
#include <QXmppFileMetadata.h>
#include <QXmppHash.h>
#include <QXmppHttpFileSource.h>
#include <QXmppMamManager.h>
#include <QXmppMessageReaction.h>
#include <QXmppMixInvitation.h>
#include <QXmppOutOfBandUrl.h>
#include <QXmppRosterManager.h>
#include <QXmppThumbnail.h>
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "AccountDb.h"
#include "AccountManager.h"
#include "Algorithms.h"
#include "ChatController.h"
#include "ClientWorker.h"
#include "EncryptionController.h"
#include "FutureUtils.h"
#include "Globals.h"
#include "GroupChatController.h"
#include "GroupChatUser.h"
#include "GroupChatUserDb.h"
#include "Kaidan.h"
#include "KaidanCoreLog.h"
#include "MediaUtils.h"
#include "MessageDb.h"
#include "MessageModel.h"
#include "Notifications.h"
#include "RosterDb.h"
#include "RosterManager.h"
#include "RosterModel.h"

// Number of messages fetched at once when loading MAM backlog
constexpr int MAM_BACKLOG_FETCH_COUNT = 40;

using namespace std::placeholders;
using std::ranges::find;

MessageController *MessageController::s_instance = nullptr;

MessageController *MessageController::instance()
{
    return s_instance;
}

MessageController::MessageController(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    connect(RosterDb::instance(), &RosterDb::itemsReplaced, this, &MessageController::handleRosterReceived);

    runOnThread(Kaidan::instance()->client(), [this]() {
        connect(Kaidan::instance()->client()->xmppClient(), &QXmppClient::messageReceived, this, [this](const QXmppMessage &msg) {
            handleMessage(msg, MessageOrigin::Stream);
        });

        connect(Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMessageReceiptManager>(),
                &QXmppMessageReceiptManager::messageDelivered,
                this,
                [](const QString &jid, const QString &id) {
                    MessageDb::instance()->updateMessage(id,
                                                         [accountJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare(),
                                                          jid = QXmppUtils::jidToBareJid(jid)](Message &message) {
                                                             // Only the recipient of the message is allowed to confirm its delivery.
                                                             if (message.accountJid == accountJid && message.chatJid == jid) {
                                                                 message.deliveryState = Enums::DeliveryState::Delivered;
                                                                 message.errorText.clear();
                                                             }
                                                         });
                });
    });
}

MessageController::~MessageController()
{
    s_instance = nullptr;
}

QFuture<QXmpp::SendResult> MessageController::send(QXmppMessage &&message)
{
    QFutureInterface<QXmpp::SendResult> interface(QFutureInterfaceBase::Started);

    const auto recipientJid = message.to();

    auto sendEncrypted = [=, this](const QList<QString> &groupChatUserJids = {}) mutable {
        if (groupChatUserJids.isEmpty()) {
            runOnThread(Kaidan::instance()->client()->xmppClient(), [this, interface, message]() mutable {
                Kaidan::instance()->client()->xmppClient()->sendSensitive(std::move(message)).then(this, [=](QXmpp::SendResult &&result) mutable {
                    reportFinishedResult(interface, result);
                });
            });
        } else {
            QXmppSendStanzaParams params;
            params.setEncryptionJids(groupChatUserJids);

            runOnThread(Kaidan::instance()->client()->xmppClient(), [this, interface, message, params]() mutable {
                Kaidan::instance()->client()->xmppClient()->sendSensitive(std::move(message), params).then(this, [=](QXmpp::SendResult &&result) mutable {
                    reportFinishedResult(interface, result);
                });
            });
        }
    };

    auto sendUnencrypted = [=, this]() mutable {
        // Ensure that a message containing files but without a body is stored/archived via MAM by
        // the server.
        // That is not needed if the message is encrypted because that is handled by the
        // corresponding encryption manager.
        if (!message.sharedFiles().isEmpty() && message.body().isEmpty()) {
            message.addHint(QXmppMessage::Store);
        }

        runOnThread(Kaidan::instance()->client()->xmppClient(), [this, interface, message]() mutable {
            Kaidan::instance()->client()->xmppClient()->send(std::move(message)).then(this, [=](QXmpp::SendResult &&result) mutable {
                reportFinishedResult(interface, result);
            });
        });
    };

    // If the message is sent for the current chat, its information is used to determine whether to
    // send encrypted.
    // Otherwise, that information is retrieved from the database.
    if (ChatController::instance()->isChatCurrentChat(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), recipientJid)) {
        if (ChatController::instance()->isEncryptionEnabled()) {
            if (const auto rosterItem = RosterModel::instance()->findItem(recipientJid); rosterItem && rosterItem->isGroupChat()) {
                sendEncrypted(GroupChatController::instance()->currentUserJids());
            } else {
                sendEncrypted();
            }
        } else {
            sendUnencrypted();
        }
    } else {
        const auto rosterItem = RosterModel::instance()->findItem(recipientJid);

        if (const auto omemoEncryptionActive = rosterItem && rosterItem->encryption == Encryption::Omemo2) {
            if (rosterItem->isGroupChat()) {
                await(GroupChatUserDb::instance()->userJids(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), recipientJid),
                      this,
                      [this, omemoEncryptionActive, sendEncrypted, sendUnencrypted](QList<QString> &&userJids) mutable {
                          if (!userJids.isEmpty()) {
                              await(EncryptionController::instance()->hasUsableDevices({userJids.cbegin(), userJids.cend()}),
                                    this,
                                    [omemoEncryptionActive, sendEncrypted, sendUnencrypted, userJids](bool hasUsableDevices) mutable {
                                        if (omemoEncryptionActive && hasUsableDevices) {
                                            sendEncrypted(userJids);
                                        } else {
                                            sendUnencrypted();
                                        }
                                    });
                          }
                      });
            } else {
                await(EncryptionController::instance()->hasUsableDevices({recipientJid}),
                      this,
                      [omemoEncryptionActive, sendEncrypted, sendUnencrypted](bool hasUsableDevices) mutable {
                          if (omemoEncryptionActive && hasUsableDevices) {
                              sendEncrypted();
                          } else {
                              sendUnencrypted();
                          }
                      });
            }
        } else {
            sendUnencrypted();
        }
    }

    return interface.future();
}

void MessageController::sendPendingMessages()
{
    await(MessageDb::instance()->fetchPendingMessages(AccountManager::instance()->account().jid), this, [this](QList<Message> &&messages) {
        for (Message message : messages) {
            sendPendingMessage(std::move(message));
        }
    });
}

void MessageController::sendPendingMessageReactions(const QString &accountJid)
{
    auto future = MessageDb::instance()->fetchPendingReactions(accountJid);
    await(future, this, [=, this](QMap<QString, QMap<QString, MessageReactionSender>> reactions) {
        for (auto reactionItr = reactions.cbegin(); reactionItr != reactions.cend(); ++reactionItr) {
            const auto &reactionSenders = reactionItr.value();

            for (auto reactionSenderItr = reactionSenders.cbegin(); reactionSenderItr != reactionSenders.cend(); ++reactionSenderItr) {
                const auto messageId = reactionSenderItr.key();
                QList<QString> emojis;

                for (const auto &reaction : reactionSenderItr->reactions) {
                    if (const auto deliveryState = reaction.deliveryState; deliveryState != MessageReactionDeliveryState::PendingRemovalAfterSent
                        && deliveryState != MessageReactionDeliveryState::PendingRemovalAfterDelivered
                        && deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterSent
                        && deliveryState != MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered) {
                        emojis.append(reaction.emoji);
                    }
                }

                const auto chatJid = reactionItr.key();

                await(MessageController::instance()->sendMessageReaction(ChatController::instance()->chatJid(),
                                                                         messageId,
                                                                         ChatController::instance()->rosterItem().isGroupChat(),
                                                                         emojis),
                      this,
                      [this, messageId, accountJid](QXmpp::SendResult &&result) {
                          if (const auto error = std::get_if<QXmppError>(&result)) {
                              Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Reaction could not be sent: %1").arg(error->description));

                              MessageDb::instance()->updateMessage(messageId, [accountJid](Message &message) {
                                  auto &reactionSender = message.reactionSenders[accountJid];
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
                              updateMessageReactionsAfterSending(messageId, accountJid);
                          }
                      });
            }
        }
    });
}

void MessageController::updateMessageReactionsAfterSending(const QString &messageId, const QString &senderId)
{
    MessageDb::instance()->updateMessage(messageId, [senderId](Message &message) {
        auto &reactionSenders = message.reactionSenders;
        auto &reactionSender = reactionSenders[senderId];
        reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();
        auto &reactions = reactionSender.reactions;

        for (auto itr = reactions.begin(); itr != reactions.end();) {
            switch (auto &deliveryState = itr->deliveryState) {
            case MessageReactionDeliveryState::PendingAddition:
            case MessageReactionDeliveryState::ErrorOnAddition:
                deliveryState = MessageReactionDeliveryState::Sent;
                ++itr;
                break;
            case MessageReactionDeliveryState::PendingRemovalAfterSent:
            case MessageReactionDeliveryState::PendingRemovalAfterDelivered:
            case MessageReactionDeliveryState::ErrorOnRemovalAfterSent:
            case MessageReactionDeliveryState::ErrorOnRemovalAfterDelivered:
                itr = reactions.erase(itr);
                break;
            default:
                ++itr;
                break;
            }
        }

        // Remove the reaction sender if it has no reactions anymore.
        if (reactions.isEmpty()) {
            reactionSenders.remove(senderId);
        }
    });
}

void MessageController::sendChatState(const QString &toJid, bool isGroupChat, const QXmppMessage::State state)
{
    QXmppMessage message;

    if (isGroupChat) {
        message.setType(QXmppMessage::GroupChat);
    }

    message.setTo(toJid);
    message.setState(state);
    send(std::move(message));
}

QFuture<QXmpp::SendResult>
MessageController::sendMessageReaction(const QString &chatJid, const QString &messageId, bool isGroupChatMessage, const QList<QString> &emojis)
{
    QXmppMessageReaction reaction;
    reaction.setMessageId(messageId);
    reaction.setEmojis(emojis);

    QXmppMessage message;

    if (isGroupChatMessage) {
        message.setType(QXmppMessage::GroupChat);
    }

    message.setTo(chatJid);
    message.setStamp(QDateTime::currentDateTimeUtc());
    message.addHint(QXmppMessage::Store);
    message.setReaction(reaction);
    message.setReceiptRequested(!isGroupChatMessage);

    return send(std::move(message));
}

void MessageController::sendPendingMessage(Message message)
{
    runOnThread(
        Kaidan::instance()->client()->xmppClient(),
        []() {
            return Kaidan::instance()->client()->xmppClient()->state();
        },
        this,
        [this, message](QXmppClient::State state) mutable {
            if (state == QXmppClient::ConnectedState) {
                // if the message is a pending edition of the existing in the history message
                // I need to send it with the most recent stamp
                // for that I'm gonna copy that message and update in the copy just the stamp
                if (!message.replaceId.isEmpty()) {
                    message.timestamp = QDateTime::currentDateTimeUtc();
                }

                message.receiptRequested = !message.isGroupChatMessage();

                await(send(message.toQXmpp()),
                      this,
                      [this, messageId = message.id, fileFallbackMessages = message.fileFallbackMessages()](QXmpp::SendResult result) mutable {
                          if (const auto error = std::get_if<QXmppError>(&result)) {
                              qCWarning(KAIDAN_CORE_LOG) << "Could not send message:" << error->description;

                              // The error message of the message is saved untranslated. To make
                              // translation work in the UI, the tr() call of the passive
                              // notification must contain exactly the same string.
                              Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Message could not be sent."));
                              MessageDb::instance()->updateMessage(messageId, [](Message &msg) {
                                  msg.deliveryState = Enums::DeliveryState::Error;
                                  msg.errorText = QStringLiteral("Message could not be sent.");
                              });
                          } else {
                              MessageDb::instance()->updateMessage(messageId, [](Message &msg) {
                                  msg.deliveryState = Enums::DeliveryState::Sent;
                                  msg.errorText.clear();
                              });

                              for (auto &fileFallbackMessage : fileFallbackMessages) {
                                  // TODO: Track sending of fallback messages individually
                                  // Needed for the case when the success differs between the main message
                                  // and the fallback messages.
                                  send(std::move(fileFallbackMessage));
                              }
                          }
                      });
            }
        });
}

QFuture<bool> MessageController::retrieveBacklogMessages(const QString &jid, bool isGroupChat, const QString &oldestMessageStanzaId)
{
    QFutureInterface<bool> interface(QFutureInterfaceBase::Started);

    QXmppResultSetQuery queryLimit;
    queryLimit.setMax(MAM_BACKLOG_FETCH_COUNT);
    queryLimit.setBefore(oldestMessageStanzaId);

    // TODO: Find out why sometimes messages are not correctly retrieved (maybe not even correctly parsed, at least with Tigase)
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, jid, isGroupChat, queryLimit]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMamManager>()->retrieveMessages(isGroupChat ? jid : QString{},
                                                                                                                            {},
                                                                                                                            isGroupChat ? QString{} : jid,
                                                                                                                            {},
                                                                                                                            {},
                                                                                                                            queryLimit),
                             this};
        },
        this,
        [this, interface, jid](QXmppMamManager::RetrieveResult &&result) mutable {
            if (auto error = std::get_if<QXmppError>(&result)) {
                qCDebug(KAIDAN_CORE_LOG) << "Could not retrieve backlog messages:" << error->description;
                reportFinishedResult(interface, false);
            } else {
                const auto retrievedMessages = std::get<QXmppMamManager::RetrievedMessages>(std::move(result));
                const auto messages = retrievedMessages.messages;

                for (const auto &message : messages) {
                    handleMessage(message, MessageOrigin::MamBacklog);
                }

                reportFinishedResult(interface, retrievedMessages.result.complete());
            }
        });

    return interface.future();
}

void MessageController::sendReadMarker(const QString &chatJid, const QString &messageId)
{
    QXmppMessage message;

    if (const auto rosterItem = RosterModel::instance()->findItem(chatJid); rosterItem && rosterItem->isGroupChat()) {
        message.setType(QXmppMessage::GroupChat);
    }

    message.setTo(chatJid);
    message.setMarker(QXmppMessage::Displayed);
    message.setMarkerId(messageId);
    message.addHint(QXmppMessage::Store);

    send(std::move(message));
}

void MessageController::handleRosterReceived()
{
    // TODO: Retrieve initial messages only if MAM is enabled

    // If it is Kaidan's first login with the account, request one initial message for each roster
    // item.
    // If Kaidan already retrieved initial messages, request all new messages from the server since
    // then.
    // If Kaidan already tried to retrieve the initial messages but there is none, request all
    // messages in order to get the messages since the last request.
    if (AccountManager::instance()->hasNewAccount()) {
        retrieveInitialMessages();
    } else {
        await(AccountDb::instance()->fetchLatestMessageStanzaId(Kaidan::instance()->client()->xmppClient()->configuration().jidBare()),
              this,
              [this](QString &&stanzaId) {
                  if (stanzaId.isEmpty()) {
                      retrieveAllMessages();
                  } else {
                      retrieveCatchUpMessages(stanzaId);
                  }
              });

        if (const auto rosterItems = RosterModel::instance()->items(); !rosterItems.isEmpty()) {
            // TODO: Check whether the own server supports archiving group chat messages and skip the retrieval for each group chat in that case
            for (const auto &rosterItem : std::as_const(rosterItems)) {
                if (rosterItem.isGroupChat()) {
                    if (rosterItem.latestGroupChatMessageStanzaId.isEmpty()) {
                        retrieveAllMessages(rosterItem.jid);
                    } else {
                        retrieveCatchUpMessages(rosterItem.latestGroupChatMessageStanzaId, rosterItem.jid);
                    }
                }
            }
        }
    }
}

void MessageController::retrieveInitialMessages()
{
    if (const auto rosterItems = RosterModel::instance()->items(); !rosterItems.isEmpty()) {
        for (const auto &rosterItem : std::as_const(rosterItems)) {
            retrieveInitialMessage(rosterItem.jid, rosterItem.isGroupChat());
        }
    }
}

void MessageController::retrieveInitialMessage(const QString &jid, bool isGroupChat, const QString &offsetMessageId)
{
    QXmppResultSetQuery queryLimit;
    queryLimit.setMax(1);
    queryLimit.setBefore(offsetMessageId);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this, jid, isGroupChat, queryLimit]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMamManager>()->retrieveMessages(isGroupChat ? jid : QString{},
                                                                                                                            {},
                                                                                                                            isGroupChat ? QString{} : jid,
                                                                                                                            {},
                                                                                                                            {},
                                                                                                                            queryLimit),
                             this};
        },
        this,
        [this, jid, isGroupChat](QXmppMamManager::RetrieveResult &&result) mutable {
            if (auto error = std::get_if<QXmppError>(&result)) {
                qCDebug(KAIDAN_CORE_LOG) << "Could not retrieve initial message:" << error->description;
            } else {
                const auto retrievedMessages = std::get<QXmppMamManager::RetrievedMessages>(std::move(result));

                if (const auto messages = retrievedMessages.messages; !messages.empty()) {
                    const auto message = messages.constFirst();

                    if (message.body().isEmpty() && message.sharedFiles().isEmpty()) {
                        retrieveInitialMessage(jid, isGroupChat, retrievedMessages.result.resultSetReply().first());
                    } else {
                        handleMessage(message, MessageOrigin::MamInitial);
                    }
                }
            }
        });
}

void MessageController::retrieveAllMessages(const QString &groupChatJid)
{
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, groupChatJid]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMamManager>()->retrieveMessages(groupChatJid), this};
        },
        this,
        [this, groupChatJid](QXmppMamManager::RetrieveResult &&result) mutable {
            if (auto error = std::get_if<QXmppError>(&result)) {
                qCDebug(KAIDAN_CORE_LOG) << "Could not retrieve all messages:" << error->description;
            } else {
                const auto retrievedMessages = std::get<QXmppMamManager::RetrievedMessages>(std::move(result));

                for (const auto &message : std::as_const(retrievedMessages.messages)) {
                    handleMessage(message, MessageOrigin::MamCatchUp);

                    // Send delivery receipts for the retrieved message.
                    Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMessageReceiptManager>()->handleMessage(message);
                }
            }
        });
}

void MessageController::retrieveCatchUpMessages(const QString &latestMessageStanzaId, const QString &groupChatJid)
{
    QXmppResultSetQuery queryLimit;
    queryLimit.setAfter(latestMessageStanzaId);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this, groupChatJid, queryLimit]() {
            return std::pair{
                Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMamManager>()->retrieveMessages(groupChatJid, {}, {}, {}, {}, queryLimit),
                this};
        },
        this,
        [this, groupChatJid](QXmppMamManager::RetrieveResult &&result) mutable {
            if (auto error = std::get_if<QXmppError>(&result)) {
                qCDebug(KAIDAN_CORE_LOG) << "Could not retrieve catch-up messages:" << error->description;
            } else {
                const auto retrievedMessages = std::get<QXmppMamManager::RetrievedMessages>(std::move(result));

                for (const auto &message : std::as_const(retrievedMessages.messages)) {
                    handleMessage(message, MessageOrigin::MamCatchUp);

                    // Send delivery receipts for the retrieved message.
                    Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMessageReceiptManager>()->handleMessage(message);
                }
            }
        });
}

void MessageController::handleMessage(const QXmppMessage &msg, MessageOrigin origin)
{
    if (msg.type() == QXmppMessage::Error) {
        MessageDb::instance()->updateMessage(msg.id(), [errorText{msg.error().text()}](Message &msg) {
            msg.deliveryState = Enums::DeliveryState::Error;
            msg.errorText = errorText;
        });
        return;
    }

    const auto accountJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare();
    const auto senderJid = QXmppUtils::jidToBareJid(msg.from());
    const auto receivedFromGroupChat = msg.type() == QXmppMessage::GroupChat;

    QString ownGroupChatParticipantId;
    bool isOwn;
    const auto groupChatSenderId = msg.mixParticipantId();

    if (receivedFromGroupChat) {
        // Skip messages from group chats that the user is not participating in.
        if (const auto groupChat = RosterModel::instance()->findItem(senderJid)) {
            ownGroupChatParticipantId = groupChat->groupChatParticipantId;
            isOwn = groupChatSenderId == ownGroupChatParticipantId;
        } else {
            return;
        }

    } else {
        isOwn = senderJid == accountJid;
    }

    QString chatJid;
    QString senderId;
    const auto recipientJid = QXmppUtils::jidToBareJid(msg.to());

    if (receivedFromGroupChat) {
        chatJid = senderJid;
        senderId = isOwn ? accountJid : groupChatSenderId;
    } else {
        chatJid = isOwn ? recipientJid : senderJid;
        senderId = isOwn ? accountJid : chatJid;
    }

    if (const auto chatState = msg.state(); chatState != QXmppMessage::State::None) {
        ChatController::instance()->handleChatState(senderJid, chatState);
    }

    const auto messageId = msg.id();
    const auto stanzaIds = msg.stanzaIds();
    QString stanzaId;

    // Retrieve the appropriate stanza ID.
    //
    // If the group chat message has multiple stanza IDs, the right stanza ID depending on its
    // originator is used.
    // That is needed if the own server archives group chat messages resulting in a second stanza
    // ID added by the own server to the group chat message.
    if (receivedFromGroupChat) {
        const auto itr = std::find_if(stanzaIds.cbegin(), stanzaIds.cend(), [senderJid](const QXmppStanzaId &stanzaId) {
            return stanzaId.by == senderJid;
        });

        if (itr == stanzaIds.cend()) {
            // Use the message ID as a fallback for the stanza ID in case the group chat server does
            // not provide a stanza ID.
            // That is the case for ejabberd (at least until version 24.10).
            stanzaId = messageId;
        } else {
            stanzaId = itr->id;
        }
    } else if (!stanzaIds.isEmpty()) {
        stanzaId = msg.stanzaIds().first().id;
    }

    const auto timestamp = msg.stamp().isValid() ? msg.stamp().toUTC() : QDateTime::currentDateTimeUtc();

    // Every message that is stored on the server is a candidate for being the message whose
    // stanza ID is used for retrieving offline (i.e., catch up) messages once connected.
    updateLatestMessage(accountJid, chatJid, stanzaId, timestamp, receivedFromGroupChat);

    if (handleReadMarker(msg, senderJid, recipientJid, isOwn) || handleReaction(msg, senderId) || handleFileSourcesAttachments(msg, chatJid)) {
        return;
    }

    const auto qxmppReply = msg.reply();
    const auto messageBody = qxmppReply ? msg.readFallbackRemovedText(QXmppFallback::Element::Body, {XMLNS_MESSAGE_REPLIES.toString()}) : msg.body();
    const auto encryptionName = msg.encryptionName();

    // Determine whether the message could be encrypted for a group chat or the chat with oneself
    // and reflected by the server.
    // In that case, the message cannot be decrypted by this device because a sending device does
    // not encrypt for itself.
    // Thus, there is no decrypted body (except a possible fallback body, e2eeFallbackBody()).
    // That is needed for marking the sent message as delivered.
    const auto possiblyReflectedEncrypted = isOwn && !encryptionName.isEmpty();

    if (!possiblyReflectedEncrypted && messageBody.isEmpty() && msg.outOfBandUrl().isEmpty() && msg.sharedFiles().isEmpty()) {
        return;
    }

    Message message;

    message.accountJid = accountJid;
    message.chatJid = chatJid;
    message.isOwn = isOwn;
    message.groupChatSenderId = groupChatSenderId;

    // Set a generated message ID for local use (removing a message locally etc.) if it is empty.
    // That behavior was detected for server messages.
    message.id = messageId.isEmpty() ? QXmppUtils::generateStanzaUuid() : messageId;

    message.originId = msg.originId();
    message.stanzaId = stanzaId;

    if (qxmppReply) {
        Message::Reply reply;

        if (receivedFromGroupChat) {
            if (const auto groupChatParticipantId = qxmppReply->to; groupChatParticipantId != ownGroupChatParticipantId) {
                reply.toGroupChatParticipantId = qxmppReply->to;
            }
        } else if (const auto toJid = qxmppReply->to; toJid != accountJid) {
            reply.toJid = toJid;
        }

        reply.id = qxmppReply->id;
        reply.quote = msg.readReplyQuoteFromBody();

        message.reply = reply;
    }

    message.isSpoiler = msg.isSpoiler();
    message.spoilerHint = msg.spoilerHint();

    // file sharing messages for backwards-compatibility are ignored
    if (find(msg.fallbackMarkers(), XMLNS_SFS, &QXmppFallback::forNamespace) != msg.fallbackMarkers().end() && msg.sharedFiles().empty()) {
        return;
    }

    // Close a notification for messages to which the user replied via another own resource.
    if (isOwn) {
        Notifications::instance()->closeMessageNotification(senderJid, recipientJid);
    }

    if (const auto e2eeMetadata = msg.e2eeMetadata()) {
        message.encryption = Encryption::Enum(e2eeMetadata->encryption());
        message.senderKey = e2eeMetadata->senderKey();
    }

    parseSharedFiles(msg, message);

    if (!encryptionName.isEmpty() && message.encryption == Encryption::NoEncryption) {
        message.setPreparedBody(tr("This message is encrypted with %1 but could not be decrypted").arg(encryptionName));
    } else {
        // Do not use the file sharing fallback body.
        if (messageBody != msg.outOfBandUrl()) {
            message.setUnpreparedBody(messageBody);
        }
    }

    if (const auto mixInvitation = msg.mixInvitation()) {
        message.groupChatInvitation = {
            .inviterJid = mixInvitation->inviterJid(),
            .inviteeJid = mixInvitation->inviteeJid(),
            .groupChatJid = mixInvitation->channelJid(),
            .token = mixInvitation->token(),
        };
    }

    // Ignore messages without any displayable content.
    if (message.body().isEmpty() && !message.groupChatInvitation) {
        return;
    }

    // If the message is sent from this device and reflected from a group chat or the chat with
    // oneself, update its delivery state and store the received "stanzaId".
    // The "stanzaId" is updated to be used when the message is referenced (e.g., if message
    // reactions are sent for it).
    auto updateReflectedMessage = [](Message &storedMessage, const QString &stanzaId) {
        if (storedMessage.isOwn && storedMessage.deliveryState == Enums::DeliveryState::Sent
            && (storedMessage.isGroupChatMessage() || storedMessage.accountJid == storedMessage.chatJid)) {
            storedMessage.deliveryState = Enums::DeliveryState::Delivered;
            storedMessage.errorText.clear();
            storedMessage.stanzaId = stanzaId;

            return true;
        }

        return false;
    };

    // If the message is a new message, store it.
    // Otherwise, correct a stored message.
    if (const auto replaceId = msg.replaceId(); replaceId.isEmpty()) {
        // Add a new group chat user if none is stored for the received message.
        // "groupChatSenderId" is checked because it can be empty if the server uses an unsupported
        // format for participants.
        if (receivedFromGroupChat && !groupChatSenderId.isEmpty() && !isOwn) {
            GroupChatUser sender;

            sender.accountJid = accountJid;
            sender.chatJid = chatJid;
            sender.id = groupChatSenderId;
            sender.jid = msg.mixUserJid();
            sender.name = msg.mixUserNick();

            GroupChatUserDb::instance()->handleMessageSender(sender);
        }

        message.timestamp = timestamp;

        // If the message is fetched via the initial MAM fetching, store the message directly
        // because it is ensured that the message is not already stored.
        // If the message is reflected, update it.
        // If the message is not reflected but new, store it.
        if (origin == MessageOrigin::MamInitial) {
            MessageDb::instance()->addMessage(message, origin);
        } else {
            MessageDb::instance()->addOrUpdateMessage(message, origin, message.originId, [updateReflectedMessage, stanzaId](Message &storedMessage) {
                updateReflectedMessage(storedMessage, stanzaId);
            });
        }

        // Add the message's sender to the roster if not already done and only for direct
        // messages.
        // Otherwise, the chat could only be opened via the message's notification and could not
        // be opened again later.
        if (!receivedFromGroupChat && !RosterModel::instance()->hasItem(senderJid)) {
            runOnThread(Kaidan::instance()->client(), [senderJid]() {
                Kaidan::instance()->client()->rosterManager()->addContact(senderJid, {}, {}, true);
            });
        }
    } else {
        message.replaceId = replaceId;
        MessageDb::instance()->updateMessage(replaceId, [updateReflectedMessage, message](Message &storedMessage) {
            // Update a reflected message or correct a previous message if allowed.
            // Only the author of the original message is allowed to correct it.
            if (!updateReflectedMessage(storedMessage, message.stanzaId) && storedMessage.accountJid == message.accountJid
                && storedMessage.chatJid == message.chatJid && storedMessage.isOwn == message.isOwn
                && storedMessage.groupChatSenderId == message.groupChatSenderId) {
                storedMessage.id = message.id;
                storedMessage.originId = message.originId;
                storedMessage.stanzaId = message.stanzaId;
                storedMessage.replaceId = message.replaceId;
                storedMessage.reply = message.reply;
                storedMessage.setPreparedBody(message.body());
                storedMessage.encryption = message.encryption;
                storedMessage.senderKey = message.senderKey;
                storedMessage.isSpoiler = message.isSpoiler;
                storedMessage.spoilerHint = message.spoilerHint;
                storedMessage.senderKey = message.senderKey;
                storedMessage.groupChatInvitation = message.groupChatInvitation;
            }
        });
    }
}

void MessageController::updateLatestMessage(const QString &accountJid,
                                            const QString &chatJid,
                                            const QString &stanzaId,
                                            const QDateTime &timestamp,
                                            bool receivedFromGroupChat)
{
    if (stanzaId.isEmpty()) {
        return;
    }

    // TODO: Check whether the own server supports archiving group chat messages and do not store stanza IDs for each group chat in that case
    if (receivedFromGroupChat) {
        RosterDb::instance()->updateItem(chatJid, [stanzaId, timestamp](RosterItem &item) {
            // Check "<=" instead of "<" to update the ID in the rare case that the timestamps are
            // equal (e.g., because the timestamps are not precise enough).
            if (item.latestGroupChatMessageStanzaTimestamp <= timestamp) {
                item.latestGroupChatMessageStanzaId = stanzaId;
                item.latestGroupChatMessageStanzaTimestamp = timestamp;
            }
        });
    } else {
        AccountDb::instance()->updateAccount(accountJid, [stanzaId, timestamp](Account &account) {
            // Check "<=" instead of "<" to update the ID in the rare case that the timestamps are
            // equal (e.g., because the timestamps are not precise enough).
            if (account.latestMessageStanzaTimestamp <= timestamp) {
                account.latestMessageStanzaId = stanzaId;
                account.latestMessageStanzaTimestamp = timestamp;
            }
        });
    }
}

bool MessageController::handleReadMarker(const QXmppMessage &message, const QString &senderJid, const QString &recipientJid, bool isOwnMessage)
{
    if (message.marker() == QXmppMessage::Displayed) {
        const auto markedId = message.markedId();
        if (isOwnMessage) {
            Notifications::instance()->closeMessageNotification(senderJid, recipientJid);

            // Retrieve the count of messages between "lastReadContactMessageId" and "markedId"
            // to decrease the corresponding counter by 1 (if IDs could not be found) or by the
            // actual count of read messages.
            auto future = MessageDb::instance()->messageCount(recipientJid,
                                                              senderJid,
                                                              RosterModel::instance()->lastReadContactMessageId(senderJid, recipientJid),
                                                              markedId);
            await(future, this, [recipientJid, markedId](int count) {
                RosterDb::instance()->updateItem(recipientJid, [=](RosterItem &item) {
                    item.unreadMessages = count == 0 ? item.unreadMessages - 1 : item.unreadMessages - count + 1;
                    item.lastReadContactMessageId = markedId;
                    item.readMarkerPending = false;
                });
            });
        } else {
            RosterDb::instance()->updateItem(senderJid, [markedId](RosterItem &item) {
                item.lastReadOwnMessageId = markedId;
            });
        }

        return true;
    }

    return false;
}

bool MessageController::handleReaction(const QXmppMessage &message, const QString &senderId)
{
    if (const auto receivedReaction = message.reaction()) {
        MessageDb::instance()->updateMessage(message.reaction()->messageId(),
                                             [senderId, receivedEmojis = receivedReaction->emojis(), receivedTimestamp = message.stamp()](Message &m) {
                                                 auto &reactionSenders = m.reactionSenders;
                                                 auto &reactionSender = reactionSenders[senderId];

                                                 // Process only newer reactions.
                                                 if (reactionSender.latestTimestamp.isNull() || reactionSender.latestTimestamp < receivedTimestamp) {
                                                     reactionSender.latestTimestamp = receivedTimestamp;
                                                     auto &reactions = reactionSender.reactions;

                                                     // Add new reactions.
                                                     for (const auto &receivedEmoji : std::as_const(receivedEmojis)) {
                                                         const auto reactionNew =
                                                             std::none_of(reactions.cbegin(), reactions.cend(), [&](const MessageReaction &reaction) {
                                                                 return reaction.emoji == receivedEmoji;
                                                             });

                                                         if (reactionNew) {
                                                             MessageReaction reaction;
                                                             reaction.emoji = receivedEmoji;

                                                             reactions.append(reaction);
                                                         }
                                                     }

                                                     // Remove existing reactions.
                                                     for (auto itr = reactions.begin(); itr != reactions.end();) {
                                                         if (!receivedEmojis.contains(itr->emoji)) {
                                                             reactions.erase(itr);
                                                         } else {
                                                             ++itr;
                                                         }
                                                     }

                                                     // Remove the reaction sender if it has no reactions anymore.
                                                     if (reactions.isEmpty()) {
                                                         reactionSenders.remove(senderId);
                                                     }
                                                 }
                                             });

        return true;
    }

    return false;
}

bool MessageController::handleFileSourcesAttachments(const QXmppMessage &message, const QString &chatJid)
{
    if (message.attachId().isEmpty()) {
        return false;
    }

    const auto attachments = message.fileSourcesAttachments();
    for (const auto &attachment : attachments) {
        // set DB file ID of 0, attachFileSources will replace this with the correct ID
        auto httpSources = transform(attachment.httpSources(), [](const auto &s) {
            return HttpSource{0, s.url()};
        });
        auto encryptedSources = transformFilter(attachment.encryptedSources(), std::bind(&MessageController::parseEncryptedSource, 0, _1));
        MessageDb::instance()
            ->attachFileSources(AccountManager::instance()->account().jid, chatJid, message.attachId(), attachment.id(), httpSources, encryptedSources);
    }
    return !attachments.empty();
}

std::optional<EncryptedSource> MessageController::parseEncryptedSource(qint64 fileId, const QXmppEncryptedFileSource &source)
{
    if (source.httpSources().empty()) {
        return {};
    }
    std::optional<qint64> encryptedDataId;
    if (!source.hashes().empty()) {
        encryptedDataId = MessageDb::instance()->newFileId();
    }
    return EncryptedSource{fileId,
                           source.httpSources().front().url(),
                           source.cipher(),
                           source.key(),
                           source.iv(),
                           encryptedDataId,
                           transform(source.hashes(), [&](const QXmppHash &h) {
                               return FileHash{*encryptedDataId, h.algorithm(), h.hash()};
                           })};
}

void MessageController::parseSharedFiles(const QXmppMessage &message, Message &messageToEdit)
{
    if (const auto sharedFiles = message.sharedFiles(); !sharedFiles.empty()) {
        messageToEdit.fileGroupId = MessageDb::instance()->newFileGroupId();
        messageToEdit.files = transform(sharedFiles, [message, fgid = messageToEdit.fileGroupId](const QXmppFileShare &fileShare) {
            auto fileId = MessageDb::instance()->newFileId();

            File file;

            file.id = fileId;
            file.fileGroupId = fgid.value();
            file.name = fileShare.metadata().filename();
            file.description = fileShare.metadata().description().value_or(QString());
            file.mimeType = fileShare.metadata().mediaType().value_or(QMimeDatabase().mimeTypeForName(QStringLiteral("application/octet-stream")));
            file.size = fileShare.metadata().size();
            file.lastModified = fileShare.metadata().lastModified().value_or(QDateTime());
            file.disposition = fileShare.disposition();
            file.externalId = fileShare.id();
            file.hashes = transform(fileShare.metadata().hashes(), [&](const QXmppHash &hash) {
                return FileHash{.dataId = fileId, .hashType = hash.algorithm(), .hashValue = hash.hash()};
            });
            file.thumbnail = [&]() {
                const auto &bobData = message.bitsOfBinaryData();
                if (!fileShare.metadata().thumbnails().empty()) {
                    auto cid = QXmppBitsOfBinaryContentId::fromCidUrl(fileShare.metadata().thumbnails().front().uri());
                    const auto thumbnailData = std::find_if(bobData.begin(), bobData.end(), [&](auto bobBlob) {
                        return bobBlob.cid() == cid;
                    });

                    if (thumbnailData != bobData.cend()) {
                        return thumbnailData->data();
                    }
                }
                return QByteArray();
            }();
            file.httpSources = transform(fileShare.httpSources(), [&](const auto &source) {
                return HttpSource{fileId, source.url()};
            });
            file.encryptedSources = transformFilter(fileShare.encryptedSources(), std::bind(MessageController::parseEncryptedSource, fileId, _1));

            return file;
        });
    } else if (auto urls = message.outOfBandUrls(); !urls.isEmpty()) {
        const qint64 fileGroupId = MessageDb::instance()->newFileGroupId();
        messageToEdit.files = transformFilter(urls, [&](auto &file) {
            return MessageController::parseOobUrl(file, fileGroupId);
        });

        // don't set file group id if there are no files
        if (!messageToEdit.files.empty()) {
            messageToEdit.fileGroupId = fileGroupId;
        }
    }
}

std::optional<File> MessageController::parseOobUrl(const QXmppOutOfBandUrl &url, qint64 fileGroupId)
{
    if (!MediaUtils::isHttp(url.url())) {
        return {};
    }

    // TODO: consider doing a HEAD request to fill in the remaining metadata

    const auto name = QUrl(url.url()).fileName();
    const auto id = MessageDb::instance()->newFileId();

    File file;
    file.id = id;
    file.fileGroupId = fileGroupId;
    file.name = name;
    file.description = url.description();
    file.mimeType = [&name] {
        const auto possibleMimeTypes = MediaUtils::mimeDatabase().mimeTypesForFileName(name);
        if (possibleMimeTypes.empty()) {
            return MediaUtils::mimeDatabase().mimeTypeForName(QStringLiteral("application/octet-stream"));
        }

        return possibleMimeTypes.front();
    }();
    file.httpSources = {HttpSource{.fileId = id, .url = QUrl(url.url())}};

    return file;
}

#include "moc_MessageController.cpp"
