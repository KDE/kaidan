// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Nicolas Fella <nicolas.fella@gmx.de>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AbstractQrCodeGenerator.h"

// Qt
#include <QImage>
// KDE
#include <Prison/barcode.h>
// Kaidan
#include "Kaidan.h"

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
    if (m_edgePixelCount > 0 && !m_text.isEmpty()) {
        try {
            auto qrCodeGenerator = Prison::Barcode::create(Prison::BarcodeType::QRCode);
            qrCodeGenerator->setData(m_text);
            return qrCodeGenerator->toImage({static_cast<double>(m_edgePixelCount), static_cast<double>(m_edgePixelCount)});
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

#include "moc_AbstractQrCodeGenerator.cpp"
