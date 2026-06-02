// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "InputValidator.h"

// Qt
#include <QStringLiteral>

using namespace Qt::StringLiterals;

#define RX_SINGLE(c) c
#define RX_BEGIN RX_SINGLE("^")
#define RX_END RX_SINGLE("$")
#define RX_USERNAME uR"([\w.-]+)"_s
#define RX_IPV4 uR"(((?:(?:25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\.){3}(?:25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)))"_s
#define RX_IPV6                                                                                                                                                \
    uR"(((?:[0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4}|(?:[0-9A-Fa-f]{1,4}:){1,7}:|(?:[0-9A-Fa-f]{1,4}:){1,6}:[0-9A-Fa-f]{1,4}|(?:[0-9A-Fa-f]{1,4}:){1,5}(?::[0-9A-Fa-f]{1,4}){1,2}|(?:[0-9A-Fa-f]{1,4}:){1,4}(?::[0-9A-Fa-f]{1,4}){1,3}|(?:[0-9A-Fa-f]{1,4}:){1,3}(?::[0-9A-Fa-f]{1,4}){1,4}|(?:[0-9A-Fa-f]{1,4}:){1,2}(?::[0-9A-Fa-f]{1,4}){1,5}|[0-9A-Fa-f]{1,4}:(?::[0-9A-Fa-f]{1,4}){1,6}|:(?::[0-9A-Fa-f]{1,4}){1,7}|::))"_s
#define RX_DOMAIN uR"((?=.{1,253}$)(?:(?!-)[\p{L}\p{N}](?:[\p{L}\p{N}-]{0,61}[\p{L}\p{N}])?\.)*(?!-)[\p{L}\p{N}](?:[\p{L}\p{N}-]{0,61}[\p{L}\p{N}])?)"_s
#define RX_HOSTNAME RX_SINGLE("(") RX_IPV4 RX_SINGLE("|") RX_IPV6 RX_SINGLE("|") RX_DOMAIN RX_SINGLE(")")

const QMap<InputValidator::Pattern, QString> &patternMap()
{
    using Pattern = InputValidator::Pattern;
    static const QMap<Pattern, QString> patterns{
        {Pattern::None, uR"(^.*$)"_s},
        {Pattern::Empty, uR"(^$)"_s},
        {Pattern::NotEmpty, uR"(^.+$)"_s},
        {Pattern::Username, RX_BEGIN RX_USERNAME RX_END},
        {Pattern::Hostname, RX_BEGIN RX_HOSTNAME RX_END},
        {Pattern::Jid, RX_BEGIN RX_USERNAME RX_SINGLE("@") RX_HOSTNAME RX_END},
        // 0 - 65535
        {Pattern::Port, uR"(^(?:|0|[1-9]\d{0,3}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5])$)"_s},
    };
    return patterns;
}

InputValidator::InputValidator(QObject *parent)
    : QRegularExpressionValidator(parent)
{
}

InputValidator::InputValidator(Patterns patterns, QObject *parent)
    : QRegularExpressionValidator(parent)
{
    setPatterns(patterns);
}

QValidator::State InputValidator::validate(QString &input, int &pos) const
{
    const auto result = QRegularExpressionValidator::validate(input, pos);

    if (m_state != result) {
        m_state = result;
        Q_EMIT const_cast<InputValidator *>(this)->stateChanged(m_state);
    }

    return result;
}

InputValidator::Patterns InputValidator::patterns() const
{
    return m_patterns;
}

void InputValidator::setPatterns(Patterns patterns)
{
    if (m_patterns != patterns) {
        m_patterns = patterns;
        Q_EMIT patternsChanged(m_patterns);

        setRegularExpression(regularExpression(m_patterns));
    }
}

QValidator::State InputValidator::state() const
{
    return m_state;
}

QRegularExpression InputValidator::regularExpression(Patterns patterns)
{
    const auto &patternMap = ::patternMap();
    QRegularExpression rx;
    QStringList entries;

    entries.reserve(patternMap.count());

    if (patterns == Pattern::None) {
        entries.append(*patternMap.constFind(Pattern::None));
    } else {
        for (auto it = patternMap.constBegin(); it != patternMap.constEnd(); ++it) {
            if (patterns.testFlag(it.key())) {
                entries.append(it.value());
            }
        }
    }

    rx.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);

    if (!entries.isEmpty()) {
        rx.setPattern(entries.join(QLatin1Char('|')));
    }

    return rx;
}

bool InputValidator::isValid(const QString &subject, Patterns patterns)
{
    const auto rx = regularExpression(patterns);
    const auto match = rx.match(subject);

    return match.hasMatch();
}

#include "moc_InputValidator.cpp"
