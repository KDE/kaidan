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

class KNotification;

class Notifications : public QObject
{
    Q_OBJECT

public:
    struct MessageNotificationWrapper {
        QString accountJid;
        QString chatJid;
        QDateTime initalTimestamp;
        QString latestMessageId;
        QList<QString> messages;
        bool isDeletionEnabled = true;
        KNotification *notification = nullptr;
    };

    struct PresenceSubscriptionRequestNotificationWrapper {
        QString accountJid;
        QString chatJid;
        KNotification *notification = nullptr;
    };

    static Notifications *instance();

    explicit Notifications(QObject *parent = nullptr);

    /**
     * Sends a system notification for a chat message.
     *
     * @param accountJid JID of the message's account
     * @param chatJid JID of the message's chat
     * @param messageId ID of the message
     * @param messageBody body of the message to display as the notification's text
     */
    void sendMessageNotification(const QString &accountJid, const QString &chatJid, const QString &messageId, const QString &messageBody);

    /**
     * Closes all chat message notifications of the same age or older than a timestamp.
     *
     * @param accountJid JID of the message's account
     * @param chatJid JID of the message's chat
     */
    void closeMessageNotification(const QString &accountJid, const QString &chatJid);

    void sendPresenceSubscriptionRequestNotification(const QString &accountJid, const QString &chatJid);
    void closePresenceSubscriptionRequestNotification(const QString &accountJid, const QString &chatJid);

private:
    QString determineChatName(const QString &chatJid) const;
    void showChat(const QString &accountJid, const QString &chatJid);

    QList<MessageNotificationWrapper> m_openMessageNotifications;
    QList<PresenceSubscriptionRequestNotificationWrapper> m_openPresenceSubscriptionRequestNotifications;

    static Notifications *s_instance;
};
