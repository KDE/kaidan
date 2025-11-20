// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>

class Provider;

class ProviderModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int providersMatchingSystemLocaleMinimumCount READ providersMatchingSystemLocaleMinimumCount CONSTANT)
    Q_PROPERTY(int maximumBusFactor MEMBER m_maximumBusFactor CONSTANT)
    Q_PROPERTY(QList<QString> availableFlags MEMBER m_availableFlags CONSTANT)
    Q_PROPERTY(QList<QString> availableOrganizations MEMBER m_availableOrganizations CONSTANT)

public:
    enum class Role {
        Display = Qt::DisplayRole,
        IsCustomProvider = Qt::UserRole + 1,
        Jid,
        SupportsInBandRegistration,
        RegistrationWebPage,
        Languages,
        Flags,
        FlagsText,
        Website,
        BusFactor,
        Organization,
        SupportsPasswordReset,
        HostedGreen,
        Since,
        SinceText,
        HttpUploadFileSize,
        HttpUploadFileSizeText,
        HttpUploadTotalSize,
        HttpUploadStorageDuration,
        MessageStorageDuration,
        MessageStorageDurationText,
        FreeOfCharge,
    };
    Q_ENUM(Role)

    explicit ProviderModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Overloaded method for ProviderFilterModel.
    QVariant data(const QModelIndex &index, ProviderModel::Role role) const;

    // Overloaded method for QML.
    Q_INVOKABLE QVariant data(int row, ProviderModel::Role role) const;

    Q_INVOKABLE Provider providerFromBareJid(const QString &jid) const;

    Q_INVOKABLE int randomlyChooseIndex(const QList<int> &excludedIndexes = {}, bool providersMatchingSystemLocaleOnly = true) const;
    static int providersMatchingSystemLocaleMinimumCount();

private:
    void readItemsFromJsonFile(const QString &filePath);

    void initializeAvailableFlags();
    void initializeMaximumBusFactor();
    void initializeAvailableOrganizations();

    QList<Provider> providersSupportingInBandRegistration() const;
    QList<Provider> providersWithSystemLocale(const QList<Provider> &preSelectedProviders) const;

    Provider provider(const QString &jid) const;
    int indexOfRandomlySelectedProvider(const QList<Provider> &preSelectedProviders, const QList<int> &excludedIndexes) const;

    QList<Provider> m_providers;
    QList<QString> m_availableFlags;
    int m_maximumBusFactor;
    QList<QString> m_availableOrganizations;
};
