// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QPromise>

// QXmpp
class QXmppCall;
class QXmppCallManager;
class QXmppJingleMessageInitiation;
class QXmppJingleMessageInitiationManager;
class QXmppJingleRtpDescription;
// Kaidan
class AccountSettings;
class Call;
class Connection;
class NotificationController;

class CallController : public QObject
{
    Q_OBJECT

public:
    struct CallWrapper {
        Call *call;
        std::shared_ptr<QPromise<void>> promise;
    };

    CallController(AccountSettings *accountSettings,
                   Connection *connection,
                   NotificationController *notificationController,
                   QXmppJingleMessageInitiationManager *jmiManager,
                   QXmppCallManager *callManager,
                   QObject *parent = nullptr);

    Q_INVOKABLE void startAudioCall(const QString &chatJid);
    Q_INVOKABLE void startVideoCall(const QString &chatJid);

private:
    void handleCallProposed(const std::shared_ptr<QXmppJingleMessageInitiation> &jmi, const QString &id, const QList<QXmppJingleRtpDescription> &descriptions);
    void handleCallReceived(std::unique_ptr<QXmppCall> &call);

    Call *addCall(const QString &chatJid);
    void removeQuitCall(Call *call);
    void removeCallOnLogOut(Call *call);
    Call *findCall(const QString &chatJid);

    AccountSettings *const m_accountSettings;
    Connection *const m_connection;
    NotificationController *const m_notificationController;

    QXmppJingleMessageInitiationManager *const m_jmiManager;
    QXmppCallManager *const m_callManager;

    QList<CallWrapper> m_callWrappers;
};
