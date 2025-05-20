// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>

class QImage;

class QrCodeGenerator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int edgePixelCount MEMBER m_edgePixelCount WRITE setEdgePixelCount)
    Q_PROPERTY(QImage qrCode READ qrCode NOTIFY qrCodeChanged)

public:
    explicit QrCodeGenerator(QObject *parent = nullptr);

    void setText(const QString &text);
    void setEdgePixelCount(int edgePixelCount);

    QImage qrCode() const;
    Q_SIGNAL void qrCodeChanged();

private:
    // Text to be encoded as a QR code.
    QString m_text;

    // Number of pixels as the width and height of the QR code.
    int m_edgePixelCount;
};
