// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ProviderListItem.h"
// Qt
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
// Kaidan
#include "Algorithms.h"

#define REGIONAL_INDICATOR_SYMBOL_BASE 0x1F1A5

template<typename T>
struct LanguageMap
{
	T pickBySystemLocale() const
	{
		auto languageCode = QLocale::system().name().split(u'_').first();

		// Use the system-wide language chat support jid if available.
		auto resultItr = values.find(languageCode);
		if (resultItr != values.end() && !resultItr->second.isEmpty()) {
			return resultItr->second;
		}

		// Use the English chat support jid if no system-wide language chat support jid is available but English is.
		resultItr = values.find(QStringLiteral("EN"));
		if (resultItr != values.end() && !resultItr->second.isEmpty()) {
			return resultItr->second;
		}

		// Use the first chat support jid if also no English version is available but another one is.
		if (!values.empty()) {
			return values.begin()->second;
		}

		return {};
	}

	std::unordered_map<QString, T> values;
};

class ProviderListItemPrivate : public QSharedData
{
public:
	ProviderListItemPrivate();

	bool isCustomProvider;
	QString jid;
	bool supportsInBandRegistration;
	QUrl registrationWebPage;
	QVector<QString> languages;
	QVector<QString> countries;
	QMap<QString, QUrl> websites;
	int onlineSince;
	int httpUploadSize;
	int messageStorageDuration;
	LanguageMap<QVector<QString>> chatSupport;
	LanguageMap<QVector<QString>> groupChatSupport;
};

ProviderListItemPrivate::ProviderListItemPrivate()
	: isCustomProvider(false), supportsInBandRegistration(false), onlineSince(-1), httpUploadSize(-1), messageStorageDuration(-1)
{
}

ProviderListItem ProviderListItem::fromJson(const QJsonObject &object)
{
	ProviderListItem item;
	item.setIsCustomProvider(false);
	item.setJid(object.value(QLatin1String("jid")).toString());
	item.setSupportsInBandRegistration(object.value(QLatin1String("inBandRegistration")).toBool());
	item.setRegistrationWebPage(QUrl(object.value(QLatin1String("registrationWebPage")).toString()));

	const auto serverLocations = object.value(QLatin1String("serverLocations")).toArray();
	QVector<QString> countries;
	for (const auto &country : serverLocations) {
		countries.append(country.toString().toUpper());
	}
	item.setCountries(countries);

	const auto websiteLanguageVersions = object.value(QLatin1String("website")).toObject();
	QMap<QString, QUrl> websites;
	for (auto itr = websiteLanguageVersions.constBegin(); itr != websiteLanguageVersions.constEnd(); ++itr) {
		const auto language = itr.key().toUpper();
		const QUrl url = { itr.value().toString() };
		websites.insert(language, url);
	}
	item.setWebsites(websites);

	item.setOnlineSince(object.value(QLatin1String("since")).toInt(-1));
	item.setHttpUploadSize(object.value(QLatin1String("maximumHttpFileUploadFileSize")).toInt(-1));
	item.setMessageStorageDuration(object.value(QLatin1String("maximumMessageArchiveManagementStorageTime")).toInt(-1));

	const auto chatSupportLanguageAddresses = object.value(QLatin1String("chatSupport")).toObject();
	for (auto itr = chatSupportLanguageAddresses.constBegin(); itr != chatSupportLanguageAddresses.constEnd(); ++itr) {
		item.d->chatSupport.values.insert_or_assign(
			itr.key().toUpper(),
			transform(itr.value().toArray(), [](auto item) { return item.toString(); })
		);
	}

	const auto groupChatSupportLanguageAddresses = object.value(QLatin1String("groupChatSupport")).toObject();
	for (auto itr = groupChatSupportLanguageAddresses.constBegin(); itr != groupChatSupportLanguageAddresses.constEnd(); ++itr) {
		item.d->groupChatSupport.values.insert_or_assign(
			itr.key().toUpper(),
			transform(itr.value().toArray(), [](auto item) { return item.toString(); })
		);
	}

	return item;
}

ProviderListItem::ProviderListItem(bool isCustomProvider)
	: d(new ProviderListItemPrivate)
{
	d->isCustomProvider = isCustomProvider;
}

ProviderListItem::ProviderListItem(const ProviderListItem& other) = default;

ProviderListItem::~ProviderListItem() = default;

ProviderListItem & ProviderListItem::operator=(const ProviderListItem& other) = default;

bool ProviderListItem::isCustomProvider() const
{
	return d->isCustomProvider;
}

void ProviderListItem::setIsCustomProvider(bool isCustomProvider)
{
	d->isCustomProvider = isCustomProvider;
}

QString ProviderListItem::jid() const
{
	return d->jid;
}

void ProviderListItem::setJid(const QString &jid)
{
	d->jid = jid;
}

bool ProviderListItem::supportsInBandRegistration() const
{
	return d->supportsInBandRegistration;
}

void ProviderListItem::setSupportsInBandRegistration(bool supportsInBandRegistration)
{
	d->supportsInBandRegistration = supportsInBandRegistration;
}

QUrl ProviderListItem::registrationWebPage() const
{
	return d->registrationWebPage;
}

void ProviderListItem::setRegistrationWebPage(const QUrl &registrationWebPage)
{
	d->registrationWebPage = registrationWebPage;
}

QVector<QString> ProviderListItem::languages() const
{
	return { d->websites.keyBegin(), d->websites.keyEnd() };
}

QVector<QString> ProviderListItem::countries() const
{
	return d->countries;
}

void ProviderListItem::setCountries(const QVector<QString> &countries)
{
	d->countries = countries;
}

QVector<QString> ProviderListItem::flags() const
{
	// If this object is the custom provider, no flag should be shown.
	if (d->isCustomProvider)
		return {};

	// If no country is specified, return a flag for an unknown country.
	if (d->countries.isEmpty()) {
		return { QStringLiteral("🏳️‍🌈") };
	}

	QVector<QString> flags;
	for (const auto &country : std::as_const(d->countries)) {
		QString flag;

		// Iterate over the characters of the country string.
		// Example: For the country string "DE", the loop iterates over the characters "D" and "E".
		// An emoji flag sequence (i.e. the flag of the corresponding country / region) is represented by two regional indicator symbols.
		// Example: 🇩 (U+1F1E9 = 0x1F1E9 = 127465) and 🇪 (U+1F1EA = 127466) concatenated result in 🇩🇪.
		// Each regional indicator symbol is created by a string which has the following Unicode code point:
		// REGIONAL_INDICATOR_SYMBOL_BASE + unicode code point of the character of the country string.
		// Example: 127397 (REGIONAL_INDICATOR_SYMBOL_BASE) + 68 (unicode code point of "D") = 127465 for 🇩
		//
		// QString does not provide creating a string by its corresponding Unicode code point.
		// Therefore, QChar must be used to create a character by its Unicode code point.
		// Unfortunately, that cannot be done in one step because QChar does not support creating Unicode characters greater than 16 bits.
		// For this reason, each character of the country string is split into two parts.
		// Each part consists of 16 bits of the original character.
		// The first and the second part are then merged into one string.
		//
		// Finally, the string consisting of the first regional indicator symbol and the string consisting of the second one are concatenated.
		// The resulting string represents the emoji flag sequence.
		for (const auto &character : country) {
			auto regionalIncidatorSymbolCodePoint = REGIONAL_INDICATOR_SYMBOL_BASE + character.unicode();
			QChar regionalIncidatorSymbolParts[2];
			regionalIncidatorSymbolParts[0] = QChar::highSurrogate(regionalIncidatorSymbolCodePoint);
			regionalIncidatorSymbolParts[1] = QChar::lowSurrogate(regionalIncidatorSymbolCodePoint);

			auto regionalIncidatorSymbol = QString(regionalIncidatorSymbolParts, 2);
			flag.append(regionalIncidatorSymbol);
		}

		flags.append(flag);
	}

	return flags;
}

QMap<QString, QUrl> ProviderListItem::websites() const
{
	return d->websites;
}

void ProviderListItem::setWebsites(const QMap<QString, QUrl> &websites)
{
	d->websites = websites;
}

int ProviderListItem::onlineSince() const
{
	return d->onlineSince;
}

void ProviderListItem::setOnlineSince(int onlineSince)
{
	d->onlineSince = onlineSince;
}

int ProviderListItem::httpUploadSize() const
{
	return d->httpUploadSize;
}

void ProviderListItem::setHttpUploadSize(int httpUploadSize)
{
	d->httpUploadSize = httpUploadSize;
}

int ProviderListItem::messageStorageDuration() const
{
	return d->messageStorageDuration;
}

void ProviderListItem::setMessageStorageDuration(int messageStorageDuration)
{
	d->messageStorageDuration = messageStorageDuration;
}

QVector<QString> ProviderListItem::chatSupport() const
{
	return d->chatSupport.pickBySystemLocale();
}

void ProviderListItem::setChatSupport(std::unordered_map<QString, QVector<QString>> &&chatSupport)
{
	d->chatSupport.values = std::move(chatSupport);
}

QVector<QString> ProviderListItem::groupChatSupport() const
{
	return d->groupChatSupport.pickBySystemLocale();
}

void ProviderListItem::setGroupChatSupport(std::unordered_map<QString, QVector<QString>> &&groupChatSupport)
{
	d->groupChatSupport.values = std::move(groupChatSupport);
}

bool ProviderListItem::operator<(const ProviderListItem& other) const
{
	return d->jid < other.jid();
}

bool ProviderListItem::operator>(const ProviderListItem& other) const
{
	return d->jid > other.jid();
}

bool ProviderListItem::operator<=(const ProviderListItem& other) const
{
	return d->jid <= other.jid();
}

bool ProviderListItem::operator>=(const ProviderListItem& other) const
{
	return d->jid >= other.jid();
}

bool ProviderListItem::operator==(const ProviderListItem& other) const
{
	return d == other.d;
}
