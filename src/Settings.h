// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QMutex>
#include <QObject>
#include <QPoint>
#include <QSettings>
#include <QSize>

#include "AccountManager.h"
#include "Encryption.h"
#include "Kaidan.h"

#include <optional>

constexpr quint16 PORT_AUTODETECT = 0;

/**
 * Manages settings stored in the settings file.
 *
 * All methods are thread-safe.
 */
class Settings : public QObject
{
	Q_OBJECT

	Q_PROPERTY(Kaidan::PasswordVisibility passwordVisibility READ authPasswordVisibility WRITE setAuthPasswordVisibility NOTIFY authPasswordVisibilityChanged)
	Q_PROPERTY(Encryption::Enum encryption READ encryption WRITE setEncryption NOTIFY encryptionChanged)
	Q_PROPERTY(bool contactAdditionQrCodePageExplanationVisible READ contactAdditionQrCodePageExplanationVisible WRITE setContactAdditionQrCodePageExplanationVisible NOTIFY contactAdditionQrCodePageExplanationVisibleChanged)
	Q_PROPERTY(bool keyAuthenticationPageExplanationVisible READ keyAuthenticationPageExplanationVisible WRITE setKeyAuthenticationPageExplanationVisible NOTIFY keyAuthenticationPageExplanationVisibleChanged)
	Q_PROPERTY(QPoint windowPosition READ windowPosition WRITE setWindowPosition NOTIFY windowPositionChanged)
	Q_PROPERTY(QSize windowSize READ windowSize WRITE setWindowSize NOTIFY windowSizeChanged)
	Q_PROPERTY(AccountManager::AutomaticMediaDownloadsRule automaticMediaDownloadsRule READ automaticMediaDownloadsRule WRITE setAutomaticMediaDownloadsRule NOTIFY automaticMediaDownloadsRuleChanged)

public:
	explicit Settings(QObject *parent = nullptr);

	///
	/// Avoid using this in favour of adding methods here,
	/// but it is useful if you need to manually manage config groups
	///
	QSettings &raw();

	bool authOnline() const;
	void setAuthOnline(bool online);

	QString authJid() const;
	void setAuthJid(const QString &jid);

	QString authJidResourcePrefix() const;
	void setAuthJidResourcePrefix(const QString &prefix);

	QString authPassword() const;
	void setAuthPassword(const QString &password);

	QString authHost() const;
	void setAuthHost(const QString &host);
	void resetAuthHost();

	quint16 authPort() const;
	void setAuthPort(quint16 port);
	void resetAuthPort();
	bool isDefaultAuthPort() const;

	bool authTlsErrorsIgnored() const;
	void setAuthTlsErrorsIgnored(bool enabled);

	QXmppConfiguration::StreamSecurityMode authTlsRequirement() const;
	void setAuthTlsRequirement(QXmppConfiguration::StreamSecurityMode mode);

	Kaidan::PasswordVisibility authPasswordVisibility() const;
	void setAuthPasswordVisibility(Kaidan::PasswordVisibility visibility);

	QUuid userAgentDeviceId() const;
	void setUserAgentDeviceId(QUuid deviceId);

	Encryption::Enum encryption() const;
	void setEncryption(Encryption::Enum encryption);

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

	AccountManager::AutomaticMediaDownloadsRule automaticMediaDownloadsRule() const;
	void setAutomaticMediaDownloadsRule(AccountManager::AutomaticMediaDownloadsRule rule);

	void remove(const QStringList &keys);

Q_SIGNALS:
	void authOnlineChanged();
	void authJidChanged();
	void authJidResourcePrefixChanged();
	void authPasswordChanged();
	void authHostChanged();
	void authPortChanged();
	void authIgnoreTlsErrosChanged();
	void authTlsRequirementChanged();
	void authPasswordVisibilityChanged();
	void encryptionChanged();
	void contactAdditionQrCodePageExplanationVisibleChanged();
	void keyAuthenticationPageExplanationVisibleChanged();
	void favoriteEmojisChanged();
	void windowPositionChanged();
	void windowSizeChanged();
	void automaticMediaDownloadsRuleChanged();

private:
	template<typename T>
	T value(const QString &key, const std::optional<T> &defaultValue = {}) const {
		QMutexLocker locker(&m_mutex);

		if (defaultValue) {
			return m_settings.value(key, QVariant::fromValue(*defaultValue)).template value<T>();
		}

		return m_settings.value(key).template value<T>();
	}


	template<typename T>
	void setValue(const QString &key, const T &value) {
		QMutexLocker locker(&m_mutex);
		if constexpr (!has_enum_type<T>::value && std::is_enum<T>::value) {
			m_settings.setValue(key, static_cast<std::underlying_type_t<T>>(value));
		} else if constexpr (has_enum_type<T>::value) {
			m_settings.setValue(key, static_cast<typename T::Int>(value));
		} else {
			m_settings.setValue(key, QVariant::fromValue(value));
		}
	}

	template<typename T, typename S, typename std::enable_if<int(QtPrivate::FunctionPointer<S>::ArgumentCount) <= 1, T> * = nullptr>
	void setValue(const QString &key, const T &value, S s) {
		setValue(key, value);

		if constexpr (int(QtPrivate::FunctionPointer<S>::ArgumentCount) == 0) {
			Q_EMIT(this->*s)();
		} else {
			Q_EMIT(this->*s)(value);
		}
	}

	template<typename S, typename std::enable_if<int(QtPrivate::FunctionPointer<S>::ArgumentCount) == 0, S> * = nullptr>
	void remove(const QString &key, S s) {
		QMutexLocker locker(&m_mutex);
		m_settings.remove(key);
		locker.unlock();

		Q_EMIT(this->*s)();
	}

	QSettings m_settings;
	mutable QMutex m_mutex;
};

#endif // SETTINGS_H
