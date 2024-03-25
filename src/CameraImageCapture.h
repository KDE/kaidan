// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImageCapture>
#include <QUrl>

// A QImageCapture that mimic api of QMediaRecorder

class CameraImageCapture : public QImageCapture
{
	Q_OBJECT

	Q_PROPERTY(QUrl actualLocation READ actualLocation NOTIFY actualLocationChanged)
	// TODO Q_PROPERTY(QMultimedia::AvailabilityStatus availability READ availability NOTIFY availabilityChanged)
	Q_PROPERTY(bool isAvailable READ isAvailable NOTIFY availabilityChanged)

public:
	CameraImageCapture(QMediaCaptureSession *mediaObject, QObject *parent = nullptr);

	QUrl actualLocation() const;

signals:
	void availabilityChanged(QImageCapture::Error availability);
	void actualLocationChanged(const QUrl &location);

protected:
	bool setMediaObject(QMediaCaptureSession *mediaObject);

private:
	QUrl m_actualLocation;
};
