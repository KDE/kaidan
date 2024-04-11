// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QAudio>
#include <QAudioDevice>

class AudioDeviceInfo : public QAudioDevice
{
    Q_GADGET

    Q_PROPERTY(bool isNull READ isNull CONSTANT)
    // TODO Q_PROPERTY(QString deviceName READ deviceName CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    // TODO Q_PROPERTY(QStringList supportedCodecs READ supportedCodecs CONSTANT)
    // TODO Q_PROPERTY(QList<int> supportedSampleRates READ supportedSampleRates CONSTANT)
    // TODO Q_PROPERTY(QList<int> supportedChannelCounts READ supportedChannelCounts CONSTANT)
    // TODO Q_PROPERTY(QList<int> supportedSampleSizes READ supportedSampleSizes CONSTANT)

public:
    AudioDeviceInfo(const QAudioDevice &other);
    AudioDeviceInfo() = default;

    QString description() const;

    static QString description(const QString &deviceName);
};

class AudioDeviceModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(AudioDeviceModel::Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY audioDevicesChanged)
    Q_PROPERTY(QList<QAudioDevice> audioDevices READ audioDevices NOTIFY audioDevicesChanged)
    Q_PROPERTY(AudioDeviceInfo defaultAudioDevice READ defaultAudioDevice NOTIFY audioDevicesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(AudioDeviceInfo currentAudioDevice READ currentAudioDevice WRITE setCurrentAudioDevice NOTIFY currentIndexChanged)

public:
    enum Mode { AudioInput = QAudioDevice::Mode::Input, AudioOutput = QAudioDevice::Mode::Output };
    Q_ENUM(Mode)

    enum CustomRoles {
        IsNullRole = Qt::UserRole,
        DeviceNameRole,
        DescriptionRole,
        SupportedCodecsRole,
        SupportedSampleRatesRole,
        SupportedChannelCountsRole,
        SupportedSampleSizesRole,
        AudioDeviceInfoRole
    };

    explicit AudioDeviceModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    AudioDeviceModel::Mode mode() const;
    void setMode(AudioDeviceModel::Mode mode);

    QList<QAudioDevice> audioDevices() const;
    AudioDeviceInfo defaultAudioDevice() const;

    int currentIndex() const;
    void setCurrentIndex(int currentIndex);

    AudioDeviceInfo currentAudioDevice() const;
    void setCurrentAudioDevice(const AudioDeviceInfo &currentAudioDevice);

    Q_INVOKABLE AudioDeviceInfo audioDevice(int row) const;
    Q_INVOKABLE int indexOf(const QString &deviceName) const;

    static AudioDeviceInfo defaultAudioInputDevice();
    static AudioDeviceInfo audioInputDevice(const QString &deviceName);

    static AudioDeviceInfo defaultAudioOutputDevice();
    static AudioDeviceInfo audioOutputDevice(const QString &deviceName);

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void modeChanged();
    void audioDevicesChanged();
    void currentIndexChanged();

private:
    AudioDeviceModel::Mode m_mode = AudioDeviceModel::Mode::AudioInput;
    QList<QAudioDevice> m_audioDevices;
    int m_currentIndex = -1;
};

Q_DECLARE_METATYPE(AudioDeviceInfo)
