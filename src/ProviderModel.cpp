// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ProviderModel.h"

// Qt
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Algorithms.h"
#include "Globals.h"
#include "KaidanCoreLog.h"
#include "Provider.h"
#include "QmlUtils.h"

ProviderModel::ProviderModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_providers.append(Provider{});
    readItemsFromJsonFile(PROVIDER_LIST_FILE_PATH);
    initializeAvailableFlags();
    initializeMaximumBusFactor();
    initializeAvailableOrganizations();
}

QHash<int, QByteArray> ProviderModel::roleNames() const
{
    return {{static_cast<int>(Role::Display), QByteArrayLiteral("display")},
            {static_cast<int>(Role::IsCustomProvider), QByteArrayLiteral("isCustomProvider")},
            {static_cast<int>(Role::Jid), QByteArrayLiteral("jid")},
            {static_cast<int>(Role::SupportsInBandRegistration), QByteArrayLiteral("supportsInBandRegistration")},
            {static_cast<int>(Role::RegistrationWebPage), QByteArrayLiteral("registrationWebPage")},
            {static_cast<int>(Role::Languages), QByteArrayLiteral("languages")},
            {static_cast<int>(Role::Flags), QByteArrayLiteral("flags")},
            {static_cast<int>(Role::FlagsText), QByteArrayLiteral("flagsText")},
            {static_cast<int>(Role::Website), QByteArrayLiteral("website")},
            {static_cast<int>(Role::BusFactor), QByteArrayLiteral("busFactor")},
            {static_cast<int>(Role::Organization), QByteArrayLiteral("organization")},
            {static_cast<int>(Role::SupportsPasswordReset), QByteArrayLiteral("supportsPasswordReset")},
            {static_cast<int>(Role::HostedGreen), QByteArrayLiteral("hostedGreen")},
            {static_cast<int>(Role::Since), QByteArrayLiteral("since")},
            {static_cast<int>(Role::SinceText), QByteArrayLiteral("sinceText")},
            {static_cast<int>(Role::HttpUploadFileSize), QByteArrayLiteral("httpUploadFileSize")},
            {static_cast<int>(Role::HttpUploadFileSizeText), QByteArrayLiteral("httpUploadFileSizeText")},
            {static_cast<int>(Role::HttpUploadTotalSize), QByteArrayLiteral("httpUploadTotalSize")},
            {static_cast<int>(Role::HttpUploadStorageDuration), QByteArrayLiteral("httpUploadStorageDuration")},
            {static_cast<int>(Role::MessageStorageDuration), QByteArrayLiteral("messageStorageDuration")},
            {static_cast<int>(Role::MessageStorageDurationText), QByteArrayLiteral("messageStorageDurationText")},
            {static_cast<int>(Role::FreeOfCharge), QByteArrayLiteral("freeOfCharge")}};
}

int ProviderModel::rowCount(const QModelIndex &parent) const
{
    return parent == QModelIndex() ? m_providers.size() : 0;
}

QVariant ProviderModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::ParentIsInvalid));

    const Provider &item = m_providers.at(index.row());

    switch (static_cast<Role>(role)) {
    case Role::Display:
        return item.isCustomProvider() ? tr("Custom provider") : item.jid();
    case Role::IsCustomProvider:
        return item.isCustomProvider();
    case Role::Jid:
        return item.jid();
    case Role::SupportsInBandRegistration:
        return item.supportsInBandRegistration();
    case Role::RegistrationWebPage:
        return item.registrationWebPages().pickBySystemLocale();
    case Role::Languages:
        return item.languages().join(u" | ");
    case Role::Flags:
        return item.flags();
    case Role::FlagsText:
        return item.flags().join(u" | ");
    case Role::Website:
        return item.websites().pickBySystemLocale();
    case Role::BusFactor:
        return item.busFactor();
    case Role::Organization:
        return item.organization();
    case Role::SupportsPasswordReset:
        return item.supportsPasswordReset();
    case Role::HostedGreen:
        return item.hostedGreen();
    case Role::Since:
        return item.since();
    case Role::SinceText:
        if (const auto since = item.since(); since.isValid()) {
            return QLocale::system().toString(since, QLocale::ShortFormat);
        }

        return QString();
    case Role::HttpUploadFileSize:
        return item.httpUploadFileSize();
    case Role::HttpUploadFileSizeText:
        switch (item.httpUploadFileSize()) {
        case -1:
            return QString();
        case 0:
            //: Unlimited file size for uploading
            return tr("Unlimited");
        default:
            return QmlUtils::formattedDataSize(item.httpUploadFileSize() * 1000 * 1000);
        }
    case Role::HttpUploadTotalSize:
        return item.httpUploadTotalSize();
    case Role::HttpUploadStorageDuration:
        return item.httpUploadStorageDuration();
    case Role::MessageStorageDuration:
        return item.messageStorageDuration();
    case Role::MessageStorageDurationText:
        switch (item.messageStorageDuration()) {
        case -1:
            return QString();
        case 0:
            //: Deletion of message history stored on provider
            return tr("Unlimited");
        default:
            return tr("%1 days").arg(item.messageStorageDuration());
        }
    case Role::FreeOfCharge:
        return item.freeOfCharge();
    }

    return {};
}

QVariant ProviderModel::data(const QModelIndex &index, Role role) const
{
    return data(index, static_cast<int>(role));
}

QVariant ProviderModel::data(int row, ProviderModel::Role role) const
{
    return data(index(row), static_cast<int>(role));
}

Provider ProviderModel::providerFromBareJid(const QString &jid) const
{
    return provider(QXmppUtils::jidToDomain(jid));
}

int ProviderModel::randomlyChooseIndex(const QList<int> &excludedIndexes, bool providersMatchingSystemLocaleOnly) const
{
    QList<Provider> providersWithInBandRegistration = providersSupportingInBandRegistration();

    if (providersMatchingSystemLocaleOnly) {
        QList<Provider> providersWithInBandRegistrationAndSystemLocale = providersWithSystemLocale(providersWithInBandRegistration);

        if (providersWithInBandRegistrationAndSystemLocale.size() < PROVIDER_LIST_MIN_PROVIDERS_FROM_COUNTRY) {
            return indexOfRandomlySelectedProvider(providersWithInBandRegistration, excludedIndexes);
        }

        return indexOfRandomlySelectedProvider(providersWithInBandRegistrationAndSystemLocale, excludedIndexes);
    }

    return indexOfRandomlySelectedProvider(providersWithInBandRegistration, excludedIndexes);
}

int ProviderModel::providersMatchingSystemLocaleMinimumCount()
{
    return PROVIDER_LIST_MIN_PROVIDERS_FROM_COUNTRY;
}

void ProviderModel::readItemsFromJsonFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        qCWarning(KAIDAN_CORE_LOG) << "Could not parse provider list:" << filePath << "- file does not exist!";
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(KAIDAN_CORE_LOG) << "Could not open file for reading:" << filePath;
        return;
    }

    QByteArray content = file.readAll();

    QJsonParseError parseError;
    QJsonArray jsonProviderArray = QJsonDocument::fromJson(content, &parseError).array();
    if (jsonProviderArray.isEmpty()) {
        qCWarning(KAIDAN_CORE_LOG) << "Could not parse provider list JSON file or no providers defined.";
        qCWarning(KAIDAN_CORE_LOG) << "QJsonParseError:" << parseError.errorString() << "at" << parseError.offset;
        return;
    }

    for (auto jsonProvider : jsonProviderArray) {
        if (!jsonProvider.isNull() && jsonProvider.isObject()) {
            m_providers.append(Provider(jsonProvider.toObject()));
        }
    }
}

void ProviderModel::initializeAvailableFlags()
{
    std::ranges::for_each(transform(m_providers,
                                    [](const Provider &provider) {
                                        return provider.flags();
                                    }),
                          [this](const QList<QString> &flags) {
                              m_availableFlags.append(flags);
                          });

    makeUnique(m_availableFlags);
    m_availableFlags.prepend(NO_SELECTION_TEXT);
}

void ProviderModel::initializeMaximumBusFactor()
{
    m_maximumBusFactor = std::ranges::max(transform(m_providers, [](const Provider &provider) {
        return provider.busFactor();
    }));
}

void ProviderModel::initializeAvailableOrganizations()
{
    m_availableOrganizations = transformFilter(m_providers, [](const Provider &provider) -> std::optional<QString> {
        if (const auto organization = provider.organization(); !organization.isEmpty()) {
            return organization;
        }

        return {};
    });

    makeUnique(m_availableOrganizations);
    m_availableOrganizations.prepend(NO_SELECTION_TEXT);
}

QList<Provider> ProviderModel::providersSupportingInBandRegistration() const
{
    QList<Provider> providers;

    // The search starts at index 1 to exclude the custom provider.
    std::copy_if(m_providers.begin() + 1, m_providers.end(), std::back_inserter(providers), [](const Provider &item) {
        return item.supportsInBandRegistration();
    });

    return providers;
}

QList<Provider> ProviderModel::providersWithSystemLocale(const QList<Provider> &preSelectedProviders) const
{
    QList<Provider> providers;

    for (const auto &provider : preSelectedProviders) {
        if (const auto systemLocale = SystemUtils::systemLocaleCodes();
            provider.languages().contains(systemLocale.languageCode) && provider.countries().contains(systemLocale.countryCode)) {
            providers << provider;
        }
    }

    return providers;
}

Provider ProviderModel::provider(const QString &jid) const
{
    auto item = std::ranges::find_if(m_providers, [&jid](const auto &it) {
        return it.jid() == jid;
    });

    return item == m_providers.end() ? Provider{} : *item;
}

int ProviderModel::indexOfRandomlySelectedProvider(const QList<Provider> &preSelectedProviders, const QList<int> &excludedIndexes) const
{
    int index;

    do {
        index = m_providers.indexOf(preSelectedProviders.at(QRandomGenerator::global()->generate() % preSelectedProviders.size()));
    } while (!excludedIndexes.isEmpty() && excludedIndexes.contains(index));

    return index;
}

#include "moc_ProviderModel.cpp"
