// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QMimeType>
#include <QtQml/qqmlregistration.h>
// QXmpp
#include <QXmppSpamReport.h>

// Registers types from other libraries for QML.

namespace QXmppSpamReportQmlEnums
{
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(QXmppSpamReport)
QML_NAMED_ELEMENT(SpamReport)
}

struct QMimeTypeForeign {
    Q_GADGET
    QML_FOREIGN(QMimeType)
    QML_ANONYMOUS
};
