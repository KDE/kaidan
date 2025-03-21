// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Test.h"

// Qt
#include <QStandardPaths>
#include <QTest>

void Test::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    // Remove old test data.
    removeTestDataDirectory();
}

void Test::removeTestDataDirectory()
{
    const auto testDirectoryPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation);

    if (auto testDirectory = QDir(testDirectoryPath); testDirectory.exists()) {
        QVERIFY(testDirectory.removeRecursively());
    }
}

#include "moc_Test.cpp"
