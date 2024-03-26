// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCameraImageCapture>
#include <QUrl>

// A QCameraImageCapture that mimic api of QMediaRecorder

class CameraImageCapture : public QCameraImageCapture
{
	Q_OBJECT

	Q_PROPERTY(QUrl actualLocation READ actualLocation NOTIFY actualLocationChanged)
	Q_PROPERTY(QMultimedia::AvailabilityStatus availability READ availability NOTIFY availabilityChanged)
	Q_PROPERTY(bool isAvailable READ isAvailable NOTIFY availabilityChanged)

public:
	CameraImageCapture(QMediaObject *mediaObject, QObject *parent = nullptr);

	QUrl actualLocation() const;

Q_SIGNALS:
	void availabilityChanged(QMultimedia::AvailabilityStatus availability);
	void actualLocationChanged(const QUrl &location);

protected:
	bool setMediaObject(QMediaObject *mediaObject) override;

private:
	QUrl m_actualLocation;
};
