// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QMutex>
#include <QObject>

/**
 * @brief The ServerFeaturesCache class temporarily stores the features of a server. This
 * can be used to for example enable or disable certain features in the UI.
 *
 * All methods in the class are thread-safe.
 */
class ServerFeaturesCache : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool inBandRegistrationSupported READ inBandRegistrationSupported NOTIFY inBandRegistrationSupportedChanged)
    Q_PROPERTY(bool httpUploadSupported READ httpUploadSupported NOTIFY httpUploadSupportedChanged)
    Q_PROPERTY(qint64 httpUploadLimit READ httpUploadLimit NOTIFY httpUploadLimitChanged)
    Q_PROPERTY(QString httpUploadLimitString READ httpUploadLimitString NOTIFY httpUploadLimitChanged)

public:
    explicit ServerFeaturesCache(QObject *parent = nullptr);

    /**
     * Returns whether In-Band Registration features after login on the server are supported by it.
     */
    bool inBandRegistrationSupported();

    /**
     * Sets whether In-Band Registration is supported.
     */
    void setInBandRegistrationSupported(bool supported);

    /**
     * Returns whether HTTP File Upload is available and can be currently be used.
     */
    bool httpUploadSupported();
    void setHttpUploadSupported(bool supported);

    /**
     * Returns the HTTP File Upload maximum allowed syze in bytes.
     */
    qint64 httpUploadLimit();
    QString httpUploadLimitString();
    void setHttpUploadLimit(qint64 size);

Q_SIGNALS:
    /**
     * Emitted when In-Band Registration support changed.
     */
    void inBandRegistrationSupportedChanged();

    void httpUploadSupportedChanged();
    void httpUploadLimitChanged();

private:
    QMutex m_mutex;
    bool m_inBandRegistrationSupported = false;
    bool m_httpUploadSupported = false;
    qint64 m_httpUploadLimit = 0;
};
