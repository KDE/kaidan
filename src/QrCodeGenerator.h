// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <ZXing/BitMatrix.h>

class QImage;

class QrCodeGenerator : public QObject
{
	Q_OBJECT

public:
	/**
	 * Instantiates a QR code generator.
	 *
	 * @param parent parent object
	 */
	explicit QrCodeGenerator(QObject *parent = nullptr);

	/**
	 * Gerenates a QR code encoding the credentials of the currently used account to log into it with another client.
	 *
	 * @param edgePixelCount number of pixels as the width and height of the QR code
	 */
	Q_INVOKABLE static QImage generateLoginUriQrCode(int edgePixelCount);

	/**
	 * Gerenates a QR code encoding the Trust Message URI of the currently used
	 * account.
	 *
	 * If no keys for the Trust Message URI can be found, an XMPP URI containing
	 * only the own bare JID is used.
	 *
	 * @param edgePixelCount number of pixels as the width and height of the QR code
	 */
	Q_INVOKABLE static QImage generateOwnTrustMessageQrCode(int edgePixelCount);

	/**
	 * Gerenates a QR code encoding the Trust Message URI of a contact.
	 *
	 * If no keys for the Trust Message URI can be found, an XMPP URI containing
	 * only the contact's bare JID is used.
	 *
	 * @param edgePixelCount number of pixels as the width and height of the QR code
	 * @param contactJid bare JID of the contact
	 */
	Q_INVOKABLE static QImage generateContactTrustMessageQrCode(int edgePixelCount, const QString &contactJid);

private:
	/**
	 * Gerenates a QR code encoding a Trust Message URI.
	 *
	 * If no keys for the Trust Message URI can be found, an XMPP URI containing
	 * only the bare JID is used.
	 *
	 * @param edgePixelCount number of pixels as the width and height of the QR code
	 * @param jid bare JID of the key owner
	 */
	Q_INVOKABLE static QImage generateTrustMessageQrCode(int edgePixelCount, const QString &jid);

	/**
	 * Gerenates a QR code.
	 *
	 * @param edgePixelCount number of pixels as the width and height of the QR code
	 * @param text string being encoded as a QR code
	 */
	Q_INVOKABLE static QImage generateQrCode(int edgePixelCount, const QString &text);

	/**
	 * Generates an image with black and white pixels from a given matrix of bits representing a QR code.
	 *
	 * @param bitMatrix matrix of bits representing the two colors black and white
	 */
	static QImage toImage(const ZXing::BitMatrix &bitMatrix);

	/**
	 * Sets up a color image for a given monochrome image consisting only of black and white pixels.
	 * @param blackAndWhiteImage image for which a color table with the colors black and white is created
	 */
	static void createColorTable(QImage &blackAndWhiteImage);
};
