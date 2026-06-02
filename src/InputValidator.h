// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QRegularExpressionValidator>

class InputValidator : public QRegularExpressionValidator
{
    Q_OBJECT

    Q_PROPERTY(InputValidator::Patterns patterns READ patterns WRITE setPatterns NOTIFY patternsChanged)
    Q_PROPERTY(QValidator::State state READ state NOTIFY stateChanged)

public:
    enum class Pattern {
        None = 0,
        Empty = 1 << 0,
        NotEmpty = 1 << 1,
        Username = 1 << 2,
        Hostname = 1 << 3,
        Jid = 1 << 4,
        Port = 1 << 5,
    };
    Q_ENUM(Pattern)
    Q_DECLARE_FLAGS(Patterns, Pattern)
    Q_FLAG(Patterns)

    explicit InputValidator(QObject *parent = nullptr);
    explicit InputValidator(Patterns patterns, QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &pos) const override;

    Patterns patterns() const;
    void setPatterns(Patterns patterns);
    Q_SIGNAL void patternsChanged(InputValidator::Patterns patterns);

    QValidator::State state() const;
    Q_SIGNAL void stateChanged(QValidator::State state);

    static QRegularExpression regularExpression(Patterns patterns);
    static bool isValid(const QString &subject, Patterns patterns);

private:
    Patterns m_patterns = Pattern::None;
    mutable QValidator::State m_state = QValidator::State::Acceptable;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(InputValidator::Patterns)
