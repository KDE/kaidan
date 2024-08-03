// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Nicolas Fella <nicolas.fella@gmx.de>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AbstractQrCodeGenerator.h"

#include <QRgb>

#include <ZXing/BarcodeFormat.h>
#include <ZXing/MultiFormatWriter.h>

#include "Kaidan.h"

#define COLOR_TABLE_INDEX_FOR_WHITE 0
#define COLOR_TABLE_INDEX_FOR_BLACK 1

// The maximum is set because QML seems to sometimes set a very high value even if that is not used.
constexpr int MAX_EDGE_PIXEL_COUNT = 1000;

AbstractQrCodeGenerator::AbstractQrCodeGenerator(QObject *parent)
	: QObject(parent)
{
}

QString AbstractQrCodeGenerator::jid() const
{
	return m_jid;
}

void AbstractQrCodeGenerator::setJid(const QString &jid)
{
	if (m_jid != jid) {
		m_jid = jid;
		Q_EMIT jidChanged();
	}
}

void AbstractQrCodeGenerator::setEdgePixelCount(int edgePixelCount)
{
	if (m_edgePixelCount != edgePixelCount) {
		m_edgePixelCount = edgePixelCount;
		Q_EMIT qrCodeChanged();
	}
}

QImage AbstractQrCodeGenerator::qrCode() const
{
	if (m_edgePixelCount > 0 && m_edgePixelCount < MAX_EDGE_PIXEL_COUNT && !m_text.isEmpty()) {
		try {
			ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::QRCode);
			const ZXing::BitMatrix &bitMatrix = writer.encode(m_text.toStdWString(), m_edgePixelCount, m_edgePixelCount);
			return toImage(bitMatrix);
		} catch (const std::invalid_argument &e) {
			Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Generating the QR code failed: %1").arg(QString::fromUtf8(e.what())));
		}
	}

	return {};
}

void AbstractQrCodeGenerator::setText(const QString &text)
{
	if (m_text != text) {
		m_text = text;
		Q_EMIT qrCodeChanged();
	}
}

QImage AbstractQrCodeGenerator::toImage(const ZXing::BitMatrix &bitMatrix)
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

void AbstractQrCodeGenerator::createColorTable(QImage &blackAndWhiteImage)
{
	blackAndWhiteImage.setColor(COLOR_TABLE_INDEX_FOR_WHITE, qRgb(255, 255, 255));
	blackAndWhiteImage.setColor(COLOR_TABLE_INDEX_FOR_BLACK, qRgb(0, 0, 0));
}
