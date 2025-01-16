// SPDX-FileCopyrightText: 2016 J-P Nurmi
//
// SPDX-License-Identifier: MIT

/*
 * Copyright (c) 2016 J-P Nurmi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "StatusBar.h"
#ifdef Q_OS_ANDROID
#include "private/qandroidextras_p.h"
#include <QtCore/private/qjnihelpers_p.h>

// WindowManager.LayoutParams
#define FLAG_TRANSLUCENT_STATUS 0x04000000
#define FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS 0x80000000
#define SYSTEM_UI_FLAG_LAYOUT_STABLE 0x00000100
#endif

StatusBar::StatusBar(QObject *parent)
    : QObject(parent)
{
}

QColor StatusBar::color() const
{
    return m_color;
}

void StatusBar::setColor(const QColor &color)
{
    m_color = color;
    Q_EMIT colorChanged();
    if (!isAvailable())
        return;
}

bool StatusBar::isAvailable() const
{
#ifdef Q_OS_ANDROID
    return QtAndroidPrivate::androidSdkVersion() >= 21;
#else
    return false;
#endif
}

#include "moc_StatusBar.cpp"
