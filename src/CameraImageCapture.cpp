// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CameraImageCapture.h"

CameraImageCapture::CameraImageCapture(QMediaObject *mediaObject, QObject *parent)
	: QCameraImageCapture(mediaObject, parent)
{
	connect(this, &QCameraImageCapture::imageSaved,
		this, [this](int id, const QString &filePath) {
			Q_UNUSED(id);
			m_actualLocation = QUrl::fromLocalFile(filePath);
			emit actualLocationChanged(m_actualLocation);
		});
}

QUrl CameraImageCapture::actualLocation() const
{
	return m_actualLocation;
}

bool CameraImageCapture::setMediaObject(QMediaObject *mediaObject)
{
	const QMultimedia::AvailabilityStatus previousAvailability = availability();
	const bool result = QCameraImageCapture::setMediaObject(mediaObject);

	if (previousAvailability != availability()) {
		QMetaObject::invokeMethod(this, [this]() {
				emit availabilityChanged(availability());
			}, Qt::QueuedConnection);
	}

	return result;
}
