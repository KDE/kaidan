// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>

// QXmpp
class QXmppCall;
class QXmppCallManager;
class QXmppCallStream;
class QXmppJingleMessageInitiation;
class QXmppJingleMessageInitiationManager;
class QXmppJingleRtpDescription;

typedef struct _GstElement GstElement;

class Call : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString accountJid READ accountJid CONSTANT)
    Q_PROPERTY(QString chatJid READ chatJid CONSTANT)
    Q_PROPERTY(bool audioOnly READ audioOnly NOTIFY audioOnlyChanged)
    Q_PROPERTY(bool videoPlaybackActive MEMBER m_videoPlaybackActive NOTIFY videoPlaybackActiveChanged)

public:
    Call(const QString &accountJid,
         const QString &chatJid,
         QXmppJingleMessageInitiationManager *jmiManager,
         QXmppCallManager *callManager,
         QObject *parent = nullptr);

    void startAudioCall();
    void startVideoCall();

    void handleCallProposed(const std::shared_ptr<QXmppJingleMessageInitiation> &jmi, const QString &id, const QList<QXmppJingleRtpDescription> &descriptions);
    bool handleCallReceived(std::unique_ptr<QXmppCall> &call);

    QString accountJid() const;
    QString chatJid() const;

    bool audioOnly() const;
    Q_SIGNAL void audioOnlyChanged();

    Q_SIGNAL void videoPlaybackActiveChanged();

    void accept();
    void reject();
    Q_INVOKABLE void hangUp();

    Q_SIGNAL void quit();

private:
    void startCall();
    void setUpCall();

    void addAudioMediaType();
    void addVideoMediaType();

    void setUpAudioStream(QXmppCallStream *stream);
    void setUpVideoStream(QXmppCallStream *stream);

    void setUpHandlingJmiClosed();
    void setCall(std::unique_ptr<QXmppCall> call);

    void terminate();

    const QString m_accountJid;
    const QString m_chatJid;

    QXmppJingleMessageInitiationManager *const m_jmiManager;
    QXmppCallManager *const m_callManager;

    std::shared_ptr<QXmppJingleMessageInitiation> m_jmi;
    QList<QXmppJingleRtpDescription> m_descriptions;
    QXmppCall *m_call = nullptr;
    std::function<void()> m_jmiFinishTask;

    bool m_proposedCallAccepted = false;
    bool m_videoPlaybackActive = false;
};
