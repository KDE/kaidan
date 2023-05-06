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

#include "Encryption.h"
#include "Kaidan.h"

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
	Q_PROPERTY(bool qrCodePageExplanationVisible READ qrCodePageExplanationVisible WRITE setQrCodePageExplanationVisible NOTIFY qrCodePageExplanationVisibleChanged)
	Q_PROPERTY(QPoint windowPosition READ windowPosition WRITE setWindowPosition NOTIFY windowPositionChanged)
	Q_PROPERTY(QSize windowSize READ windowSize WRITE setWindowSize NOTIFY windowSizeChanged)

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

	Kaidan::PasswordVisibility authPasswordVisibility() const;
	void setAuthPasswordVisibility(Kaidan::PasswordVisibility visibility);

	Encryption::Enum encryption() const;
	void setEncryption(Encryption::Enum encryption);

	/**
	 * Retrieves the visibility of the QrCodePage's explanation from the settings file.
	 *
	 * @return true if the explanation is set to be visible, otherwise false
	 */
	bool qrCodePageExplanationVisible() const;

	/**
	 * Stores the visibility of the QrCodePage's explanation in the settings file.
	 *
	 * @param isVisible true if the explanation should be visible in the future, otherwise false
	 */
	void setQrCodePageExplanationVisible(bool isVisible);

	QStringList favoriteEmojis() const;
	void setFavoriteEmojis(const QStringList &emoji);

	QPoint windowPosition() const;
	void setWindowPosition(const QPoint &windowPosition);

	QSize windowSize() const;
	void setWindowSize(const QSize &windowSize);

	void remove(const QStringList &keys);

signals:
	void authOnlineChanged();
	void authJidChanged();
	void authJidResourcePrefixChanged();
	void authPasswordChanged();
	void authHostChanged();
	void authPortChanged();
	void authPasswordVisibilityChanged();
	void encryptionChanged();
	void qrCodePageExplanationVisibleChanged();
	void favoriteEmojisChanged();
	void windowPositionChanged();
	void windowSizeChanged();

private:
	QSettings m_settings;
	mutable QMutex m_mutex;
};

#endif // SETTINGS_H
