// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NotificationController.h"

// Qt
#include <QGuiApplication>
// KDE
#ifdef HAVE_KNOTIFICATIONS
#include <KNotification>
#endif
// Kaidan
#include "Account.h"
#include "ChatController.h"
#include "GroupChatUserDb.h"
#include "MainController.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "RosterDb.h"
#include "RosterModel.h"

// Q_OS_BSD4 includes all BSD variants and also Q_OS_APPLE
// Q_OS_LINUX is also defined on Android
#if (defined(Q_OS_LINUX) || defined(Q_OS_BSD4) || defined(Q_OS_HURD)) && !defined(Q_OS_ANDROID) && !defined(Q_OS_APPLE)
#define DESKTOP_LINUX_ALIKE_OS
#endif

using namespace std::chrono_literals;

// Event IDs corresponding to the section entries in the "kaidan.notifyrc" configuration file
constexpr QStringView NEW_MESSAGE_EVENT_ID = u"new-message";
constexpr QStringView NEW_SUBSEQUENT_MESSAGE_EVENT_ID = u"new-subsequent-message";
constexpr QStringView PRESENCE_SUBSCRIPTION_REQUEST_EVENT_ID = u"presence-subscription-request";

constexpr auto SUBSEQUENT_MESSAGE_INTERVAL = 5s;
constexpr int MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT = 6;

NotificationController::NotificationController(AccountSettings *accountSettings, MessageController *messageController, QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_messageController(messageController)
{
    connect(MessageDb::instance(), &MessageDb::messageAdded, this, &NotificationController::handleMessage);
    connect(MessageDb::instance(), &MessageDb::messageUpdated, this, [this](const Message &message) {
        handleMessage(message, MessageOrigin::Stream);
    });

    connect(m_messageController, &MessageController::contactMessageRead, this, &NotificationController::closeMessageNotification);
}

#ifdef HAVE_KNOTIFICATIONS
void NotificationController::handleMessage(const Message &message, MessageOrigin origin)
{
    // Send a notification in the following cases:
    // * The message was not sent by the user from another resource and received via Message Carbons.
    // * Notifications are allowed according to the set rule.
    // * The corresponding chat is not opened while the application window is active.

    const auto accountJid = m_accountSettings->jid();

    if (message.accountJid != accountJid) {
        return;
    }

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

    if (message.isOwn) {
        // Close a notification for messages to which the user replied via another own resource.
        closeMessageNotification(message.chatJid);
    } else {
        const auto chatJid = message.chatJid;
        const auto rosterItem = RosterModel::instance()->item(accountJid, chatJid);

        auto sendNotification = [this, accountJid, chatJid, message]() {
            const auto chatActive = m_chatController && m_chatController->jid() == chatJid && QGuiApplication::applicationState() == Qt::ApplicationActive;
            const auto previewText = message.previewText();
            const auto notificationBody = message.isGroupChatMessage() ? message.groupChatSenderName + QStringLiteral(": ") + previewText : previewText;

            if (!chatActive) {
                sendMessageNotification(chatJid, message.id, notificationBody);
            }
        };

        auto sendNotificationOnMention = [this, accountJid, chatJid, rosterItem, message, sendNotification]() {
            GroupChatUserDb::instance()
                ->user(accountJid, chatJid, rosterItem->groupChatParticipantId)
                .then(this, [body = message.body(), sendNotification](const std::optional<GroupChatUser> user) {
                    if (user && (body.contains(user->id) || body.contains(user->jid) || body.contains(user->name))) {
                        sendNotification();
                    }
                });
        };

        switch (rosterItem->effectiveNotificationRule()) {
        case RosterItem::EffectiveNotificationRule::Never:
            break;
        case RosterItem::EffectiveNotificationRule::Mentioned:
            sendNotificationOnMention();
            break;
        case RosterItem::EffectiveNotificationRule::Always:
            sendNotification();
            break;
        }
    }
}

void NotificationController::closeMessageNotification(const QString &chatJid)
{
    const auto notificationWrapperItr =
        std::find_if(m_openMessageNotifications.cbegin(), m_openMessageNotifications.cend(), [chatJid](const MessageNotificationWrapper &notificationWrapper) {
            return notificationWrapper.chatJid == chatJid;
        });

    if (notificationWrapperItr != m_openMessageNotifications.cend()) {
        notificationWrapperItr->notification->close();
    }
}

void NotificationController::sendPresenceSubscriptionRequestNotification(const QString &chatJid)
{
#ifdef DESKTOP_LINUX_ALIKE_OS
    static bool IS_USING_GNOME = qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QStringLiteral("GNOME"), Qt::CaseInsensitive);
#endif

    auto notificationWrapperItr =
        std::find_if(m_openMessageNotifications.begin(), m_openMessageNotifications.end(), [&chatJid](const auto &notificationWrapper) {
            return notificationWrapper.chatJid == chatJid;
        });

    // Only create a new notification if none exists.
    if (notificationWrapperItr != m_openMessageNotifications.end()) {
        return;
    }

    KNotification *notification = new KNotification(PRESENCE_SUBSCRIPTION_REQUEST_EVENT_ID.toString());
    notification->setTitle(determineChatName(chatJid));
    notification->setText(tr("Requests to receive your personal data"));

    PresenceSubscriptionRequestNotificationWrapper notificationWrapper{.chatJid = chatJid, .notification = notification};
    m_openPresenceSubscriptionRequestNotifications.append(notificationWrapper);

#ifdef DESKTOP_LINUX_ALIKE_OS
    if (IS_USING_GNOME) {
        notification->setFlags(KNotification::Persistent);
    }
#endif
#ifdef Q_OS_ANDROID
    notification->setIconName("kaidan-bw");
#endif

    connect(notification->addDefaultAction(tr("Open")), &KNotificationAction::activated, this, [this, chatJid, notification] {
        showChat(chatJid);
        notification->close();
    });

    connect(notification, &KNotification::closed, this, [=, this]() {
        auto notificationWrapperItr = std::find_if(m_openPresenceSubscriptionRequestNotifications.begin(),
                                                   m_openPresenceSubscriptionRequestNotifications.end(),
                                                   [chatJid](const PresenceSubscriptionRequestNotificationWrapper &notificationWrapper) {
                                                       return notificationWrapper.chatJid == chatJid;
                                                   });

        if (notificationWrapperItr != m_openPresenceSubscriptionRequestNotifications.end()) {
            m_openPresenceSubscriptionRequestNotifications.erase(notificationWrapperItr);
        }
    });

    notification->sendEvent();
}

void NotificationController::closePresenceSubscriptionRequestNotification(const QString &chatJid)
{
    const auto notificationWrapperItr = std::find_if(m_openPresenceSubscriptionRequestNotifications.cbegin(),
                                                     m_openPresenceSubscriptionRequestNotifications.cend(),
                                                     [chatJid](const PresenceSubscriptionRequestNotificationWrapper &notificationWrapper) {
                                                         return notificationWrapper.chatJid == chatJid;
                                                     });

    if (notificationWrapperItr != m_openPresenceSubscriptionRequestNotifications.cend()) {
        notificationWrapperItr->notification->close();
    }
}

void NotificationController::setChatController(ChatController *chatController)
{
    m_chatController = chatController;
}

void NotificationController::sendMessageNotification(const QString &chatJid, const QString &messageId, const QString &messageBody)
{
#ifdef DESKTOP_LINUX_ALIKE_OS
    static bool IS_USING_GNOME = qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QStringLiteral("GNOME"), Qt::CaseInsensitive);
#endif

    KNotification *notification = nullptr;

    auto notificationWrapperItr =
        std::find_if(m_openMessageNotifications.begin(), m_openMessageNotifications.end(), [&chatJid](const auto &notificationWrapper) {
            return notificationWrapper.chatJid == chatJid;
        });

    // Update an existing notification or create a new one.
    if (notificationWrapperItr != m_openMessageNotifications.end()) {
        auto &messages = notificationWrapperItr->messages;
        messages.append(messageBody);

        // Initialize variables by known values.
        notificationWrapperItr->latestMessageId = messageId;

        QList<QString> notificationTextLines;

        // Append the message's body to the text of existing notifications.
        // If the text of the existing notifications and messageBody contain together more than
        // MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT of lines, keep only the last
        // MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT - 1 of them and replace the first one by an
        // ellipse.
        //
        // The loop exits in the following cases:
        // 1. The message of the current iteration has lines that would result in more
        // lines than MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT when prepended to
        // notificationTextLines.
        // 2. notificationTextLines would have more lines than
        // MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT when the next message was being prepended.
        for (auto messageItr = messages.end() - 1; messageItr != messages.begin() - 1; --messageItr) {
            auto messageNotificationTextLines = (*messageItr).split(u'\n');
            const auto overflowingMessageLineCount = messageNotificationTextLines.size() + notificationTextLines.size() - MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT;

            if (overflowingMessageLineCount > 0) {
                messageNotificationTextLines = messageNotificationTextLines.mid(overflowingMessageLineCount + 1);
                messageNotificationTextLines.prepend(QStringLiteral("…"));

                *messageItr = messageNotificationTextLines.join(u'\n');
                messages.erase(messages.begin(), messageItr);

                notificationTextLines = messageNotificationTextLines << notificationTextLines;
                break;
            } else {
                notificationTextLines = messageNotificationTextLines << notificationTextLines;

                if (notificationTextLines.size() == MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT && messageItr - 1 != messages.begin() - 1) {
                    notificationTextLines[0] = QStringLiteral("…");
                    messages.erase(messages.begin(), messageItr);
                    break;
                }
            }
        }

        // Do not disturb the user when messages are received in quick succession by only playing a
        // notification sound for the first message.
        const auto initalTimestamp = notificationWrapperItr->initalTimestamp.toSecsSinceEpoch() * 1s;
        const auto currentTimestamp = QDateTime::currentSecsSinceEpoch() * 1s;
        if (currentTimestamp - initalTimestamp > SUBSEQUENT_MESSAGE_INTERVAL) {
            notification = new KNotification(NEW_MESSAGE_EVENT_ID.toString());
        } else {
            notification = new KNotification(NEW_SUBSEQUENT_MESSAGE_EVENT_ID.toString());
        }

        notification->setText(notificationTextLines.join(u'\n'));

        notificationWrapperItr->isDeletionEnabled = false;
        notificationWrapperItr->notification->close();
        notificationWrapperItr->notification = notification;
    } else {
        notification = new KNotification(NEW_MESSAGE_EVENT_ID.toString());
        notification->setText(messageBody);

        MessageNotificationWrapper notificationWrapper{.chatJid = chatJid,
                                                       .initalTimestamp = QDateTime::currentDateTimeUtc(),
                                                       .latestMessageId = messageId,
                                                       .messages = {messageBody},
                                                       .notification = notification};
        m_openMessageNotifications.append(notificationWrapper);
    }

    notification->setTitle(determineChatName(chatJid));

#ifdef DESKTOP_LINUX_ALIKE_OS
    if (IS_USING_GNOME) {
        notification->setFlags(KNotification::Persistent);
    }
#endif
#ifdef Q_OS_ANDROID
    notification->setIconName("kaidan-bw");
#endif

    connect(notification->addDefaultAction(tr("Open")), &KNotificationAction::activated, this, [this, chatJid] {
        showChat(chatJid);
    });

    connect(notification->addAction(tr("Mark as read")), &KNotificationAction::activated, this, [this, chatJid, messageId] {
        RosterDb::instance()->updateItem(m_accountSettings->jid(), chatJid, [messageId](RosterItem &item) {
            item.lastReadContactMessageId = messageId;
            item.unreadMessages = 0;
        });

        if (const auto item = RosterModel::instance()->item(m_accountSettings->jid(), chatJid); item && item->readMarkerSendingEnabled) {
            m_messageController->sendReadMarker(chatJid, messageId);
        }
    });

    connect(notification, &KNotification::closed, this, [=, this]() {
        auto notificationWrapperItr = std::find_if(m_openMessageNotifications.begin(),
                                                   m_openMessageNotifications.end(),
                                                   [chatJid](const MessageNotificationWrapper &notificationWrapper) {
                                                       return notificationWrapper.chatJid == chatJid;
                                                   });

        if (notificationWrapperItr != m_openMessageNotifications.end()) {
            if (notificationWrapperItr->isDeletionEnabled) {
                m_openMessageNotifications.erase(notificationWrapperItr);
            } else {
                notificationWrapperItr->isDeletionEnabled = true;
            }
        }
    });

    notification->sendEvent();
}

QString NotificationController::determineChatName(const QString &chatJid) const
{
    auto rosterItem = RosterModel::instance()->item(m_accountSettings->jid(), chatJid);
    return rosterItem ? rosterItem->displayName() : chatJid;
}

void NotificationController::showChat(const QString &chatJid)
{
    if (!m_chatController || m_chatController->jid() != chatJid) {
        Q_EMIT MainController::instance()->openChatPageRequested(m_accountSettings->jid(), chatJid);
    }

    Q_EMIT MainController::instance()->raiseWindowRequested();
}
#else
void Notifications::showMessageNotification(const Message &message, MessageOrigin origin)
{
}

void Notifications::closeMessageNotification(const QString &, const QString &)
{
}

void Notifications::sendPresenceSubscriptionRequestNotification(const QString &accountJid, const QString &chatJid)
{
}

void Notifications::closePresenceSubscriptionRequestNotification(const QString &accountJid, const QString &chatJid)
{
}

void NotificationController::setChatController(ChatController *chatController)
{
}

void Notifications::sendMessageNotification(const QString &, const QString &, const QString &, const QString &)
{
}

QString NotificationController::determineChatName(const QString &chatJid) const
{
}

void NotificationController::showChat(const QString &chatJid)
{
}
#endif // HAVE_KNOTIFICATIONS

#include "moc_NotificationController.cpp"
