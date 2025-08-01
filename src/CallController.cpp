// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CallController.h"

// GStreamer
#include <gst/gst.h>
// QXmpp
#include <QXmppCall.h>
#include <QXmppCallManager.h>
#include <QXmppJingleMessageInitiationManager.h>
#include <QXmppUtils.h>
// Kaidan
#include <KaidanCoreLog.h>

using namespace Qt::Literals;

CallController::CallController(QXmppCallManager *callManager, QXmppJingleMessageInitiationManager *jmiManager, QObject *parent)
    : QObject(parent)
    , m_callManager(callManager)
    , m_jmiManager(jmiManager)
{
    connect(m_callManager, &QXmppCallManager::callReceived, this, &CallController::handleCallReceived);
}

void CallController::call(const QString &chatJid, bool audioOnly)
{
    // TODO: Use QXmppJingleMessageInitiationManager to let the callee accept the call via a specific resource.

    QXmppJingleRtpDescription description;
    description.setMedia(audioOnly ? u"audio"_s : u"video"_s);

    m_jmiManager->propose(chatJid, description).then(this, [this, chatJid](QXmppJingleMessageInitiationManager::ProposeResult &&result) {
        if (auto *error = std::get_if<QXmppError>(&result)) {
            qCDebug(KAIDAN_CORE_LOG) << "Could not propose a call:" << error->description;
        } else {
            m_activeJmi = std::get<std::shared_ptr<QXmppJingleMessageInitiation>>(result);

            connect(m_activeJmi.get(), &QXmppJingleMessageInitiation::closed, this, [chatJid](const QXmppJingleMessageInitiation::Result &result) {
                if (auto *error = std::get_if<QXmppError>(&result)) {
                    qCDebug(KAIDAN_CORE_LOG) << "The proposed call is closed:" << error->description;
                } else if (std::holds_alternative<QXmppJingleMessageInitiation::Rejected>(result)) {
                    qCDebug(KAIDAN_CORE_LOG) << "The proposed call is rejected";
                } else if (std::holds_alternative<QXmppJingleMessageInitiation::Retracted>(result)) {
                    qCDebug(KAIDAN_CORE_LOG) << "The proposed call is retracted";
                } else if (std::holds_alternative<QXmppJingleMessageInitiation::Finished>(result)) {
                    qCDebug(KAIDAN_CORE_LOG) << "The proposed call is finished";
                }
            });

            connect(m_activeJmi.get(), &QXmppJingleMessageInitiation::proceeded, this, [this, chatJid](const QString &, const QString &callPartnerResource) {
                m_activeCall = m_callManager->call(chatJid + u"/" + callPartnerResource).release();

                setUpCall();

                connect(m_activeCall, &QXmppCall::ringing, [this]() {
                    qCDebug(KAIDAN_CORE_LOG) << "[Call] Ringing on" << m_activeCall->jid() << "until they accept";
                });
            });
        }
    });
}

void CallController::accept()
{
    m_activeCall->accept();
}

void CallController::hangUp()
{
    if (m_activeCall) {
        m_activeCall->hangup();
    } else {
        m_activeJmi->retract({});
    }
}

void CallController::setUpCall()
{
    // Use possibly already existing streams.
    if (auto *audioStream = m_activeCall->audioStream()) {
        setUpAudioStream(m_activeCall->pipeline(), audioStream);
    }
    if (auto *videoStream = m_activeCall->videoStream()) {
        setUpVideoStream(m_activeCall->pipeline(), videoStream);
    }

    // Use newly created streams.
    connect(m_activeCall, &QXmppCall::streamCreated, this, [this](QXmppCallStream *stream) {
        if (stream->media() == u"audio") {
            setUpAudioStream(m_activeCall->pipeline(), stream);
        } else if (stream->media() == u"video") {
            setUpVideoStream(m_activeCall->pipeline(), stream);
        } else {
            qCDebug(KAIDAN_CORE_LOG) << "[AVCall]" << "Unknown stream added to call";
        }
    });

    connect(m_activeCall, &QXmppCall::finished, [this]() {
        qCDebug(KAIDAN_CORE_LOG) << "[Call] Call with" << m_activeCall->jid() << "ended. (Deleting)";
        m_activeCall->deleteLater();
    });

    connect(m_activeCall, &QXmppCall::connected, this, [this]() {
        qCDebug(KAIDAN_CORE_LOG) << "[Call] Call to" << m_activeCall->jid() << "connected!";

        if (m_activeCall->videoSupported()) {
            m_activeCall->addVideo();
        }
    });
}

void CallController::setUpAudioStream(GstElement *pipeline, QXmppCallStream *stream)
{
    Q_ASSERT(stream);
    Q_ASSERT(stream->media() == u"audio");

    qCDebug(KAIDAN_CORE_LOG) << "[AVCall] Begin audio stream setup";
    // output receiving audio
    stream->setReceivePadCallback([pipeline](GstPad *receivePad) {
        GstElement *output = gst_parse_bin_from_description("audioresample ! audioconvert ! autoaudiosink", true, nullptr);
        if (!gst_bin_add(GST_BIN(pipeline), output)) {
            qFatal("[AVCall] Failed to add audio playback to pipeline");
            return;
        }

        if (gst_pad_link(receivePad, gst_element_get_static_pad(output, "sink")) != GST_PAD_LINK_OK) {
            qFatal("[AVCall] Failed to link receive pad to audio playback.");
        }
        gst_element_sync_state_with_parent(output);

        qCDebug(KAIDAN_CORE_LOG) << "[AVCall] Audio playback (receive pad) set up.";
    });

    // record and send microphone
    stream->setSendPadCallback([pipeline](GstPad *sendPad) {
        GstElement *output = gst_parse_bin_from_description("autoaudiosrc ! audioconvert ! audioresample ! queue max-size-time=1000000", true, nullptr);
        if (!gst_bin_add(GST_BIN(pipeline), output)) {
            qFatal("[AVCall] Failed to add audio recorder to pipeline");
            return;
        }

        if (gst_pad_link(gst_element_get_static_pad(output, "src"), sendPad) != GST_PAD_LINK_OK) {
            qFatal("[AVCall] Failed to link audio recorder output to send pad.");
        }
        gst_element_sync_state_with_parent(output);

        qCDebug(KAIDAN_CORE_LOG) << "[AVCall] Audio recorder (send pad) set up.";
    });
}

void CallController::setUpVideoStream(GstElement *pipeline, QXmppCallStream *stream)
{
    Q_ASSERT(stream);
    Q_ASSERT(stream->media() == u"video");

    qCDebug(KAIDAN_CORE_LOG) << "[AVCall] Begin video stream setup";
    stream->setReceivePadCallback([pipeline](GstPad *receivePad) {
        GstElement *output = gst_parse_bin_from_description("autovideosink", true, nullptr);
        if (!gst_bin_add(GST_BIN(pipeline), output)) {
            qFatal("[AVCall] Failed to add video playback to pipeline");
            return;
        }

        if (gst_pad_link(receivePad, gst_element_get_static_pad(output, "sink")) != GST_PAD_LINK_OK) {
            qFatal("[AVCall] Failed to link receive pad to video playback.");
        }
        gst_element_sync_state_with_parent(output);

        qCDebug(KAIDAN_CORE_LOG) << "[AVCall] Video playback (receive pad) set up.";
    });
    stream->setSendPadCallback([pipeline](GstPad *sendPad) {
        GstElement *output = gst_parse_bin_from_description("videotestsrc", true, nullptr);
        if (!gst_bin_add(GST_BIN(pipeline), output)) {
            qFatal("[AVCall] Failed to add video test source to pipeline");
            return;
        }

        if (gst_pad_link(gst_element_get_static_pad(output, "src"), sendPad) != GST_PAD_LINK_OK) {
            qFatal("[AVCall] Failed to link video test source to send pad.");
        }
        gst_element_sync_state_with_parent(output);

        qCDebug(KAIDAN_CORE_LOG) << "[AVCall] Video test source (send pad) set up.";
    });
}

void CallController::handleCallReceived(std::unique_ptr<QXmppCall> &call)
{
    auto handledCall = call.release();

    connect(handledCall, &QXmppCall::finished, this, [handledCall]() {
        qCDebug(KAIDAN_CORE_LOG) << "[Call] Call with" << handledCall->jid() << "ended. (Deleting)";
        handledCall->deleteLater();
    });

    Q_EMIT callReceived(QXmppUtils::jidToBareJid(call->jid()));
}

#include "moc_CallController.cpp"
