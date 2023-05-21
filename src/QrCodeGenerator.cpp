// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Nicolas Fella <nicolas.fella@gmx.de>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "QrCodeGenerator.h"

#include <QImage>
#include <QRgb>

#include <ZXing/BarcodeFormat.h>
#include <ZXing/MultiFormatWriter.h>

#include "AccountManager.h"
#include "Kaidan.h"
#include "QmlUtils.h"
#include "Settings.h"
#include "qxmpp-exts/QXmppUri.h"

#include <stdexcept>

#define COLOR_TABLE_INDEX_FOR_WHITE 0
#define COLOR_TABLE_INDEX_FOR_BLACK 1

QrCodeGenerator::QrCodeGenerator(QObject *parent)
	: QObject(parent)
{
}

QImage QrCodeGenerator::generateLoginUriQrCode(int edgePixelCount)
{
	QXmppUri uri;

	uri.setJid(AccountManager::instance()->jid());
	uri.setAction(QXmppUri::Login);

	if (Kaidan::instance()->settings()->authPasswordVisibility() != Kaidan::PasswordInvisible)
		uri.setPassword(AccountManager::instance()->password());

	return generateQrCode(edgePixelCount, uri.toString());
}

QImage QrCodeGenerator::generateOwnTrustMessageQrCode(int edgePixelCount)
{
	return generateTrustMessageQrCode(edgePixelCount, AccountManager::instance()->jid());
}

QImage QrCodeGenerator::generateContactTrustMessageQrCode(int edgePixelCount, const QString &contactJid)
{
	return generateTrustMessageQrCode(edgePixelCount, contactJid);
}

QImage QrCodeGenerator::generateTrustMessageQrCode(int edgePixelCount, const QString &jid)
{
	return generateQrCode(edgePixelCount, QmlUtils::trustMessageUriString(jid));
}

QImage QrCodeGenerator::generateQrCode(int edgePixelCount, const QString &text)
{
	try {
		ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::QRCode);
		const ZXing::BitMatrix &bitMatrix = writer.encode(text.toStdWString(), edgePixelCount, edgePixelCount);
		return toImage(bitMatrix);
	} catch (const std::invalid_argument &e) {
		emit Kaidan::instance()->passiveNotificationRequested(tr("Generating the QR code failed: %1").arg(e.what()));
	}

	return {};
}

QImage QrCodeGenerator::toImage(const ZXing::BitMatrix &bitMatrix)
{
	QImage monochromeImage(bitMatrix.width(), bitMatrix.height(), QImage::Format_Mono);

	createColorTable(monochromeImage);

	for (int y = 0; y < bitMatrix.height(); ++y) {
		for (int x = 0; x < bitMatrix.width(); ++x) {
			int colorTableIndex = bitMatrix.get(x, y) ? COLOR_TABLE_INDEX_FOR_BLACK : COLOR_TABLE_INDEX_FOR_WHITE;
			monochromeImage.setPixel(y, x, colorTableIndex);
		}
	}

	return monochromeImage;
}

void QrCodeGenerator::createColorTable(QImage &blackAndWhiteImage)
{
	blackAndWhiteImage.setColor(COLOR_TABLE_INDEX_FOR_WHITE, qRgb(255, 255, 255));
	blackAndWhiteImage.setColor(COLOR_TABLE_INDEX_FOR_BLACK, qRgb(0, 0, 0));
}
