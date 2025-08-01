// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>

typedef struct _GstElement GstElement;

class QXmppCall;
class QXmppCallManager;
class QXmppCallStream;
class QXmppJingleMessageInitiation;
class QXmppJingleMessageInitiationManager;

class CallController : public QObject
{
    Q_OBJECT

public:
    explicit CallController(QXmppCallManager *callManager, QXmppJingleMessageInitiationManager *jmiManager, QObject *parent = nullptr);

    Q_INVOKABLE void call(const QString &chatJid, bool audioOnly = true);
    Q_INVOKABLE void accept();
    Q_INVOKABLE void hangUp();

    Q_SIGNAL void callReceived(const QString &chatJid);

private:
    void setUpCall();

    void setUpAudioStream(GstElement *pipeline, QXmppCallStream *stream);
    void setUpVideoStream(GstElement *pipeline, QXmppCallStream *stream);

    void handleCallReceived(std::unique_ptr<QXmppCall> &call);

    QXmppCallManager *const m_callManager;
    QXmppJingleMessageInitiationManager *const m_jmiManager;

    QXmppCall *m_activeCall = nullptr;
    std::shared_ptr<QXmppJingleMessageInitiation> m_activeJmi;
};
