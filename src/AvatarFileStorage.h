// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QMap>

class AvatarFileStorage : public QObject
{
	Q_OBJECT

public:
	AvatarFileStorage(QObject *parent = nullptr);

	struct AddAvatarResult {
		/* SHA1 HEX Hash */
		QString hash;
		/* If the hash for the JID has changed */
		bool hasChanged = false;
		/* If the avater was new and had to be written onto the disk */
		bool newWritten = false;
	};

	/**
	 * Add a new avatar in binary form that will be saved in a cache location
	 *
	 * @param jid The JID the avatar belongs to
	 * @param avatar The binary avatar (not in base64)
	 */
	AddAvatarResult addAvatar(const QString &jid, const QByteArray &avatar);

	/**
	 * Clears the user's avatar
	 */
	void clearAvatar(const QString &jid);

	/**
	 * Deletes the avatar with this hash, if it isn't used anymore
	 */
	void cleanUp(QString &oldHash);

	/**
	 * Returns the path to the avatar of the JID
	 */
	QString getAvatarPathOfJid(const QString &jid) const;

	/**
	 * Returns true if there an avatar saved for the given hash
	 *
	 * @param hash The SHA1 hash of the binary avatar
	 */
	bool hasAvatarHash(const QString &hash) const;

	/**
	 * Returns the path to a given hash
	 */
	QString getAvatarPath(const QString &hash) const;

	/**
	 * Returns the hash of the avatar of the JID
	 */
	Q_INVOKABLE QString getHashOfJid(const QString &jid) const;

	/**
	 * Returns a file URL to the avatar image of a given JID
	 */
	Q_INVOKABLE QString getAvatarUrl(const QString &jid) const;

signals:
	void avatarIdsChanged();

private:
	void saveAvatarsFile();

	QMap<QString, QString> m_jidAvatarMap;
};
