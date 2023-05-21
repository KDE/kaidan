// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <unordered_map>
// Qt
#include <QSharedDataPointer>
#include <QVariantList>

class QUrl;
class QJsonObject;

class ProviderListItemPrivate;

class ProviderListItem
{
	Q_GADGET
public:
	Q_PROPERTY(QString jid READ jid CONSTANT)
	Q_PROPERTY(QMap<QString, QUrl> websites READ websites CONSTANT)
	Q_PROPERTY(QVector<QString> chatSupport READ chatSupport CONSTANT)
	Q_PROPERTY(QVector<QString> groupChatSupport READ groupChatSupport CONSTANT)

	static ProviderListItem fromJson(const QJsonObject &object);

	ProviderListItem(bool isCustomProvider = false);
	ProviderListItem(const ProviderListItem &other);
	~ProviderListItem();

	ProviderListItem &operator=(const ProviderListItem &other);

	bool isCustomProvider() const;
	void setIsCustomProvider(bool isCustomProvider);

	QString jid() const;
	void setJid(const QString &jid);

	bool supportsInBandRegistration() const;
	void setSupportsInBandRegistration(bool supportsInBandRegistration);

	QUrl registrationWebPage() const;
	void setRegistrationWebPage(const QUrl &registrationWebPage);

	QVector<QString> languages() const;

	QVector<QString> countries() const;
	void setCountries(const QVector<QString> &country);

	QVector<QString> flags() const;

	QMap<QString, QUrl> websites() const;
	void setWebsites(const QMap<QString, QUrl> &websites);

	int onlineSince() const;
	void setOnlineSince(int onlineSince);

	int httpUploadSize() const;
	void setHttpUploadSize(int httpUploadSize);

	int messageStorageDuration() const;
	void setMessageStorageDuration(int messageStorageDuration);

	QVector<QString> chatSupport() const;
	void setChatSupport(std::unordered_map<QString, QVector<QString>> &&chatSupport);

	QVector<QString> groupChatSupport() const;
	void setGroupChatSupport(std::unordered_map<QString, QVector<QString>> &&groupChatSupport);

	bool operator<(const ProviderListItem &other) const;
	bool operator>(const ProviderListItem &other) const;
	bool operator<=(const ProviderListItem &other) const;
	bool operator>=(const ProviderListItem &other) const;

	bool operator==(const ProviderListItem &other) const;

private:
	QSharedDataPointer<ProviderListItemPrivate> d;
};
