// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "ProviderListItem.h"

class ProviderListModel : public QAbstractListModel
{
	Q_OBJECT

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
		OnlineSinceRole,
		HttpUploadSizeRole,
		MessageStorageDurationRole
	};
	Q_ENUM(Role)

	struct Locale {
		QString languageCode;
		QString countryCode;
	};

	explicit ProviderListModel(QObject *parent = nullptr);

	QHash<int, QByteArray> roleNames() const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	// overloaded method for QML
	Q_INVOKABLE QVariant data(int row, ProviderListModel::Role role) const;
	Q_INVOKABLE ProviderListItem provider(const QString &jid) const;
	Q_INVOKABLE ProviderListItem providerFromBareJid(const QString &jid) const;

	Q_INVOKABLE int randomlyChooseIndex() const;

private:
	void readItemsFromJsonFile(const QString &filePath);
	QVector<ProviderListItem> providersSupportingInBandRegistration() const;
	QVector<ProviderListItem> providersWithSystemLocale(const QVector<ProviderListItem> &preSelectedProviders) const;
	int indexOfRandomlySelectedProvider(const QVector<ProviderListItem> &preSelectedProviders) const;

	QString systemLanguageCode() const;
	QString systemCountryCode() const;
	Locale systemLocale() const;

	QVector<ProviderListItem> m_items;
};
