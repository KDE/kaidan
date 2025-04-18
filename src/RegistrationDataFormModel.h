// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "DataFormModel.h"

/**
 * This class is used to store the data of the registration form.
 */
class RegistrationDataFormModel : public DataFormModel
{
    Q_OBJECT

public:
    explicit RegistrationDataFormModel(QObject *parent = nullptr);
    explicit RegistrationDataFormModel(const QXmppDataForm &dataForm, QObject *parent = nullptr);

    Q_INVOKABLE bool hasUsernameField() const;
    Q_INVOKABLE bool hasPasswordField() const;
    Q_INVOKABLE bool hasEmailField() const;

    Q_INVOKABLE void setUsername(const QString &username);
    Q_INVOKABLE void setPassword(const QString &password);
    Q_INVOKABLE void setEmail(const QString &email);

    int usernameFieldIndex() const;
    int passwordFieldIndex() const;
    int emailFieldIndex() const;

    QXmppDataForm::Field extractUsernameField() const;
    QXmppDataForm::Field extractPasswordField() const;
    QXmppDataForm::Field extractEmailField() const;

    QString extractUsername() const;
    QString extractPassword() const;
    QString extractEmail() const;

    bool isFakeForm() const;
    void setIsFakeForm(bool isFakeForm);

    QList<int> indexesToFilter() const;

private:
    bool m_isFakeForm = false;
};
