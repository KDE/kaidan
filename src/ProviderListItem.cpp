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

class ProviderListItemPrivate : public QSharedData
{
public:
	ProviderListItemPrivate();

	bool isCustomProvider;
	QString jid;
	bool supportsInBandRegistration;
	ProviderListItem::LanguageVariants<QUrl> registrationWebPages;
	QVector<QString> languages;
	QVector<QString> countries;
	ProviderListItem::LanguageVariants<QUrl> websites;
	int onlineSince;
	int httpUploadSize;
	int messageStorageDuration;
	ProviderListItem::LanguageVariants<QVector<QString>> chatSupport;
	ProviderListItem::LanguageVariants<QVector<QString>> groupChatSupport;
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
	item.setRegistrationWebPages(parseStringLanguageVariants<QUrl>(object.value(QLatin1String("registrationWebPage")).toObject()));

	const auto serverLocations = object.value(QLatin1String("serverLocations")).toArray();
	QVector<QString> countries;
	for (const auto &country : serverLocations) {
		countries.append(country.toString().toUpper());
	}
	item.setCountries(countries);

	item.setWebsites(parseStringLanguageVariants<QUrl>(object.value(QLatin1String("website")).toObject()));
	item.setOnlineSince(object.value(QLatin1String("since")).toInt(-1));
	item.setHttpUploadSize(object.value(QLatin1String("maximumHttpFileUploadFileSize")).toInt(-1));
	item.setMessageStorageDuration(object.value(QLatin1String("maximumMessageArchiveManagementStorageTime")).toInt(-1));
	item.setChatSupport(parseStringListLanguageVariants<QVector<QString>>(object.value(QLatin1String("chatSupport")).toObject()));
	item.setGroupChatSupport(parseStringListLanguageVariants<QVector<QString>>(object.value(QLatin1String("groupChatSupport")).toObject()));

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

ProviderListItem::LanguageVariants<QUrl> ProviderListItem::registrationWebPages() const
{
	return d->registrationWebPages;
}

void ProviderListItem::setRegistrationWebPages(const LanguageVariants<QUrl> &registrationWebPages)
{
	d->registrationWebPages = registrationWebPages;
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

ProviderListItem::LanguageVariants<QUrl> ProviderListItem::websites() const
{
	return d->websites;
}

void ProviderListItem::setWebsites(const LanguageVariants<QUrl> &websites)
{
	d->websites = websites;
}

QUrl ProviderListItem::chosenWebsite() const
{
	return websites().pickBySystemLocale();
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

ProviderListItem::LanguageVariants<QVector<QString>> ProviderListItem::chatSupport() const
{
	return d->chatSupport;
}

void ProviderListItem::setChatSupport(const LanguageVariants<QVector<QString>> &chatSupport)
{
	d->chatSupport = chatSupport;
}

QVector<QString> ProviderListItem::chosenChatSupport() const
{
	return chatSupport().pickBySystemLocale();
}

ProviderListItem::LanguageVariants<QVector<QString>> ProviderListItem::groupChatSupport() const
{
	return d->groupChatSupport;
}

void ProviderListItem::setGroupChatSupport(const LanguageVariants<QVector<QString>> &groupChatSupport)
{
	d->groupChatSupport = groupChatSupport;
}

QVector<QString> ProviderListItem::chosenGroupChatSupport() const
{
	return groupChatSupport().pickBySystemLocale();
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

template<typename T>
ProviderListItem::LanguageVariants<T> ProviderListItem::parseStringLanguageVariants(const QJsonObject &stringLanguageVariants)
{
	return parseLanguageVariants<T>(stringLanguageVariants, [](const QJsonValue &value) {
		return T { value.toString() };
	});
}

template<typename T>
ProviderListItem::LanguageVariants<T> ProviderListItem::parseStringListLanguageVariants(const QJsonObject &stringListLanguageVariants)
{
	return parseLanguageVariants<T>(stringListLanguageVariants, [](const QJsonValue &value) {
		return T { transform(value.toArray(), [](const QJsonValue &item) { return item.toString(); }) };
	});
}

template<typename T>
ProviderListItem::LanguageVariants<T> ProviderListItem::parseLanguageVariants(const QJsonObject &languageVariants, const std::function<T (const QJsonValue &)> &convertToTargetType)
{
	ProviderListItem::LanguageVariants<T> parsedLanguageVariants;

	for (auto itr = languageVariants.constBegin(); itr != languageVariants.constEnd(); ++itr) {
		const auto language = itr.key().toUpper();
		const T languageVariant = convertToTargetType(itr.value());
		parsedLanguageVariants.insert(language, languageVariant);
	}

	return parsedLanguageVariants;
}
