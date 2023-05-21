// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppColorGenerator.h"
#include "../hsluv-c/hsluv.h"
#include <ctgmath>
#include <QCryptographicHash>

/// \brief Generates a color from the input value. This is intended for
/// generating colors for contacts. The generated colors are "consistent", so
/// they are shared between all clients with support for XEP-0392: Consistent
/// Color Generation.
///
/// \param name This should be the (user-specified) nickname of the
/// participant. If there is no nickname set, the bare JID shall be used.
/// \param deficiency Color correction to be done, defaults to
/// ColorVisionDeficiency::NoDeficiency.

QXmppColorGenerator::RGBColor QXmppColorGenerator::generateColor(
        const QString &name, ColorVisionDeficiency deficiency)
{
    QByteArray input = name.toUtf8();

    // hash input through SHA-1
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(input);

    // the first two bytes are used to calculate the angle/hue
    int angle = hash.result().at(0) + hash.result().at(1) * 256;

    double hue = double(angle) / 65536.0 * 360.0;
    double saturation = 100.0;
    double lightness = 50.0;

    // Corrections for Color Vision Deficiencies
    // this uses floating point modulo (fmod)
    if (deficiency == RedGreenBlindness) {
        hue += 90.0;
        hue = fmod(hue, 180);
        hue -= 90.0;
        hue = fmod(hue, 360);
    } else if (deficiency == BlueBlindness) {
        hue = fmod(hue, 180);
    }

    // convert to rgb values (values are between 0.0 and 1.0)
    double red, green, blue = 0.0;
    hsluv2rgb(hue, saturation, lightness, &red, &green, &blue);

    RGBColor color;
    color.red = quint8(red * 255.0);
    color.green = quint8(green * 255.0);
    color.blue = quint8(blue * 255.0);
    return color;
}
