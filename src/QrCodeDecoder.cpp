// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "QrCodeDecoder.h"
// Qt
#include <QImage>
// ZXing-cpp
#include <ZXing/ZXVersion.h>
#define ZXING_VERSION \
	QT_VERSION_CHECK(ZXING_VERSION_MAJOR, ZXING_VERSION_MINOR, ZXING_VERSION_PATCH)

#include <ZXing/ReadBarcode.h>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/DecodeHints.h>
#include <ZXing/Result.h>
#include <ZXing/TextUtfEncoding.h>

using namespace ZXing;

QrCodeDecoder::QrCodeDecoder(QObject *parent)
	: QObject(parent)
{
}

void QrCodeDecoder::decodeImage(const QImage &image)
{
	// Advise the decoder to check for QR codes and to try decoding rotated versions of the image.
	const auto decodeHints = DecodeHints().setFormats(BarcodeFormat::QRCode);
	const auto result = ReadBarcode({image.bits(), image.width(), image.height(), ZXing::ImageFormat::Lum, image.bytesPerLine()}, decodeHints);

	// FIXME: `this` is not supposed to become nullptr in well-defined C++ code,
	//        so if we are unlucky, the compiler may optimize the entire check away.
	//        Additionally, this could be racy if the object is deleted on the other thread
	//        in between this check and the emit.
	const auto *self = this;
	if (!self) {
		return;
	}

	// If a QR code could be found and decoded, emit a signal with the decoded string.
	// Otherwise, emit a signal for failed decoding.
	if (result.isValid())
#if ZXING_VERSION < QT_VERSION_CHECK(2, 0, 0)
		Q_EMIT decodingSucceeded(QString::fromStdString(TextUtfEncoding::ToUtf8(result.text())));
#else
		Q_EMIT decodingSucceeded(QString::fromStdString(result.text()));
#endif
	else
		Q_EMIT decodingFailed();
}
