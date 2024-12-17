// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QList>

#include "ProviderListItem.h"

class ProviderListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int providersMatchingSystemLocaleMinimumCount READ providersMatchingSystemLocaleMinimumCount CONSTANT)

public:
    enum Role {
        DisplayRole = Qt::DisplayRole,
        JidRole = Qt::UserRole + 1,
        SupportsInBandRegistrationRole,
        RegistrationWebPageRole,
        LanguagesRole,
        CountriesRole,
        FlagsRole,
        IsCustomProviderRole,
        WebsiteRole,
        sinceRole,
        HttpUploadSizeRole,
        MessageStorageDurationRole
    };
    Q_ENUM(Role)

    static int providersMatchingSystemLocaleMinimumCount();

    explicit ProviderListModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // overloaded method for QML
    Q_INVOKABLE QVariant data(int row, ProviderListModel::Role role) const;
    Q_INVOKABLE ProviderListItem provider(const QString &jid) const;
    Q_INVOKABLE ProviderListItem providerFromBareJid(const QString &jid) const;

    Q_INVOKABLE int randomlyChooseIndex(const QList<int> &excludedIndexes = {}, bool providersMatchingSystemLocaleOnly = true) const;

private:
    void readItemsFromJsonFile(const QString &filePath);
    QList<ProviderListItem> providersSupportingInBandRegistration() const;
    QList<ProviderListItem> providersWithSystemLocale(const QList<ProviderListItem> &preSelectedProviders) const;
    int indexOfRandomlySelectedProvider(const QList<ProviderListItem> &preSelectedProviders, const QList<int> &excludedIndexes) const;

    QList<ProviderListItem> m_items;
};
