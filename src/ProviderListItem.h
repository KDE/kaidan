/*
*  Kaidan - A user-friendly XMPP client for every device!
*
*  Copyright (C) 2016-2023 Kaidan developers and contributors
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

#pragma once

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
