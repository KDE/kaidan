/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "QrCodeGenerator.h"

#include <QImage>
#include <QRgb>

#include <ZXing/ZXVersion.h>
#define ZXING_VERSION \
	QT_VERSION_CHECK(ZXING_VERSION_MAJOR, ZXING_VERSION_MINOR, ZXING_VERSION_PATCH)

#include <ZXing/BarcodeFormat.h>
#include <ZXing/MultiFormatWriter.h>

#include "AccountManager.h"
#include "MessageModel.h"
#include "Kaidan.h"
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

	if (Kaidan::instance()->passwordVisibility() != Kaidan::PasswordInvisible)
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
	const auto keys = MessageModel::instance()->keys().value(jid);
	QList<QString> authenticatedKeys;
	QList<QString> distrustedKeys;

	for (auto itr = keys.constBegin(); itr != keys.constEnd(); ++itr) {
		const auto key = itr.key().toHex();
		const auto trustLevel = itr.value();

		if (trustLevel == QXmpp::TrustLevel::Authenticated) {
			authenticatedKeys.append(key);
		} else if (trustLevel == QXmpp::TrustLevel::ManuallyDistrusted) {
			distrustedKeys.append(key);
		}
	}

	QXmppUri uri;
	uri.setJid(jid);

	// Create a Trust Message URI only if there are keys for it.
	if (!authenticatedKeys.isEmpty() || !distrustedKeys.isEmpty()) {
		uri.setAction(QXmppUri::TrustMessage);
		// TODO: Find solution to pass enum to "uri.setEncryption()" instead of string (see QXmppGlobal::encryptionToString())
		uri.setEncryption(QStringLiteral("urn:xmpp:omemo:2"));
		uri.setTrustedKeysIds(authenticatedKeys);
		uri.setDistrustedKeysIds(distrustedKeys);
	}

	return generateQrCode(edgePixelCount, uri.toString());
}

QImage QrCodeGenerator::generateQrCode(int edgePixelCount, const QString &text)
{
	try {
#if ZXING_VERSION >= QT_VERSION_CHECK(1, 1, 1)
		ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::QRCode);
#else
		ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::QR_CODE);
#endif
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
