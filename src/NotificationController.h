// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QDateTime>
#include <QObject>
#include <QUuid>
// Kaidan
#include "Message.h"

class AccountSettings;
class Call;
class ChatController;
class MessageController;
class KNotification;

class NotificationController : public QObject
{
    Q_OBJECT

public:
    struct MessageNotificationWrapper {
        QString chatJid;
        QDateTime initalTimestamp;
        QString latestMessageId;
        QList<QString> messages;
        bool isDeletionEnabled = true;
        KNotification *notification = nullptr;
    };

    struct PresenceSubscriptionRequestNotificationWrapper {
        QString chatJid;
        KNotification *notification = nullptr;
    };

    NotificationController(AccountSettings *accountSettings, MessageController *messageController, QObject *parent = nullptr);

    /**
     * Shows a notification for a message if needed.
     *
     * @param message message for whom a notification may be shown
     */
    void handleMessage(const Message &message, MessageOrigin origin);

    /**
     * Closes all chat message notifications of the same age or older than a timestamp.
     *
     * @param chatJid JID of the message's chat
     */
    void closeMessageNotification(const QString &chatJid);

    void sendPresenceSubscriptionRequestNotification(const QString &chatJid);
    void closePresenceSubscriptionRequestNotification(const QString &chatJid);

    void sendCallNotification(Call *call);

    void setChatController(ChatController *chatController);

private:
    /**
     * Sends a system notification for a chat message.
     *
     * @param chatJid JID of the message's chat
     * @param messageId ID of the message
     * @param messageBody body of the message to display as the notification's text
     */
    void sendMessageNotification(const QString &chatJid, const QString &messageId, const QString &messageBody);

    QString determineChatName(const QString &chatJid) const;
    void showChat(const QString &chatJid);

    AccountSettings *const m_accountSettings;
    ChatController *m_chatController = nullptr;
    MessageController *const m_messageController;

    QList<MessageNotificationWrapper> m_openMessageNotifications;
    QList<PresenceSubscriptionRequestNotificationWrapper> m_openPresenceSubscriptionRequestNotifications;
};
