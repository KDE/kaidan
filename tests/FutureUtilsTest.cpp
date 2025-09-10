// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt
#include <QTest>
// Kaidan
#include "FutureUtils.h"
#include "Test.h"

class FutureUtilsTest : public Test
{
    Q_OBJECT

private:
    Q_SLOT void testJoin();
};

void FutureUtilsTest::testJoin()
{
    QPromise<int> i1;
    QPromise<int> i2;
    QPromise<int> i3;

    auto futures = QList{i1.future(), i2.future(), i3.future()};
    auto joinedFuture = join(this, futures);

    i2.addResult(10);
    i2.finish();
    i3.addResult(11);
    i3.finish();
    i1.addResult(9);
    i1.finish();

    while (!joinedFuture.isFinished()) {
        QCoreApplication::processEvents();
    }

    QVERIFY(joinedFuture.isFinished());
    QCOMPARE(joinedFuture.result(), (QList<int>{9, 10, 11}));
}

QTEST_GUILESS_MAIN(FutureUtilsTest)
#include "FutureUtilsTest.moc"
