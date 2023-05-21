// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef QXMPPCOLORGENERATOR_H
#define QXMPPCOLORGENERATOR_H

#include <QByteArray>
#include <QString>

class QXmppColorGenerator
{
public:
    struct RGBColor {
        quint8 red = 0;
        quint8 green = 0;
        quint8 blue = 0;
    };

    enum ColorVisionDeficiency {
        NoDeficiency,
        RedGreenBlindness,
        BlueBlindness
    };

    static RGBColor generateColor(const QString&,
                                  ColorVisionDeficiency = NoDeficiency);
};

#endif // QXMPPCOLORGENERATOR_H
