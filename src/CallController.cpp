// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CallController.h"

// QXmpp
#include <QXmppCall.h>
#include <QXmppCallManager.h>
#include <QXmppJingleMessageInitiationManager.h>
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "Call.h"
#include "MainController.h"
#include "NotificationController.h"

CallController::CallController(AccountSettings *accountSettings,
                               Connection *connection,
                               NotificationController *notificationController,
                               QXmppJingleMessageInitiationManager *jmiManager,
                               QXmppCallManager *callManager,
                               QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_connection(connection)
    , m_notificationController(notificationController)
    , m_jmiManager(jmiManager)
    , m_callManager(callManager)
{
    connect(m_jmiManager, &QXmppJingleMessageInitiationManager::proposeReceived, this, &CallController::handleCallProposed);
    connect(m_callManager, &QXmppCallManager::callReceived, this, &CallController::handleCallReceived);
}

void CallController::startAudioCall(const QString &chatJid)
{
    auto *call = addCall(chatJid);
    MainController::instance()->setActiveCall(call);
    call->startAudioCall();
}

void CallController::startVideoCall(const QString &chatJid)
{
    auto *call = addCall(chatJid);
    MainController::instance()->setActiveCall(call);
    call->startVideoCall();
}

void CallController::handleCallProposed(const std::shared_ptr<QXmppJingleMessageInitiation> &jmi,
                                        const QString &id,
                                        const QList<QXmppJingleRtpDescription> &descriptions)
{
    if (const auto chatJid = QXmppUtils::jidToBareJid(jmi->remoteJid()); chatJid != m_accountSettings->jid()) {
        auto *call = addCall(chatJid);
        call->handleCallProposed(jmi, id, descriptions);
        m_notificationController->sendCallNotification(call);
    }
}

void CallController::handleCallReceived(std::unique_ptr<QXmppCall> &call)
{
    if (const auto chatJid = QXmppUtils::jidToBareJid(call->jid()); chatJid != m_accountSettings->jid()) {
        if (auto *usedCall = findCall(chatJid)) {
            if (!usedCall->handleCallReceived(call)) {
                m_notificationController->sendCallNotification(usedCall);
            }
        } else {
            usedCall = addCall(chatJid);
            usedCall->handleCallReceived(call);
            m_notificationController->sendCallNotification(usedCall);
        }
    }
}

Call *CallController::addCall(const QString &chatJid)
{
    auto *call = new Call(m_accountSettings->jid(), chatJid, m_jmiManager, m_callManager, this);

    connect(call, &Call::quit, this, [this, call]() {
        removeQuitCall(call);
    });

    CallWrapper callWrapper;
    callWrapper.call = call;
    callWrapper.promise = m_connection->addLogoutTask([this, call]() {
        removeCallOnLogout(call);
    });

    m_callWrappers.append(callWrapper);

    return call;
}

void CallController::removeQuitCall(Call *call)
{
    if (MainController::instance()->activeCall() == call) {
        MainController::instance()->setActiveCall(nullptr);
    }

    call->deleteLater();

    auto itr = std::ranges::find_if(m_callWrappers, [call](const CallWrapper &callWrapper) {
        return callWrapper.call == call;
    });

    if (itr != m_callWrappers.end()) {
        if (itr->promise->future().isStarted()) {
            itr->promise->finish();
        } else {
            m_connection->removeLogoutTask(itr->promise);
        }

        m_callWrappers.erase(itr);
    }
}

void CallController::removeCallOnLogout(Call *call)
{
    if (MainController::instance()->activeCall() == call) {
        call->hangUp();
    } else {
        call->deleteLater();

        auto itr = std::ranges::find_if(m_callWrappers, [call](const CallWrapper &callWrapper) {
            return callWrapper.call == call;
        });

        if (itr != m_callWrappers.end()) {
            itr->promise->finish();
            m_callWrappers.erase(itr);
        }
    }
}

Call *CallController::findCall(const QString &chatJid)
{
    const auto itr = std::ranges::find_if(m_callWrappers, [chatJid](const CallWrapper &callWrapper) {
        return callWrapper.call->chatJid() == chatJid;
    });

    if (itr == m_callWrappers.end()) {
        return nullptr;
    }

    return itr->call;
}

#include "moc_CallController.cpp"
