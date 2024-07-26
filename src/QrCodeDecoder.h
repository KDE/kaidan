// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

/**
 * Decoder for QR codes. This is a backend for \c QrCodeScanner .
 */
class QrCodeDecoder : public QObject
{
	Q_OBJECT

public:
	/**
	 * Instantiates a QR code decoder.
	 *
	 * @param parent parent object
	 */
	explicit QrCodeDecoder(QObject *parent = nullptr);

Q_SIGNALS:
	/**
	 * Emitted when the decoding failed.
	 */
	void decodingFailed();

	/**
	 * Emitted when the decoding succeeded.
	 *
	 * @param tag string which was decoded by the QR code decoder
	 */
	void decodingSucceeded(const QString &tag);

public Q_SLOTS:
	/**
	 * Tries to decode the QR code from the given image. When decoding has
	 * finished @c decodingFinished() will be emitted. In case a QR code was found,
	 * also @c tagFound() will be emitted.
	 *
	 * @param image image which may contain a QR code to decode to a string.
	 *        It needs to be in grayscale format (one byte per pixel).
	 */
	void decodeImage(const QImage &image);
};
