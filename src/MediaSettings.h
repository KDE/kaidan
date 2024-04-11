// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCoreApplication>
#include <QImageCapture>
#include <QMediaFormat>
#include <QMediaRecorder>

#include "AudioDeviceModel.h"
#include "CameraModel.h"

class QSettings;

class MediaSettings
{
    Q_GADGET

    Q_PROPERTY(CameraInfo camera MEMBER camera)
    Q_PROPERTY(AudioDeviceInfo audioInputDevice MEMBER audioInputDevice)
    Q_PROPERTY(QString container MEMBER container)

public:
    MediaSettings() = default;
    explicit MediaSettings(const CameraInfo &camera, const AudioDeviceInfo &audioInputDevice);

    void loadSettings(QSettings *settings);
    void saveSettings(QSettings *settings);
    void dumpProperties(const QString &context) const;

    bool operator==(const MediaSettings &other) const;
    bool operator!=(const MediaSettings &other) const;

    CameraInfo camera;
    AudioDeviceInfo audioInputDevice;
    QString container;
};

class CommonEncoderSettings
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(CommonEncoderSettings)

    Q_PROPERTY(QString codec MEMBER codec)
    Q_PROPERTY(CommonEncoderSettings::EncodingQuality quality MEMBER quality)
    Q_PROPERTY(QVariantMap options MEMBER options)

public:
    enum class EncodingQuality {
        VeryLowQuality = QImageCapture::Quality::VeryLowQuality,
        LowQuality = QImageCapture::Quality::LowQuality,
        NormalQuality = QImageCapture::Quality::NormalQuality,
        HighQuality = QImageCapture::Quality::HighQuality,
        VeryHighQuality = QImageCapture::Quality::VeryHighQuality
    };
    Q_ENUM(EncodingQuality)

    enum class EncodingMode {
        ConstantQualityEncoding = QMediaRecorder::EncodingMode::ConstantQualityEncoding,
        ConstantBitRateEncoding = QMediaRecorder::EncodingMode::ConstantBitRateEncoding,
        AverageBitRateEncoding = QMediaRecorder::EncodingMode::AverageBitRateEncoding,
        TwoPassEncoding = QMediaRecorder::EncodingMode::TwoPassEncoding
    };
    Q_ENUM(EncodingMode)

    explicit CommonEncoderSettings(const QString &codec, CommonEncoderSettings::EncodingQuality quality, const QVariantMap &options);
    virtual ~CommonEncoderSettings() = default;

    virtual void loadSettings(QSettings *settings);
    virtual void saveSettings(QSettings *settings);
    virtual void dumpProperties(const QString &context) const = 0;

    bool operator==(const CommonEncoderSettings &other) const;
    bool operator!=(const CommonEncoderSettings &other) const;

    static QString toString(CommonEncoderSettings::EncodingQuality quality);
    static QString toString(CommonEncoderSettings::EncodingMode mode);

    QString codec;
    CommonEncoderSettings::EncodingQuality quality;
    QVariantMap options;
};

class ImageEncoderSettings : public CommonEncoderSettings
{
    Q_GADGET

    Q_PROPERTY(QSize resolution MEMBER resolution)

public:
    explicit ImageEncoderSettings(const QMediaFormat &settings = {});

    void loadSettings(QSettings *settings) override;
    void saveSettings(QSettings *settings) override;
    void dumpProperties(const QString &context) const override;

    bool operator==(const ImageEncoderSettings &other) const;
    bool operator!=(const ImageEncoderSettings &other) const;

    QMediaFormat toQMediaFormat() const;

    QSize resolution;
};

class AudioEncoderSettings : public CommonEncoderSettings
{
    Q_GADGET

    Q_PROPERTY(CommonEncoderSettings::EncodingMode mode MEMBER mode)
    Q_PROPERTY(int bitRate MEMBER bitRate)
    Q_PROPERTY(int sampleRate MEMBER sampleRate)
    Q_PROPERTY(int channelCount MEMBER channelCount)

public:
    explicit AudioEncoderSettings(const QMediaFormat &settings = {});

    void loadSettings(QSettings *settings) override;
    void saveSettings(QSettings *settings) override;
    void dumpProperties(const QString &context) const override;

    bool operator==(const AudioEncoderSettings &other) const;
    bool operator!=(const AudioEncoderSettings &other) const;

    QMediaFormat toQMediaFormat() const;

    CommonEncoderSettings::EncodingMode mode;
    int bitRate;
    int sampleRate;
    int channelCount;
};

class VideoEncoderSettings : public CommonEncoderSettings
{
    Q_GADGET

    Q_PROPERTY(CommonEncoderSettings::EncodingMode mode MEMBER mode)
    Q_PROPERTY(int bitRate MEMBER bitRate)
    Q_PROPERTY(qreal frameRate MEMBER frameRate)
    Q_PROPERTY(QSize resolution MEMBER resolution)

public:
    explicit VideoEncoderSettings(const QMediaFormat &settings = {});

    void loadSettings(QSettings *settings) override;
    void saveSettings(QSettings *settings) override;
    void dumpProperties(const QString &context) const override;

    bool operator==(const VideoEncoderSettings &other) const;
    bool operator!=(const VideoEncoderSettings &other) const;

    QMediaFormat toQMediaFormat() const;

    CommonEncoderSettings::EncodingMode mode;
    int bitRate;
    qreal frameRate;
    QSize resolution;
};

Q_DECLARE_METATYPE(MediaSettings)
Q_DECLARE_METATYPE(ImageEncoderSettings)
Q_DECLARE_METATYPE(AudioEncoderSettings)
Q_DECLARE_METATYPE(VideoEncoderSettings)
