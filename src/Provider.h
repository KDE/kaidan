// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QDate>
#include <QLocale>
#include <QSharedDataPointer>
#include <QUrl>
// Kaidan
#include "Globals.h"
#include "SystemUtils.h"

class QJsonObject;

class Provider
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

    explicit Provider(const QJsonObject &object);
    explicit Provider();

    bool isCustomProvider() const;
    QString jid() const;
    bool supportsInBandRegistration() const;
    LanguageVariants<QUrl> registrationWebPages() const;
    QList<QString> languages() const;
    QList<QString> countries() const;
    QList<QString> flags() const;

    LanguageVariants<QUrl> websites() const;
    void setWebsites(const LanguageVariants<QUrl> &websites);
    QUrl chosenWebsite() const;

    int busFactor() const;
    QString organization() const;
    bool supportsPasswordReset() const;
    bool hostedGreen() const;
    QDate since() const;
    int httpUploadFileSize() const;
    int httpUploadTotalSize() const;
    int httpUploadStorageDuration() const;
    int messageStorageDuration() const;
    bool freeOfCharge() const;

    LanguageVariants<QList<QString>> chatSupport() const;
    QList<QString> chosenChatSupport() const;

    LanguageVariants<QList<QString>> groupChatSupport() const;
    QList<QString> chosenGroupChatSupport() const;

    bool operator<(const Provider &other) const;
    bool operator>(const Provider &other) const;
    bool operator<=(const Provider &other) const;
    bool operator>=(const Provider &other) const;

    bool operator==(const Provider &other) const;

private:
    template<typename T>
    static LanguageVariants<T> parseStringLanguageVariants(const QJsonObject &stringLanguageVariants);

    template<typename T>
    static LanguageVariants<T> parseStringListLanguageVariants(const QJsonObject &stringListLanguageVariants);

    template<typename T>
    static LanguageVariants<T> parseLanguageVariants(const QJsonObject &LanguageVariants, const std::function<T(const QJsonValue &)> &convertToTargetType);

    bool m_isCustomProvider;
    QString m_jid;
    bool m_supportsInBandRegistration = false;
    Provider::LanguageVariants<QUrl> m_registrationWebPages;
    QList<QString> m_languages;
    QList<QString> m_countries;
    Provider::LanguageVariants<QUrl> m_websites;
    int m_busFactor = -1;
    QString m_organization;
    bool m_supportsPasswordReset = false;
    bool m_hostedGreen = false;
    QDate m_since;
    int m_httpUploadFileSize = -1;
    int m_httpUploadTotalSize = -1;
    int m_httpUploadStorageDuration = -1;
    int m_messageStorageDuration = -1;
    bool m_freeOfCharge = false;
    Provider::LanguageVariants<QList<QString>> m_chatSupport;
    Provider::LanguageVariants<QList<QString>> m_groupChatSupport;
};
