// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>

class Test : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

protected Q_SLOTS:
    virtual void initTestCase();

private:
    void removeTestDataDirectory();
};
