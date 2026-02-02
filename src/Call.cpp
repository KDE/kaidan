// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Call.h"

// Qt
#include <QQuickItem>
#include <QQuickWindow>
// GStreamer
#include <gst/gst.h>
// QXmpp
#include <QXmppCall.h>
#include <QXmppCallManager.h>
#include <QXmppJingleMessageInitiationManager.h>
// Kaidan
#include "KaidanCoreLog.h"

constexpr QStringView AUDIO_MEDIA_TYPE = u"audio";
constexpr QStringView VIDEO_MEDIA_TYPE = u"video";

class VideoOutputRenderJob : public QRunnable
{
public:
    VideoOutputRenderJob(GstElement *);
    ~VideoOutputRenderJob();

    void run() override;

private:
    GstElement *m_pipeline;
};

VideoOutputRenderJob::VideoOutputRenderJob(GstElement *pipeline)
{
    m_pipeline = pipeline ? static_cast<GstElement *>(gst_object_ref(pipeline)) : NULL;
}

VideoOutputRenderJob::~VideoOutputRenderJob()
{
    if (m_pipeline) {
        gst_object_unref(m_pipeline);
    }
}

void VideoOutputRenderJob::run()
{
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    }
}

Call::Call(const QString &accountJid, const QString &chatJid, QXmppJingleMessageInitiationManager *jmiManager, QXmppCallManager *callManager, QObject *parent)
    : QObject(parent)
    , m_accountJid(accountJid)
    , m_chatJid(chatJid)
    , m_jmiManager(jmiManager)
    , m_callManager(callManager)
{
}

void Call::startAudioCall()
{
    addAudioMediaType();
    startCall();
}

void Call::startVideoCall()
{
    addAudioMediaType();
    addVideoMediaType();
    startCall();
}

void Call::handleCallProposed(const std::shared_ptr<QXmppJingleMessageInitiation> &jmi, const QString &id, const QList<QXmppJingleRtpDescription> &descriptions)
{
    Q_UNUSED(id)

    m_jmi = jmi;
    setUpHandlingJmiClosed();
    m_jmi->ring();

    m_descriptions = descriptions;
    Q_EMIT audioOnlyChanged();
}

bool Call::handleCallReceived(std::unique_ptr<QXmppCall> &call)
{
    if (m_proposedCallAccepted) {
        setCall(std::move(call));
        m_call->accept();
        setUpCall();
        return true;
    } else {
        setCall(std::move(call));
        return false;
    }
}

QString Call::accountJid() const
{
    return m_accountJid;
}

QString Call::chatJid() const
{
    return m_chatJid;
}

bool Call::audioOnly() const
{
    if (m_call) {
        return !m_call->videoStream();
    }

    return std::ranges::none_of(m_descriptions, [](const QXmppJingleRtpDescription &description) {
        return description.media() == VIDEO_MEDIA_TYPE;
    });
}

void Call::accept()
{
    if (m_jmi) {
        m_proposedCallAccepted = true;
        m_jmi->proceed();
    } else {
        Q_ASSERT(m_call);

        m_call->accept();
        setUpCall();
    }
}

void Call::reject()
{
    if (m_call) {
        m_call->decline();
    }

    if (m_jmi) {
        m_jmi->reject(QXmppJingleReason{QXmppJingleReason::Decline, {}, {}}).then(this, [this](QXmpp::SendResult &&) {
            terminate();
        });
    }
}

void Call::hangUp()
{
    if (m_call) {
        m_call->hangUp();

        if (m_jmi) {
            m_jmi->finish(QXmppJingleReason{QXmppJingleReason::Success, {}, {}}).then(this, [this](QXmpp::SendResult &&) {
                terminate();
            });
        }
    } else if (m_jmi) {
        m_jmi->retract(QXmppJingleReason{QXmppJingleReason::Cancel, {}, {}}).then(this, [this](QXmpp::SendResult &&) {
            terminate();
        });
    }
}

void Call::startCall()
{
    m_jmiManager->propose(m_chatJid, m_descriptions).then(this, [this](QXmppJingleMessageInitiationManager::ProposeResult &&result) {
        if (auto *error = std::get_if<QXmppError>(&result)) {
            qCDebug(KAIDAN_CORE_LOG) << "Could not propose call:" << error->description;
        } else {
            m_jmi = std::get<std::shared_ptr<QXmppJingleMessageInitiation>>(result);

            setUpHandlingJmiClosed();

            connect(m_jmi.get(), &QXmppJingleMessageInitiation::ringing, this, [this]() {
                qCDebug(KAIDAN_CORE_LOG) << "Ringing on" << m_chatJid;
            });

            connect(m_jmi.get(), &QXmppJingleMessageInitiation::proceeded, this, [this](const QString &id, const QString &remoteResource) {
                setCall(m_callManager->call(m_chatJid + u"/" + remoteResource,
                                            audioOnly() ? QXmppCallManager::Media::Audio : QXmppCallManager::Media::AudioVideo,
                                            id));
                setUpCall();
            });
        }
    });
}

void Call::setUpCall()
{
    // Use already existing streams.
    if (auto *audioStream = m_call->audioStream()) {
        setUpAudioStream(audioStream);
    }
    if (auto *videoStream = m_call->videoStream()) {
        setUpVideoStream(videoStream);
    }

    // Use newly created streams.
    connect(m_call, &QXmppCall::streamCreated, this, [this](QXmppCallStream *stream) {
        if (stream->media() == AUDIO_MEDIA_TYPE) {
            setUpAudioStream(stream);
        } else if (stream->media() == VIDEO_MEDIA_TYPE) {
            setUpVideoStream(stream);
        } else {
            qCDebug(KAIDAN_CORE_LOG) << "Unknown stream added to call";
        }
    });
}

void Call::addAudioMediaType()
{
    m_descriptions.append(QXmppJingleRtpDescription{AUDIO_MEDIA_TYPE.toString()});
}

void Call::addVideoMediaType()
{
    m_descriptions.append(QXmppJingleRtpDescription{VIDEO_MEDIA_TYPE.toString()});
    Q_EMIT audioOnlyChanged();
}

void Call::setUpAudioStream(QXmppCallStream *stream)
{
    Q_ASSERT(stream);
    Q_ASSERT(stream->media() == u"audio");
    Q_ASSERT(m_call);

    qCDebug(KAIDAN_CORE_LOG) << "Start audio stream setup";

    auto *pipeline = m_call->pipeline();

    // Output receiving audio.
    stream->setReceivePadCallback([pipeline](GstPad *receivePad) {
        GstElement *output = gst_parse_bin_from_description("audioresample ! audioconvert ! autoaudiosink", true, nullptr);

        if (!gst_bin_add(GST_BIN(pipeline), output)) {
            qCFatal(KAIDAN_CORE_LOG) << "Could not add audio playback to pipeline";
            return;
        }

        if (gst_pad_link(receivePad, gst_element_get_static_pad(output, "sink")) != GST_PAD_LINK_OK) {
            qCFatal(KAIDAN_CORE_LOG) << "Could not link receive pad to audio playback";
        }

        gst_element_sync_state_with_parent(output);

        qCDebug(KAIDAN_CORE_LOG) << "Audio playback (receive pad) set up";
    });

    // Record and send microphone input.
    stream->setSendPadCallback([pipeline](GstPad *sendPad) {
        GstElement *output = gst_parse_bin_from_description("autoaudiosrc ! audioconvert ! audioresample ! queue max-size-time=1000000", true, nullptr);

        if (!gst_bin_add(GST_BIN(pipeline), output)) {
            qCFatal(KAIDAN_CORE_LOG) << "Could not add audio recorder to pipeline";
            return;
        }

        if (gst_pad_link(gst_element_get_static_pad(output, "src"), sendPad) != GST_PAD_LINK_OK) {
            qCFatal(KAIDAN_CORE_LOG) << "Could not link audio recorder output to send pad";
        }

        gst_element_sync_state_with_parent(output);

        qCDebug(KAIDAN_CORE_LOG) << "Audio recorder (send pad) set up";
    });
}

void Call::setUpVideoStream(QXmppCallStream *stream)
{
    Q_ASSERT(stream);
    Q_ASSERT(stream->media() == u"video");
    Q_ASSERT(m_call);

    qCDebug(KAIDAN_CORE_LOG) << "Start video stream setup";

    Q_EMIT audioOnlyChanged();
    auto *pipeline = m_call->pipeline();

    stream->setReceivePadCallback([pipeline](GstPad *receivePad) {
        QQuickWindow *rootObject = static_cast<QQuickWindow *>(QGuiApplication::topLevelWindows().first());

        auto *contactVideoItem = rootObject->findChild<QQuickItem *>("callPageContactCameraArea");
        g_assert(contactVideoItem);

        GstElement *output = gst_parse_bin_from_description("videoconvert ! glupload ! qml6glsink name=contactVideoSink", true, nullptr);

        if (!gst_bin_add(GST_BIN(pipeline), output)) {
            qCFatal(KAIDAN_CORE_LOG) << "Could not add video playback to pipeline";
            return;
        }

        GstElement *contactVideoSink = gst_bin_get_by_name((GstBin *)pipeline, "contactVideoSink");
        g_assert(contactVideoSink);
        g_object_set(contactVideoSink, "widget", contactVideoItem, NULL);

        if (gst_pad_link(receivePad, gst_element_get_static_pad(output, "sink")) != GST_PAD_LINK_OK) {
            qCFatal(KAIDAN_CORE_LOG) << "Could not link receive pad to video playback";
        }

        gst_element_sync_state_with_parent(output);
        rootObject->scheduleRenderJob(new VideoOutputRenderJob(pipeline), QQuickWindow::BeforeSynchronizingStage);

        qCDebug(KAIDAN_CORE_LOG) << "Video playback (receive pad) set up";
    });

    stream->setSendPadCallback([pipeline](GstPad *sendPad) {
        GstElement *input = gst_parse_bin_from_description("v4l2src ! videoconvert", true, nullptr);

        if (!gst_bin_add(GST_BIN(pipeline), input)) {
            qCFatal(KAIDAN_CORE_LOG) << "Could not add video source to pipeline";
            return;
        }

        if (gst_pad_link(gst_element_get_static_pad(input, "src"), sendPad) != GST_PAD_LINK_OK) {
            qCFatal(KAIDAN_CORE_LOG) << "Could not link video source to send pad";
        }

        gst_element_sync_state_with_parent(input);

        qCDebug(KAIDAN_CORE_LOG) << "Video source (send pad) set up";
    });
}

void Call::setUpHandlingJmiClosed()
{
    connect(m_jmi.get(), &QXmppJingleMessageInitiation::closed, this, [this](const QXmppJingleMessageInitiation::Result &result) {
        if (auto *error = std::get_if<QXmppError>(&result)) {
            qCDebug(KAIDAN_CORE_LOG) << "Call closed:" << error->description;
        } else if (std::holds_alternative<QXmppJingleMessageInitiation::Rejected>(result)) {
            qCDebug(KAIDAN_CORE_LOG) << "Call rejected";
        } else if (std::holds_alternative<QXmppJingleMessageInitiation::Retracted>(result)) {
            qCDebug(KAIDAN_CORE_LOG) << "Call retracted";
        } else if (std::holds_alternative<QXmppJingleMessageInitiation::Finished>(result)) {
            qCDebug(KAIDAN_CORE_LOG) << "Call finished";
        }

        terminate();
    });
}

void Call::setCall(std::unique_ptr<QXmppCall> call)
{
    m_call = call.release();
    m_call->setParent(this);
    Q_EMIT audioOnlyChanged();

    connect(m_call, &QXmppCall::finished, [this]() {
        if (!m_jmi) {
            terminate();
        }
    });
}

void Call::terminate()
{
    qCDebug(KAIDAN_CORE_LOG) << "Call with" << m_chatJid << "ended";
    Q_EMIT quit();
}

#include "moc_Call.cpp"
