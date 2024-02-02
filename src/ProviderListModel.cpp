// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ProviderListModel.h"
// std
// Qt
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "ProviderListItem.h"
#include "Globals.h"

ProviderListModel::ProviderListModel(QObject *parent)
	: QAbstractListModel(parent)
{
	ProviderListItem customProvider(true);
	customProvider.setJid(tr("Custom provider"));
	m_items << customProvider;

	readItemsFromJsonFile(PROVIDER_LIST_FILE_PATH);
}

QHash<int, QByteArray> ProviderListModel::roleNames() const
{
	return {
		{DisplayRole, QByteArrayLiteral("display")},
		{JidRole, QByteArrayLiteral("jid")},
		{SupportsInBandRegistrationRole, QByteArrayLiteral("supportsInBandRegistration")},
		{RegistrationWebPageRole, QByteArrayLiteral("registrationWebPage")},
		{LanguagesRole, QByteArrayLiteral("languages")},
		{CountriesRole, QByteArrayLiteral("countries")},
		{FlagsRole, QByteArrayLiteral("flags")},
		{IsCustomProviderRole, QByteArrayLiteral("isCustomProvider")},
		{WebsiteRole, QByteArrayLiteral("website")},
		{HttpUploadSizeRole, QByteArrayLiteral("httpUploadSize")},
		{MessageStorageDurationRole, QByteArrayLiteral("messageStorageDuration")}
	};
}

int ProviderListModel::rowCount(const QModelIndex &parent) const
{
	// For list models, only the root node (an invalid parent) should return the list's size.
	// For all other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
	if (parent.isValid())
		return 0;

	return m_items.size();
}

QVariant ProviderListModel::data(const QModelIndex &index, int role) const
{
	Q_ASSERT(checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::ParentIsInvalid));

	const ProviderListItem &item = m_items.at(index.row());

	switch (role) {
	case DisplayRole:
		return QStringLiteral("%1 %2").arg(QStringList(item.flags().toList()).join(u' '), item.jid());
	case JidRole:
		return item.jid();
	case SupportsInBandRegistrationRole:
		return item.supportsInBandRegistration();
	case RegistrationWebPageRole:
		return item.registrationWebPages().pickBySystemLocale();
	case LanguagesRole:
		return QStringList(item.languages().toList()).join(QStringLiteral(", "));
	case CountriesRole:
		return QStringList(item.countries().toList()).join(QStringLiteral(", "));
	case FlagsRole:
		return QStringList(item.flags().toList()).join(QStringLiteral(", "));
	case IsCustomProviderRole:
		return item.isCustomProvider();
	case WebsiteRole:
		return item.websites().pickBySystemLocale();
	case OnlineSinceRole:
		if (item.onlineSince() == -1)
			return QString();
		return QString::number(item.onlineSince());
	case HttpUploadSizeRole:
		switch (item.httpUploadSize()) {
		case -1:
			return QString();
		case 0:
			//: Unlimited file size for uploading
			return tr("Unlimited");
		default:
			return QLocale::system().formattedDataSize(item.httpUploadSize() * 1000 * 1000, 0, QLocale::DataSizeSIFormat);
		}
	case MessageStorageDurationRole:
		switch (item.messageStorageDuration()) {
		case -1:
			return QString();
		case 0:
			//: Deletion of message history stored on provider
			return tr("Unlimited");
		default:
			return tr("%1 days").arg(item.messageStorageDuration());
		}
	}

	return {};
}

QVariant ProviderListModel::data(int row, ProviderListModel::Role role) const
{
	return data(index(row), role);
}

ProviderListItem ProviderListModel::provider(const QString &jid) const
{
	auto item = std::find_if(m_items.begin(), m_items.end(), [&jid](const auto &it) {
		return it.jid() == jid;
	});

	return item == m_items.end() ? ProviderListItem(true) : *item;
}

ProviderListItem ProviderListModel::providerFromBareJid(const QString &jid) const
{
	return provider(QXmppUtils::jidToDomain(jid));
}

int ProviderListModel::randomlyChooseIndex() const
{
	QVector<ProviderListItem> providersWithInBandRegistration = providersSupportingInBandRegistration();
	QVector<ProviderListItem> providersWithInBandRegistrationAndSystemLocale = providersWithSystemLocale(providersWithInBandRegistration);

	if (providersWithInBandRegistrationAndSystemLocale.size() < PROVIDER_LIST_MIN_PROVIDERS_FROM_COUNTRY)
		return indexOfRandomlySelectedProvider(providersWithInBandRegistration);

	return indexOfRandomlySelectedProvider(providersWithInBandRegistrationAndSystemLocale);
}

void ProviderListModel::readItemsFromJsonFile(const QString &filePath)
{
	QFile file(filePath);
	if (!file.exists()) {
		qWarning() << "[ProviderListModel] Could not parse provider list:"
				   << filePath
				   << "- file does not exist!";
		return;
	}

	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "[ProviderListModel] Could not open file for reading:" << filePath;
		return;
	}

	QByteArray content = file.readAll();

	QJsonParseError parseError;
	QJsonArray jsonProviderArray = QJsonDocument::fromJson(content, &parseError).array();
	if (jsonProviderArray.isEmpty()) {
		qWarning() << "[ProviderListModel] Could not parse provider list JSON file or no providers defined.";
		qWarning() << "[ProviderListModel] QJsonParseError:" << parseError.errorString() << "at" << parseError.offset;
		return;
	}

	for (auto jsonProviderItem : jsonProviderArray) {
		if (!jsonProviderItem.isNull() && jsonProviderItem.isObject())
			m_items << ProviderListItem::fromJson(jsonProviderItem.toObject());
	}

	// Sort the parsed providers.
	// The first item ("Custom provider") is not sorted.
	if (m_items.size() > 1)
		std::sort(m_items.begin() + 1, m_items.end());
}

QVector<ProviderListItem> ProviderListModel::providersSupportingInBandRegistration() const
{
	QVector<ProviderListItem> providers;

	// The search starts at index 1 to exclude the custom provider.
	std::copy_if(m_items.begin() + 1, m_items.end(), std::back_inserter(providers), [](const ProviderListItem &item) {
		return item.supportsInBandRegistration();
	});

	return providers;
}

QVector<ProviderListItem> ProviderListModel::providersWithSystemLocale(const QVector<ProviderListItem> &preSelectedProviders) const
{
	QVector<ProviderListItem> providers;

	for (const auto &provider : preSelectedProviders) {
		if (provider.languages().contains(systemLocale().languageCode) && provider.countries().contains(systemLocale().countryCode))
			providers << provider;
	}

	return providers;
}

int ProviderListModel::indexOfRandomlySelectedProvider(const QVector<ProviderListItem> &preSelectedProviders) const
{
	return m_items.indexOf(preSelectedProviders.at(QRandomGenerator::global()->generate() % preSelectedProviders.size()));
}

ProviderListModel::Locale ProviderListModel::systemLocale() const
{
	const auto systemLocaleParts = QLocale::system().name().split(QStringLiteral("_"));

	// Use the retrieved codes or default values if there are not two codes.
	// An example for usage of the default values is when the "C" locale is
	// retrieved.
	if (systemLocaleParts.size() >= 2) {
		return {
			systemLocaleParts.first().toUpper(),
			systemLocaleParts.last()
		};
	}
	return {
		DEFAULT_LANGUAGE_CODE.toString(),
		DEFAULT_COUNTRY_CODE.toString()
	};
}
