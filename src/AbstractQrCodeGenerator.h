// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QObject>

#include <ZXing/BitMatrix.h>

class AbstractQrCodeGenerator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(int edgePixelCount MEMBER m_edgePixelCount WRITE setEdgePixelCount)
    Q_PROPERTY(QImage qrCode READ qrCode NOTIFY qrCodeChanged)

public:
    explicit AbstractQrCodeGenerator(QObject *parent = nullptr);

    QString jid() const;
    void setJid(const QString &jid);
    Q_SIGNAL void jidChanged();

    void setEdgePixelCount(int edgePixelCount);

    QImage qrCode() const;
    Q_SIGNAL void qrCodeChanged();

protected:
    void setText(const QString &text);

private:
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

    QString m_jid;

    // Text to be encoded as a QR code.
    QString m_text;

    // Number of pixels as the width and height of the QR code.
    int m_edgePixelCount;
};
