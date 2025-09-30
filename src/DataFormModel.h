// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
#include <QHash>
// QXmpp
#include <QXmppDataForm.h>

class DataFormModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY formChanged)
    Q_PROPERTY(QString instructions READ instructions NOTIFY formChanged)

public:
    enum DataFormFieldType : quint8 {
        BooleanField,
        FixedField,
        HiddenField,
        JidMultiField,
        JidSingleField,
        ListMultiField,
        ListSingleField,
        TextMultiField,
        TextPrivateField,
        TextSingleField
    };
    Q_ENUM(DataFormFieldType)

    enum FormRoles {
        Key = Qt::UserRole + 1,
        Type,
        Label,
        IsRequired,
        Value,
        Description,
        MediaUrl,
    };

    explicit DataFormModel(QObject *parent = nullptr);
    explicit DataFormModel(const QXmppDataForm &dataForm, QObject *parent = nullptr);
    ~DataFormModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    QXmppDataForm form() const;
    void setForm(const QXmppDataForm &form);

    QString title() const;
    QString instructions() const;

Q_SIGNALS:
    void formChanged();

protected:
    QXmppDataForm m_form;

private:
    QUrl mediaSourceUri(const QXmppDataForm::Field &field) const;
};
