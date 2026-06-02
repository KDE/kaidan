// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt
#include <QTest>
// Kaidan
#include "InputValidator.h"
#include "Test.h"

class InputValidatorTest : public Test
{
    Q_OBJECT

private Q_SLOTS:
    void testNone_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QValidator::State>("expected");

        QTest::newRow("Empty string") << QString() << QValidator::State::Acceptable;
        QTest::newRow("Non-empty string") << QStringLiteral("abc") << QValidator::State::Acceptable;
        QTest::newRow("Whitespaced string") << QStringLiteral("             ") << QValidator::State::Acceptable;
    }

    void testNone()
    {
        QFETCH(QString, input);
        QFETCH(QValidator::State, expected);
        const auto expectedBool = expected == QValidator::State::Acceptable;
        const auto basePatterns = InputValidator::Pattern::None;

        {
            QCOMPARE(InputValidator::isValid(input, basePatterns), expectedBool);
        }

        {
            InputValidator validator(basePatterns);
            int pos = 0;

            QCOMPARE(validator.validate(input, pos), expected);
        }
    }

    void testEmpty_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QValidator::State>("expected");

        QTest::newRow("Empty string") << QString() << QValidator::State::Acceptable;
        QTest::newRow("Non-empty string") << QStringLiteral("abc") << QValidator::State::Invalid;
        QTest::newRow("Whitespaced string") << QStringLiteral("             ") << QValidator::State::Invalid;
    }

    void testEmpty()
    {
        QFETCH(QString, input);
        QFETCH(QValidator::State, expected);
        const auto expectedBool = expected == QValidator::State::Acceptable;
        const auto basePatterns = InputValidator::Pattern::Empty;

        {
            QCOMPARE(InputValidator::isValid(input, basePatterns), expectedBool);
        }

        {
            InputValidator validator(basePatterns);
            int pos = 0;

            QCOMPARE(validator.validate(input, pos), expected);
        }
    }

    void testNotEmpty_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QValidator::State>("expected");

        QTest::newRow("Empty string") << QString() << QValidator::State::Intermediate;
        QTest::newRow("Non-empty string") << QStringLiteral("abc") << QValidator::State::Acceptable;
        QTest::newRow("Whitespaced string") << QStringLiteral("             ") << QValidator::State::Acceptable;
    }

    void testNotEmpty()
    {
        QFETCH(QString, input);
        QFETCH(QValidator::State, expected);
        const auto expectedBool = expected == QValidator::State::Acceptable;
        const auto basePatterns = InputValidator::Pattern::NotEmpty;

        {
            QCOMPARE(InputValidator::isValid(input, basePatterns), expectedBool);
        }

        {
            InputValidator validator(basePatterns);
            int pos = 0;

            QCOMPARE(validator.validate(input, pos), expected);
        }
    }

    void testUsername_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QValidator::State>("expected");

        QTest::newRow("Empty string") << QString() << QValidator::State::Intermediate;
        QTest::newRow("Non-empty string with dot") << QStringLiteral("abc.def") << QValidator::State::Acceptable;
        QTest::newRow("Non-empty string with hyphen") << QStringLiteral("abc-def") << QValidator::State::Acceptable;
        QTest::newRow("Non-empty string with space") << QStringLiteral("abc def") << QValidator::State::Invalid;
        QTest::newRow("Whitespaced string") << QStringLiteral("             ") << QValidator::State::Invalid;
        QTest::newRow("Elle_à_ç_è") << QStringLiteral("Elle_à_ç_è") << QValidator::State::Acceptable;
    }

    void testUsername()
    {
        QFETCH(QString, input);
        QFETCH(QValidator::State, expected);
        const auto expectedBool = expected == QValidator::State::Acceptable;
        const auto basePatterns = InputValidator::Pattern::Username;

        {
            QCOMPARE(InputValidator::isValid(input, basePatterns), expectedBool);
        }

        {
            InputValidator validator(basePatterns);
            int pos = 0;

            QCOMPARE(validator.validate(input, pos), expected);
        }
    }

    void testHostname_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QValidator::State>("expected");

        QTest::newRow("Empty string") << QString() << QValidator::State::Intermediate;
        QTest::newRow("Non-empty string with dot") << QStringLiteral("abc.def") << QValidator::State::Acceptable;
        QTest::newRow("Non-empty string with hyphen") << QStringLiteral("abc-def") << QValidator::State::Acceptable;
        QTest::newRow("Non-empty string with space") << QStringLiteral("abc def") << QValidator::State::Intermediate;
        QTest::newRow("Whitespaced string") << QStringLiteral("             ") << QValidator::State::Intermediate;
        QTest::newRow("localhost") << QStringLiteral("localhost") << QValidator::State::Acceptable;
        QTest::newRow("192.168.1.1") << QStringLiteral("192.168.1.1") << QValidator::State::Acceptable;
        QTest::newRow("::1") << QStringLiteral("::1") << QValidator::State::Acceptable;
        QTest::newRow("example.org") << QStringLiteral("example.org") << QValidator::State::Acceptable;
        QTest::newRow("xmpp.example.org") << QStringLiteral("xmpp.example.org") << QValidator::State::Acceptable;
        QTest::newRow("[::1]") << QStringLiteral("[::1]") << QValidator::State::Intermediate;
        QTest::newRow("Hell\"oo") << QStringLiteral("Hell\"oo") << QValidator::State::Intermediate;
        QTest::newRow("Hell&oo") << QStringLiteral("Hell&oo") << QValidator::State::Intermediate;
        QTest::newRow("Hell'oo") << QStringLiteral("Hell'oo") << QValidator::State::Intermediate;
        QTest::newRow("Hell/oo") << QStringLiteral("Hell/oo") << QValidator::State::Intermediate;
        QTest::newRow("Hell:oo") << QStringLiteral("Hell:oo") << QValidator::State::Intermediate;
        QTest::newRow("Hell<oo") << QStringLiteral("Hell<oo") << QValidator::State::Intermediate;
        QTest::newRow("Hell>oo") << QStringLiteral("Hell>oo") << QValidator::State::Intermediate;
        QTest::newRow("Hell@oo") << QStringLiteral("Hell@oo") << QValidator::State::Intermediate;
        QTest::newRow("Hell oo") << QStringLiteral("Hell oo") << QValidator::State::Intermediate;
    }

    void testHostname()
    {
        QFETCH(QString, input);
        QFETCH(QValidator::State, expected);
        const auto expectedBool = expected == QValidator::State::Acceptable;
        const auto basePatterns = InputValidator::Pattern::Hostname;

        {
            QCOMPARE(InputValidator::isValid(input, basePatterns), expectedBool);
        }

        {
            InputValidator validator(basePatterns);
            int pos = 0;

            QCOMPARE(validator.validate(input, pos), expected);
        }
    }

    void testJid_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QValidator::State>("expected");

        QTest::newRow("Empty string") << QString() << QValidator::State::Intermediate;
        QTest::newRow("Non-empty string with dot") << QStringLiteral("abc.def") << QValidator::State::Intermediate;
        QTest::newRow("Non-empty string with hyphen") << QStringLiteral("abc-def") << QValidator::State::Intermediate;
        QTest::newRow("Non-empty string with space") << QStringLiteral("abc def") << QValidator::State::Invalid;
        QTest::newRow("Whitespaced string") << QStringLiteral("             ") << QValidator::State::Invalid;
        QTest::newRow("localhost") << QStringLiteral("localhost") << QValidator::State::Intermediate;
        QTest::newRow("192.168.1.1") << QStringLiteral("192.168.1.1") << QValidator::State::Intermediate;
        QTest::newRow("::1") << QStringLiteral("::1") << QValidator::State::Invalid;
        QTest::newRow("example.org") << QStringLiteral("example.org") << QValidator::State::Intermediate;
        QTest::newRow("xmpp.example.org") << QStringLiteral("xmpp.example.org") << QValidator::State::Intermediate;
        QTest::newRow("[::1]") << QStringLiteral("[::1]") << QValidator::State::Invalid;
        QTest::newRow("Hell\"oo") << QStringLiteral("Hell\"oo") << QValidator::State::Invalid;
        QTest::newRow("Hell&oo") << QStringLiteral("Hell&oo") << QValidator::State::Invalid;
        QTest::newRow("Hell'oo") << QStringLiteral("Hell'oo") << QValidator::State::Invalid;
        QTest::newRow("Hell/oo") << QStringLiteral("Hell/oo") << QValidator::State::Invalid;
        QTest::newRow("Hell:oo") << QStringLiteral("Hell:oo") << QValidator::State::Invalid;
        QTest::newRow("Hell<oo") << QStringLiteral("Hell<oo") << QValidator::State::Invalid;
        QTest::newRow("Hell>oo") << QStringLiteral("Hell>oo") << QValidator::State::Invalid;
        QTest::newRow("Hell@oo") << QStringLiteral("Hell@oo") << QValidator::State::Acceptable;
        QTest::newRow("Hell oo") << QStringLiteral("Hell oo") << QValidator::State::Invalid;

        QTest::newRow("elle a trouvé@perdu.com") << QStringLiteral("elle a trouvé@perdu.com") << QValidator::State::Invalid;
        QTest::newRow("elle a trouvé@pérdu.com") << QStringLiteral("elle a trouvé@pérdu.com") << QValidator::State::Invalid;
        QTest::newRow("elle a trouvé@pér du.com") << QStringLiteral("elle a trouvé@pér du.com") << QValidator::State::Invalid;
        QTest::newRow("elle a trouvé@") << QStringLiteral("elle a trouvé@") << QValidator::State::Invalid;

        QTest::newRow("elle_a_trouvé@perdu.com") << QStringLiteral("elle_a_trouvé@perdu.com") << QValidator::State::Acceptable;
        QTest::newRow("elle_a_trouvé@pérdu.com") << QStringLiteral("elle_a_trouvé@pérdu.com") << QValidator::State::Acceptable;
        QTest::newRow("elle_a_trouvé@pér du.com") << QStringLiteral("elle_a_trouvé@pér du.com") << QValidator::State::Intermediate;
        QTest::newRow("elle_a_trouvé@") << QStringLiteral("elle_a_trouvé@") << QValidator::State::Intermediate;

        QTest::newRow("alice@localhost") << QStringLiteral("alice@localhost") << QValidator::State::Acceptable;
        QTest::newRow("alice@192.168.1.22") << QStringLiteral("alice@192.168.1.22") << QValidator::State::Acceptable;
        QTest::newRow("alice@::1") << QStringLiteral("alice@::1") << QValidator::State::Acceptable;
    }

    void testJid()
    {
        QFETCH(QString, input);
        QFETCH(QValidator::State, expected);
        const auto expectedBool = expected == QValidator::State::Acceptable;
        const auto basePatterns = InputValidator::Pattern::Jid;

        {
            QCOMPARE(InputValidator::isValid(input, basePatterns), expectedBool);
        }

        {
            InputValidator validator(basePatterns);
            int pos = 0;

            QCOMPARE(validator.validate(input, pos), expected);
        }
    }

    void testPort_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QValidator::State>("expected");

        QTest::newRow("-1") << QString::number(-1) << QValidator::State::Invalid;
        QTest::newRow("0") << QString::number(0) << QValidator::State::Acceptable;
        QTest::newRow("1024") << QString::number(1024) << QValidator::State::Acceptable;
        QTest::newRow("65535") << QString::number(65535) << QValidator::State::Acceptable;
        QTest::newRow("65536") << QString::number(65536) << QValidator::State::Invalid;
        QTest::newRow("00") << QStringLiteral("00") << QValidator::State::Invalid;
        QTest::newRow("01024") << QStringLiteral("01024") << QValidator::State::Invalid;
        QTest::newRow("065535") << QStringLiteral("065535") << QValidator::State::Invalid;
        QTest::newRow("065536") << QStringLiteral("065536") << QValidator::State::Invalid;
    }

    void testPort()
    {
        QFETCH(QString, input);
        QFETCH(QValidator::State, expected);
        const auto expectedBool = expected == QValidator::State::Acceptable;
        const auto basePatterns = InputValidator::Pattern::Port;

        {
            QCOMPARE(InputValidator::isValid(input, basePatterns), expectedBool);
        }

        {
            InputValidator validator(basePatterns);
            int pos = 0;

            QCOMPARE(validator.validate(input, pos), expected);
        }
    }
};

QTEST_GUILESS_MAIN(InputValidatorTest)
#include "InputValidatorTest.moc"
