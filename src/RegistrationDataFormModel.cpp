// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RegistrationDataFormModel.h"
// Qt
#include <QString>
// QXmpp
#include <QXmppDataForm.h>

constexpr QStringView USERNAME = u"username";
constexpr QStringView PASSWORD = u"password";
constexpr QStringView EMAIL = u"email";

static const QList<QStringView> FILTERED_DATA_FORM_FIELDS =
    {u"FORM_TYPE", u"from", u"captcha-fallback-text", u"captcha-fallback-url", u"captchahidden", u"challenge", u"sid"};

RegistrationDataFormModel::RegistrationDataFormModel(QObject *parent)
    : DataFormModel(parent)
{
}

RegistrationDataFormModel::RegistrationDataFormModel(const QXmppDataForm &dataForm, QObject *parent)
    : DataFormModel(dataForm, parent)
{
}

bool RegistrationDataFormModel::hasUsernameField() const
{
    return usernameFieldIndex() != -1;
}

bool RegistrationDataFormModel::hasPasswordField() const
{
    return passwordFieldIndex() != -1;
}

bool RegistrationDataFormModel::hasEmailField() const
{
    return emailFieldIndex() != -1;
}

void RegistrationDataFormModel::setUsername(const QString &username)
{
    setData(index(usernameFieldIndex()), username, DataFormModel::Value);
}

void RegistrationDataFormModel::setPassword(const QString &password)
{
    setData(index(passwordFieldIndex()), password, DataFormModel::Value);
}

void RegistrationDataFormModel::setEmail(const QString &email)
{
    setData(index(emailFieldIndex()), email, DataFormModel::Value);
}

int RegistrationDataFormModel::usernameFieldIndex() const
{
    const QList<QXmppDataForm::Field> &fields = m_form.fields();
    for (int i = 0; i < fields.size(); i++) {
        if (fields.at(i).type() == QXmppDataForm::Field::TextSingleField && fields.at(i).key().compare(USERNAME, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }
    return -1;
}

int RegistrationDataFormModel::passwordFieldIndex() const
{
    const QList<QXmppDataForm::Field> &fields = m_form.fields();
    for (int i = 0; i < fields.size(); i++) {
        if (fields.at(i).type() == QXmppDataForm::Field::TextPrivateField && fields.at(i).key().compare(PASSWORD, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }
    return -1;
}

int RegistrationDataFormModel::emailFieldIndex() const
{
    const QList<QXmppDataForm::Field> &fields = m_form.fields();
    for (int i = 0; i < fields.size(); i++) {
        if (fields.at(i).type() == QXmppDataForm::Field::TextSingleField && fields.at(i).key().compare(EMAIL, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }
    return -1;
}

QXmppDataForm::Field RegistrationDataFormModel::extractUsernameField() const
{
    int index = usernameFieldIndex();
    return index >= 0 ? m_form.fields().at(index) : QXmppDataForm::Field();
}

QXmppDataForm::Field RegistrationDataFormModel::extractPasswordField() const
{
    int index = passwordFieldIndex();
    return index >= 0 ? m_form.fields().at(index) : QXmppDataForm::Field();
}

QXmppDataForm::Field RegistrationDataFormModel::extractEmailField() const
{
    int index = emailFieldIndex();
    return index >= 0 ? m_form.fields().at(index) : QXmppDataForm::Field();
}

QString RegistrationDataFormModel::extractUsername() const
{
    return extractUsernameField().value().toString();
}

QString RegistrationDataFormModel::extractPassword() const
{
    return extractPasswordField().value().toString();
}

QString RegistrationDataFormModel::extractEmail() const
{
    return extractEmailField().value().toString();
}

bool RegistrationDataFormModel::isFakeForm() const
{
    return m_isFakeForm;
}

void RegistrationDataFormModel::setIsFakeForm(bool isFakeForm)
{
    m_isFakeForm = isFakeForm;
}

QList<int> RegistrationDataFormModel::indexesToFilter() const
{
    QList<int> indexes;

    // username and password
    // email is currently not filtered because we do not have an extra email view
    for (const auto &index : {usernameFieldIndex(), passwordFieldIndex()}) {
        if (index != -1) {
            indexes << index;
        }
    }

    // search for other common fields to filter for
    for (int i = 0; i < m_form.fields().size(); i++) {
        QString key = m_form.fields().at(i).key();
        if (FILTERED_DATA_FORM_FIELDS.contains(key))
            indexes << i;
    }

    return indexes;
}

#include "moc_RegistrationDataFormModel.cpp"
