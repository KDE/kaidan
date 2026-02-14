// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt
#include <QTest>
// Kaidan
#include "Keychain.h"
#include "Test.h"

using namespace std::chrono;

template<typename, typename = std::void_t<>>
struct IsVariantType : std::false_type {
};

template<typename... _Types>
struct IsVariantType<std::variant<_Types...>, std::void_t<std::variant<_Types...>>> : std::true_type {
};

template<typename T>
bool checkFutureResult(auto future, const T &value)
{
    const auto result = future.result();
    using ResultType = std::decay_t<decltype(result)>;

    if constexpr (std::is_same_v<std::decay_t<T>, ResultType>) {
        return result == value;
    } else if constexpr (IsVariantType<ResultType>::value) {
        if (auto secret = std::get_if<T>(&result)) {
            return *secret == value;
        }
    }

    return false;
}

class KeychainTest : public Test
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() override
    {
        Test::initTestCase();
        QKeychainFuture::setUnencryptedFallback(true);
    }

    void cleanupTestCase()
    {
        QKeychainFuture::setUnencryptedFallback(false);
    }

    void testKeychain()
    {
        {
            auto writeString = QKeychainFuture::writeKey(QStringLiteral("string"), QStringLiteral("string")).onFailed([](const QKeychainFuture::Error &error) {
                qDebug() << "Could not write string" << error.error() << error.message();
                return error.error();
            });
            QVERIFY(QKeychainFuture::waitForFinished(writeString, 1000ms));
            QVERIFY(checkFutureResult(writeString, QKeychain::Error::NoError));

            auto readString = QKeychainFuture::readKey(QStringLiteral("string")).onFailed([](const QKeychainFuture::Error &error) {
                qDebug() << "Could not read string" << error.error() << error.message();
                return error.error();
            });
            QVERIFY(QKeychainFuture::waitForFinished(readString, 1000ms));
            QVERIFY(checkFutureResult(readString, QStringLiteral("string")));

            auto deleteString = QKeychainFuture::deleteKey(QStringLiteral("string")).onFailed([](const QKeychainFuture::Error &error) {
                qDebug() << "Could not delete string" << error.error() << error.message();
                return error.error();
            });
            QVERIFY(QKeychainFuture::waitForFinished(deleteString, 1000ms));
            QVERIFY(checkFutureResult(deleteString, QKeychain::Error::NoError));
        }

        {
            auto readInvalid = QKeychainFuture::readKey(QStringLiteral("invalid")).onFailed([](const QKeychainFuture::Error &error) {
                qDebug() << "Read invalid string" << error.error() << error.message();
                return error.error();
            });
            QVERIFY(QKeychainFuture::waitForFinished(readInvalid, 1000ms));
            QVERIFY(checkFutureResult(readInvalid, QKeychain::Error::EntryNotFound));

            auto deleteInvalid = QKeychainFuture::deleteKey(QStringLiteral("bin")).onFailed([](const QKeychainFuture::Error &error) {
                qDebug() << "Deleted invalid string" << error.error() << error.message();
                return error.error();
            });
            QVERIFY(QKeychainFuture::waitForFinished(deleteInvalid, 1000ms));
            QVERIFY(checkFutureResult(deleteInvalid, QKeychain::Error::NoError));
        }
    }
};

QTEST_GUILESS_MAIN(KeychainTest)
#include "KeychainTest.moc"
