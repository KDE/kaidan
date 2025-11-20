// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Provider.h"

// Qt
#include <QJsonArray>
#include <QJsonObject>
// Kaidan
#include "Algorithms.h"

#define REGIONAL_INDICATOR_SYMBOL_BASE 0x1F1A5

Provider::Provider(const QJsonObject &object)
    : m_isCustomProvider(false)
    , m_jid(object.value(QLatin1String("jid")).toString())
    , m_supportsInBandRegistration(object.value(QLatin1String("inBandRegistration")).toBool())
    , m_registrationWebPages(parseStringLanguageVariants<QUrl>(object.value(QLatin1String("registrationWebPage")).toObject()))
    , m_countries([object] {
        const auto serverLocations = object.value(QLatin1String("serverLocations")).toArray();
        QList<QString> countries;

        for (const auto &country : serverLocations) {
            countries.append(country.toString().toUpper());
        }

        return countries;
    }())
    , m_websites(parseStringLanguageVariants<QUrl>(object.value(QLatin1String("website")).toObject()))
    , m_busFactor(object.value(QLatin1String("busFactor")).toInt())
    , m_organization(object.value(QLatin1String("organization")).toString())
    , m_supportsPasswordReset(!object.value(QLatin1String("passwordReset")).toObject().isEmpty())
    , m_hostedGreen(!object.value(QLatin1String("ratingGreenWebCheck")).toBool())
    , m_since(object.value(QLatin1String("since")).toVariant().toDate())
    , m_httpUploadFileSize(object.value(QLatin1String("maximumHttpFileUploadFileSize")).toInt())
    , m_httpUploadTotalSize(object.value(QLatin1String("maximumHttpFileUploadTotalSize")).toInt())
    , m_httpUploadStorageDuration(object.value(QLatin1String("maximumHttpFileUploadStorageTime")).toInt())
    , m_messageStorageDuration(object.value(QLatin1String("maximumMessageArchiveManagementStorageTime")).toInt())
    , m_freeOfCharge(object.value(QLatin1String("freeOfCharge")).toBool())
    , m_chatSupport(parseStringListLanguageVariants<QList<QString>>(object.value(QLatin1String("chatSupport")).toObject()))
    , m_groupChatSupport(parseStringListLanguageVariants<QList<QString>>(object.value(QLatin1String("groupChatSupport")).toObject()))
{
}

Provider::Provider()
    : m_isCustomProvider(true)
{
}

bool Provider::isCustomProvider() const
{
    return m_isCustomProvider;
}

QString Provider::jid() const
{
    return m_jid;
}

bool Provider::supportsInBandRegistration() const
{
    return m_supportsInBandRegistration;
}

Provider::LanguageVariants<QUrl> Provider::registrationWebPages() const
{
    return m_registrationWebPages;
}

QList<QString> Provider::languages() const
{
    return {m_websites.keyBegin(), m_websites.keyEnd()};
}

QList<QString> Provider::countries() const
{
    return m_countries;
}

QList<QString> Provider::flags() const
{
    // If this object is the custom provider, no flag should be shown.
    if (m_isCustomProvider)
        return {};

    // If no country is specified, return a flag for an unknown country.
    if (m_countries.isEmpty()) {
        return {QStringLiteral("🏳️‍🌈")};
    }

    QList<QString> flags;
    for (const auto &country : std::as_const(m_countries)) {
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

Provider::LanguageVariants<QUrl> Provider::websites() const
{
    return m_websites;
}

QUrl Provider::chosenWebsite() const
{
    return websites().pickBySystemLocale();
}

int Provider::busFactor() const
{
    return std::max(m_busFactor, 1);
}

QString Provider::organization() const
{
    return m_organization;
}

bool Provider::supportsPasswordReset() const
{
    return m_supportsPasswordReset;
}

bool Provider::hostedGreen() const
{
    return m_hostedGreen;
}

QDate Provider::since() const
{
    return m_since;
}

int Provider::httpUploadFileSize() const
{
    return m_httpUploadFileSize;
}

int Provider::httpUploadTotalSize() const
{
    return m_httpUploadTotalSize;
}

int Provider::httpUploadStorageDuration() const
{
    return m_httpUploadStorageDuration;
}

int Provider::messageStorageDuration() const
{
    return m_messageStorageDuration;
}

bool Provider::freeOfCharge() const
{
    return m_freeOfCharge;
}

Provider::LanguageVariants<QList<QString>> Provider::chatSupport() const
{
    return m_chatSupport;
}

QList<QString> Provider::chosenChatSupport() const
{
    return chatSupport().pickBySystemLocale();
}

Provider::LanguageVariants<QList<QString>> Provider::groupChatSupport() const
{
    return m_groupChatSupport;
}

QList<QString> Provider::chosenGroupChatSupport() const
{
    return groupChatSupport().pickBySystemLocale();
}

bool Provider::operator<(const Provider &other) const
{
    return m_jid < other.jid();
}

bool Provider::operator>(const Provider &other) const
{
    return m_jid > other.jid();
}

bool Provider::operator<=(const Provider &other) const
{
    return m_jid <= other.jid();
}

bool Provider::operator>=(const Provider &other) const
{
    return m_jid >= other.jid();
}

bool Provider::operator==(const Provider &other) const
{
    return m_jid == other.jid();
}

template<typename T>
Provider::LanguageVariants<T> Provider::parseStringLanguageVariants(const QJsonObject &stringLanguageVariants)
{
    return parseLanguageVariants<T>(stringLanguageVariants, [](const QJsonValue &value) {
        return T{value.toString()};
    });
}

template<typename T>
Provider::LanguageVariants<T> Provider::parseStringListLanguageVariants(const QJsonObject &stringListLanguageVariants)
{
    return parseLanguageVariants<T>(stringListLanguageVariants, [](const QJsonValue &value) {
        return T{transform(value.toArray(), [](const QJsonValue &item) {
            return item.toString();
        })};
    });
}

template<typename T>
Provider::LanguageVariants<T> Provider::parseLanguageVariants(const QJsonObject &languageVariants,
                                                              const std::function<T(const QJsonValue &)> &convertToTargetType)
{
    Provider::LanguageVariants<T> parsedLanguageVariants;

    for (auto itr = languageVariants.constBegin(); itr != languageVariants.constEnd(); ++itr) {
        const auto language = itr.key().toUpper();
        const T languageVariant = convertToTargetType(itr.value());
        parsedLanguageVariants.insert(language, languageVariant);
    }

    return parsedLanguageVariants;
}

#include "moc_Provider.cpp"
