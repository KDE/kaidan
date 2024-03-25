// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CameraImageCapture.h"

CameraImageCapture::CameraImageCapture(QMediaCaptureSession *mediaObject, QObject *parent)
	: QImageCapture() // TODO QImageCapture(mediaObject, parent)
{
	/* TODO connect(this, &QImageCaptureeSaved,
		this, [this](int id, const QString &filePath) {
			Q_UNUSED(id);
			m_actualLocation = QUrl::fromLocalFile(filePath);
			Q_EMIT actualLocationChanged(m_actualLocation);
		});*/
}

QUrl CameraImageCapture::actualLocation() const
{
	return m_actualLocation;
}

bool CameraImageCapture::setMediaObject(QMediaCaptureSession *mediaObject)
{
	/* TODO const QMultimedia::AvailabilityStatus previousAvailability = availability();
	const bool result = QImageCapture::setMediaObject(mediaObject);

	if (previousAvailability != availability()) {
		QMetaObject::invokeMethod(this, [this]() {
				Q_EMIT availabilityChanged(availability());
			}, Qt::QueuedConnection);
	}

	return result;*/
	return false;
}
