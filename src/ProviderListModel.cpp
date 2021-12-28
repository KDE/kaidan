/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ProviderListModel.h"
// Qt
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
// Kaidan
#include "ProviderListItem.h"
#include "Globals.h"
#include "QmlUtils.h"

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
		{LanguageRole, QByteArrayLiteral("language")},
		{CountryRole, QByteArrayLiteral("country")},
		{FlagRole, QByteArrayLiteral("flag")},
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
		return QStringLiteral("%1 %2").arg(item.flag(), item.jid());
	case JidRole:
		return item.jid();
	case SupportsInBandRegistrationRole:
		return item.supportsInBandRegistration();
	case RegistrationWebPageRole:
		return item.registrationWebPage();
	case LanguageRole:
		return item.language();
	case CountryRole:
		return item.country();
	case FlagRole:
		return item.flag();
	case IsCustomProviderRole:
		return item.isCustomProvider();
	case WebsiteRole:
		return QmlUtils::formatMessage(item.website().toString());
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
			return QLocale::system().formattedDataSize(item.httpUploadSize() * 1024LL * 1024LL, 0);
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

int ProviderListModel::randomlyChooseIndex() const
{
	QVector<ProviderListItem> providersWithInBandRegistration = providersSupportingInBandRegistration();

	QString systemCountryCode = QLocale::system().name().split(QStringLiteral("_")).last();
	QVector<ProviderListItem> providersWithInBandRegistrationFromCountry = providersFromCountry(providersWithInBandRegistration, systemCountryCode);

	if (providersWithInBandRegistrationFromCountry.size() < PROVIDER_LIST_MIN_PROVIDERS_FROM_COUNTRY)
		return indexOfRandomlySelectedProvider(providersWithInBandRegistration);

	return indexOfRandomlySelectedProvider(providersWithInBandRegistrationFromCountry);
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

QVector<ProviderListItem> ProviderListModel::providersFromCountry(const QVector<ProviderListItem> &preSelectedProviders, const QString &country) const
{
	QVector<ProviderListItem> providers;

	for (const auto &provider : preSelectedProviders) {
		if (provider.country() == country)
			providers << provider;
	}

	return providers;
}

int ProviderListModel::indexOfRandomlySelectedProvider(const QVector<ProviderListItem> &preSelectedProviders) const
{
	return m_items.indexOf(preSelectedProviders.at(QRandomGenerator::global()->generate() % preSelectedProviders.size()));
}
