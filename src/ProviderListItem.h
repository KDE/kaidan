// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QLocale>
#include <QSharedDataPointer>
#include <QVariantList>
// Kaidan
#include "Globals.h"
#include "SystemUtils.h"

class QUrl;
class QJsonObject;

class ProviderListItemPrivate;

class ProviderListItem
{
    Q_GADGET

    Q_PROPERTY(QUrl chosenWebsite READ chosenWebsite CONSTANT)
    Q_PROPERTY(QList<QString> chosenChatSupport READ chosenChatSupport CONSTANT)
    Q_PROPERTY(QList<QString> chosenGroupChatSupport READ chosenGroupChatSupport CONSTANT)

public:
    template<typename T>
    struct LanguageVariants : QMap<QString, T> {
        T pickBySystemLocale() const
        {
            const auto languageCode = SystemUtils::systemLocaleCodes().languageCode;

            // Use the system-wide language variant if available.
            const auto systemLanguageItr = this->find(languageCode);
            if (systemLanguageItr != this->cend() && !systemLanguageItr.value().isEmpty()) {
                return systemLanguageItr.value();
            }

            // Use the English variant if no system-wide language variant is available but English
            // is.
            const auto defaultLanguageItr = this->find(DEFAULT_LANGUAGE_CODE.toString());
            if (defaultLanguageItr != this->cend() && !defaultLanguageItr.value().isEmpty()) {
                return defaultLanguageItr.value();
            }

            // Use the first language variant if also no English variant is available but another
            // one is.
            if (!this->empty()) {
                return this->cbegin().value();
            }

            return {};
        }
    };

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

    LanguageVariants<QUrl> registrationWebPages() const;
    void setRegistrationWebPages(const LanguageVariants<QUrl> &registrationWebPages);

    QList<QString> languages() const;

    QList<QString> countries() const;
    void setCountries(const QList<QString> &country);

    QList<QString> flags() const;

    LanguageVariants<QUrl> websites() const;
    void setWebsites(const LanguageVariants<QUrl> &websites);
    QUrl chosenWebsite() const;

    QDate since() const;
    void setSince(const QDate &since);

    int httpUploadSize() const;
    void setHttpUploadSize(int httpUploadSize);

    int messageStorageDuration() const;
    void setMessageStorageDuration(int messageStorageDuration);

    LanguageVariants<QList<QString>> chatSupport() const;
    void setChatSupport(const LanguageVariants<QList<QString>> &chatSupport);
    QList<QString> chosenChatSupport() const;

    LanguageVariants<QList<QString>> groupChatSupport() const;
    void setGroupChatSupport(const LanguageVariants<QList<QString>> &groupChatSupport);
    QList<QString> chosenGroupChatSupport() const;

    bool operator<(const ProviderListItem &other) const;
    bool operator>(const ProviderListItem &other) const;
    bool operator<=(const ProviderListItem &other) const;
    bool operator>=(const ProviderListItem &other) const;

    bool operator==(const ProviderListItem &other) const;

private:
    template<typename T>
    static LanguageVariants<T> parseStringLanguageVariants(const QJsonObject &stringLanguageVariants);

    template<typename T>
    static LanguageVariants<T> parseStringListLanguageVariants(const QJsonObject &stringListLanguageVariants);

    template<typename T>
    static LanguageVariants<T> parseLanguageVariants(const QJsonObject &LanguageVariants, const std::function<T(const QJsonValue &)> &convertToTargetType);

    QSharedDataPointer<ProviderListItemPrivate> d;
};
