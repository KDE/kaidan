// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <optional>
// Qt
#include <QObject>
#include <QPoint>
#include <QSettings>
#include <QSize>
// Kaidan
#include "Enums.h"

/**
 * Manages settings stored in the settings file.
 *
 * All methods are thread-safe.
 */
class Settings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool contactAdditionQrCodePageExplanationVisible READ contactAdditionQrCodePageExplanationVisible WRITE
                   setContactAdditionQrCodePageExplanationVisible NOTIFY contactAdditionQrCodePageExplanationVisibleChanged)
    Q_PROPERTY(bool keyAuthenticationPageExplanationVisible READ keyAuthenticationPageExplanationVisible WRITE setKeyAuthenticationPageExplanationVisible NOTIFY
                   keyAuthenticationPageExplanationVisibleChanged)
    Q_PROPERTY(QPoint windowPosition READ windowPosition WRITE setWindowPosition NOTIFY windowPositionChanged)
    Q_PROPERTY(QSize windowSize READ windowSize WRITE setWindowSize NOTIFY windowSizeChanged)

    friend class Database;

public:
    static Settings *instance();

    explicit Settings(QObject *parent = nullptr);
    ~Settings();

    ///
    /// Avoid using this in favour of adding methods here,
    /// but it is useful if you need to manually manage config groups
    ///
    QSettings &raw();

    bool contactAdditionQrCodePageExplanationVisible() const;
    void setContactAdditionQrCodePageExplanationVisible(bool visible);

    bool keyAuthenticationPageExplanationVisible() const;
    void setKeyAuthenticationPageExplanationVisible(bool visible);

    QStringList favoriteEmojis() const;
    void setFavoriteEmojis(const QStringList &emoji);

    QPoint windowPosition() const;
    void setWindowPosition(const QPoint &windowPosition);

    QSize windowSize() const;
    void setWindowSize(const QSize &windowSize);

    void remove(const QStringList &keys);

Q_SIGNALS:
    void contactAdditionQrCodePageExplanationVisibleChanged();
    void keyAuthenticationPageExplanationVisibleChanged();
    void favoriteEmojisChanged();
    void windowPositionChanged();
    void windowSizeChanged();

private:
    template<typename T>
    T value(const QString &key, const std::optional<T> &defaultValue = {}) const
    {
        if (defaultValue) {
            return m_settings.value(key, QVariant::fromValue(*defaultValue)).template value<T>();
        }

        return m_settings.value(key).template value<T>();
    }

    template<typename T>
    void setValue(const QString &key, const T &value)
    {
        if constexpr (!has_enum_type<T>::value && std::is_enum<T>::value) {
            m_settings.setValue(key, static_cast<std::underlying_type_t<T>>(value));
        } else if constexpr (has_enum_type<T>::value) {
            m_settings.setValue(key, static_cast<typename T::Int>(value));
        } else {
            m_settings.setValue(key, QVariant::fromValue(value));
        }
    }

    template<typename T, typename S, typename std::enable_if<int(QtPrivate::FunctionPointer<S>::ArgumentCount) <= 1, T> * = nullptr>
    void setValue(const QString &key, const T &value, S s)
    {
        setValue(key, value);

        if constexpr (int(QtPrivate::FunctionPointer<S>::ArgumentCount) == 0) {
            Q_EMIT(this->*s)();
        } else {
            Q_EMIT(this->*s)(value);
        }
    }

    template<typename S, typename std::enable_if<int(QtPrivate::FunctionPointer<S>::ArgumentCount) == 0, S> * = nullptr>
    void remove(const QString &key, S s)
    {
        m_settings.remove(key);
        Q_EMIT(this->*s)();
    }

    QSettings m_settings;

    static Settings *s_instance;
};
