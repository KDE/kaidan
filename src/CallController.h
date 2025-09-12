// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QXmppJingleData.h>

typedef struct _GstElement GstElement;

class AccountSettings;
class QXmppCall;
class QXmppCallManager;
class QXmppCallStream;
class QXmppJingleMessageInitiation;
class QXmppJingleMessageInitiationManager;

static bool videoSourceFree = true;

class CallController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool callActive READ callActive NOTIFY callActiveChanged)

public:
    explicit CallController(AccountSettings *accountSettings,
                            QXmppCallManager *callManager,
                            QXmppJingleMessageInitiationManager *jmiManager,
                            QObject *parent = nullptr);

    Q_INVOKABLE bool callActive();
    Q_SIGNAL void callActiveChanged();

    Q_INVOKABLE void call(const QString &chatJid, bool audioOnly = true);
    Q_INVOKABLE void accept();
    Q_INVOKABLE void hangUp();

    Q_SIGNAL void callProposed(const QString &chatJid, bool audioOnly);
    Q_SIGNAL void callReceived(const QString &chatJid);

private:
    void setUpCall();

    void setUpAudioStream(GstElement *pipeline, QXmppCallStream *stream);
    void setUpVideoStream(GstElement *pipeline, QXmppCallStream *stream);

    void handleCallProposed(const std::shared_ptr<QXmppJingleMessageInitiation> &jmi,
                            const QString &id,
                            const std::optional<QXmppJingleRtpDescription> &description);
    void handleCallReceived(std::unique_ptr<QXmppCall> &call);

    void setActiveCall(std::unique_ptr<QXmppCall> call);
    void deleteActiveCall();

    AccountSettings *const m_accountSettings;
    QXmppCallManager *const m_callManager;
    QXmppJingleMessageInitiationManager *const m_jmiManager;

    QXmppCall *m_activeCall = nullptr;
    std::shared_ptr<QXmppJingleMessageInitiation> m_activeJmi;
};
