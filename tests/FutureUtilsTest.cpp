// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/FutureUtils.h"
#include <QtTest>

class FutureUtilsTest : public QObject
{
	Q_OBJECT

private:
	Q_SLOT void testJoin();
};

void FutureUtilsTest::testJoin()
{
	QFutureInterface<int> i1;
	QFutureInterface<int> i2;
	QFutureInterface<int> i3;

	auto futures = QVector { i1.future(), i2.future(), i3.future() };
	auto joinedFuture = join(this, futures);

	i2.reportResult(10);
	i2.reportFinished();
	i3.reportResult(11);
	i3.reportFinished();
	i1.reportResult(9);
	i1.reportFinished();

	while (!joinedFuture.isFinished()) {
		QCoreApplication::processEvents();
	}

	QVERIFY(joinedFuture.isFinished());
	QCOMPARE(joinedFuture.result(), (QVector<int> { 9, 10, 11 }));
}

QTEST_GUILESS_MAIN(FutureUtilsTest)
#include "FutureUtilsTest.moc"