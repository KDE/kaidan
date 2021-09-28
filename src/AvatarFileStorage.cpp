/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2021 Kaidan developers and contributors
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

#include "AvatarFileStorage.h"
// Qt
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

AvatarFileStorage::AvatarFileStorage(QObject *parent) : QObject(parent)
{
	// create avatar directory, if it doesn't exists
	QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
	if (!cacheDir.exists(QStringLiteral("avatars")))
		cacheDir.mkpath(QStringLiteral("avatars"));

	// search for the avatar list file (hash <-> jid)
	QString avatarFilePath = getAvatarPath(QStringLiteral("avatar_list.sha1"));

	try {
		// check if file was found
		if (avatarFilePath != QLatin1String("")) {
			//
			// restore saved avatars
			//

			// open the file; if fails return
			QFile avatarFile(avatarFilePath);
			if (!avatarFile.open(QIODevice::ReadOnly))
				return;

			QTextStream stream(&avatarFile);
			// get the first line
			QString line = stream.readLine();
			while (!line.isNull()) {
				// get hash and jid from line (seperated by a blank)
				QStringList list = line.split(' ', Qt::SkipEmptyParts);

				// set the hash for the jid
				m_jidAvatarMap[list.at(1)] = list.at(0);

				// read the next line
				line = stream.readLine();
			};
		}
	} catch (...) {
		qDebug() << "[AvatarFileStorage] Error in" << avatarFilePath << "(avatar list file)";
	}
}

AvatarFileStorage::AddAvatarResult AvatarFileStorage::addAvatar(const QString &jid,
	const QByteArray &avatar)
{
	AddAvatarResult result;

	// generate a hexadecimal hash of the raw avatar
	result.hash = QString(QCryptographicHash::hash(avatar, QCryptographicHash::Sha1).toHex());
	QString oldHash = m_jidAvatarMap[jid];

	// set the new hash and the `hasChanged` tag
	if (oldHash != result.hash) {
		m_jidAvatarMap[jid] = result.hash;
		result.hasChanged = true;

		// delete the avatar if it isn't used anymore
		cleanUp(oldHash);
	}

	// abort if the avatar with this hash is already saved
	// only update GUI, if avatar really has changed
	if (hasAvatarHash(result.hash)) {
		if (result.hasChanged)
			emit avatarIdsChanged();
		return result;
	}

	// write the avatar to disk:
	// get the writable cache location for writing
	QFile file(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
		   QDir::separator() + QStringLiteral("avatars") + QDir::separator() + result.hash);
	if (!file.open(QIODevice::WriteOnly))
		return result;

	// write the binary avatar
	file.write(avatar);

	saveAvatarsFile();

	// mark that the avatar is new
	result.newWritten = true;

	emit avatarIdsChanged();
	return result;
}

void AvatarFileStorage::clearAvatar(const QString &jid)
{
	QString oldHash;
	if (m_jidAvatarMap.contains(jid))
		oldHash = m_jidAvatarMap[jid];

	// if user had no avatar before, just return
	if (oldHash.isEmpty())
		return;

	m_jidAvatarMap.remove(jid);
	saveAvatarsFile();
	cleanUp(oldHash);
	emit avatarIdsChanged();
}

void AvatarFileStorage::cleanUp(QString &oldHash)
{
	if (oldHash.isEmpty())
		return;

	// check if the same avatar is still used by another account
	if (std::find(m_jidAvatarMap.cbegin(), m_jidAvatarMap.cend(), oldHash) != m_jidAvatarMap.cend())
		return;

	// delete the old avatar locally
	QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
	               QDir::separator() + QStringLiteral("avatars");
	QDir dir(path);
	if (dir.exists(oldHash))
		dir.remove(oldHash);
}

QString AvatarFileStorage::getAvatarPath(const QString &hash) const
{
	return QStandardPaths::locate(QStandardPaths::CacheLocation, QStringLiteral("avatars") +
	       QDir::separator() + hash, QStandardPaths::LocateFile);
}

QString AvatarFileStorage::getHashOfJid(const QString& jid) const
{
	return m_jidAvatarMap[jid];
}

QString AvatarFileStorage::getAvatarPathOfJid(const QString& jid) const
{
	return getAvatarPath(getHashOfJid(jid));
}

QString AvatarFileStorage::getAvatarUrl(const QString &jid) const
{
	return QUrl::fromLocalFile(getAvatarPathOfJid(jid)).toString();
}

bool AvatarFileStorage::hasAvatarHash(const QString& hash) const
{
	return !getAvatarPath(hash).isNull();
}

void AvatarFileStorage::saveAvatarsFile()
{
	QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
	               QDir::separator() + QStringLiteral("avatars") + QDir::separator() +
	               QStringLiteral("avatar_list.sha1");
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&file);
	const QStringList jidAvatarKeys = m_jidAvatarMap.keys();
	for (const auto &jid : jidAvatarKeys)
		/*     < HASH >            < JID >  */
		out << m_jidAvatarMap[jid] << " " << jid << "\n";
}
