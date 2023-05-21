// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAbstractVideoFilter>
#include <QFuture>

#include "QrCodeDecoder.h"
#include "QrCodeVideoFrame.h"

/**
 * video filter to be registered in C++, instantiated and attached in QML
 */
class QrCodeScannerFilter : public QAbstractVideoFilter
{
	friend class QrCodeScannerFilterRunnable;

	Q_OBJECT

public:
	/**
	 * Instantiates a QR code scanner filter.
	 *
	 * @param parent parent object
	 */
	explicit QrCodeScannerFilter(QObject *parent = nullptr);
	~QrCodeScannerFilter() override;

	/**
	 * @return decoder for decoding a video frame
	 */
	QrCodeDecoder *decoder();
	QVideoFilterRunnable *createFilterRunnable() override;

	/**
	 * Sets the video frame format of the QML Camera object to our default format
	 *
	 * @param qmlCamera Camera object from QML
	 */
	Q_INVOKABLE void setCameraDefaultVideoFormat(QObject *qmlCamera);

signals:
	/**
	 * Emitted when the scanning of an image did not succeed, i.e. no valid QR code was found.
	 */
	void scanningFailed();

	/**
	 * Emitted when the scanning of an image succeeded, i.e. a valid QR code was found and decoded.
	 *
	 * @param result decoded content of the QR code
	 */
	void scanningSucceeded(const QString& result);

	/**
	 * Emitted when a video frame with an unsupported format is received.
	 *
	 * @param format format of the video frame which is not supported
	 */
	void unsupportedFormatReceived(const QString& format);

private:
	bool m_videoFrameMirrored = false;
	QrCodeDecoder *m_decoder;

	/**
	 * frame of the video which may contain a QR code
	 */
	QrCodeVideoFrame m_frame;
	QFuture<void> m_processThread;
};

/**
 * runnable which is created everytime the filter gets a new frame
 */
class QrCodeScannerFilterRunnable : public QObject, public QVideoFilterRunnable
{
	Q_OBJECT

public:
	explicit QrCodeScannerFilterRunnable(QrCodeScannerFilter *m_filter);

	/**
	 * Runs the decoding in a new thread whenever a new frame is taken by the camera.
	 */
	QVideoFrame run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags) override;

	/**
	 * Converts a given frame, which may contain a QR code, to an image and then tries to decode it.
	 *
	 * @param videoFrame frame to be converted and which may contain a QR code to be decoded
	 * @param filter filter of the current execution
	 */
	void processVideoFrameProbed(QrCodeVideoFrame videoFrame, QrCodeScannerFilter *filter);

private:
	QrCodeScannerFilter *m_filter;
};
