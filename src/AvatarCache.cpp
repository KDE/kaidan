// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2018 Ilya Bizyaev <bizyaev@zoho.com>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AvatarCache.h"

// Qt
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>
// Kaidan
#include "KaidanCoreLog.h"

AvatarCache *AvatarCache::s_instance = nullptr;

AvatarCache *AvatarCache::instance()
{
    return s_instance;
}

AvatarCache::AvatarCache(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

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
                // get hash and jid from line (separated by a blank)
                QStringList list = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);

                // set the hash for the jid
                m_jidAvatarMap[list.at(1)] = list.at(0);

                // read the next line
                line = stream.readLine();
            };
        }
    } catch (...) {
        qCDebug(KAIDAN_CORE_LOG) << "Error in" << avatarFilePath << "(avatar list file)";
    }
}

AvatarCache::~AvatarCache()
{
    s_instance = nullptr;
}

AvatarCache::AddAvatarResult AvatarCache::addAvatar(const QString &jid, const QByteArray &avatar)
{
    AddAvatarResult result;

    // generate a hexadecimal hash of the raw avatar
    result.hash = QString::fromUtf8(QCryptographicHash::hash(avatar, QCryptographicHash::Sha1).toHex());
    QString oldHash = QString::fromUtf8(m_jidAvatarMap[jid].toUtf8());

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
            Q_EMIT avatarIdsChanged();
        return result;
    }

    // write the avatar to disk:
    // get the writable cache location for writing
    QFile file(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator() + QStringLiteral("avatars") + QDir::separator()
               + result.hash);
    if (!file.open(QIODevice::WriteOnly))
        return result;

    // write the binary avatar
    file.write(avatar);

    saveAvatarsFile();

    // mark that the avatar is new
    result.newWritten = true;

    Q_EMIT avatarIdsChanged();
    return result;
}

void AvatarCache::clearAvatar(const QString &jid)
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
    Q_EMIT avatarIdsChanged();
}

void AvatarCache::cleanUp(QString &oldHash)
{
    if (oldHash.isEmpty())
        return;

    // check if the same avatar is still used by another account
    if (std::find(m_jidAvatarMap.cbegin(), m_jidAvatarMap.cend(), oldHash) != m_jidAvatarMap.cend())
        return;

    // delete the old avatar locally
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator() + QStringLiteral("avatars");
    QDir dir(path);
    if (dir.exists(oldHash))
        dir.remove(oldHash);
}

QString AvatarCache::getAvatarPath(const QString &hash) const
{
    return QStandardPaths::locate(QStandardPaths::CacheLocation, QStringLiteral("avatars") + QDir::separator() + hash, QStandardPaths::LocateFile);
}

QString AvatarCache::getHashOfJid(const QString &jid) const
{
    return m_jidAvatarMap[jid];
}

QString AvatarCache::getAvatarPathOfJid(const QString &jid) const
{
    return getAvatarPath(getHashOfJid(jid));
}

QUrl AvatarCache::getAvatarUrl(const QString &jid) const
{
    return QUrl::fromLocalFile(getAvatarPathOfJid(jid));
}

bool AvatarCache::hasAvatarHash(const QString &hash) const
{
    return !getAvatarPath(hash).isNull();
}

void AvatarCache::saveAvatarsFile()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator() + QStringLiteral("avatars") + QDir::separator()
        + QStringLiteral("avatar_list.sha1");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    const QStringList jidAvatarKeys = m_jidAvatarMap.keys();
    for (const auto &jid : jidAvatarKeys)
        /*     < HASH >            < JID >  */
        out << m_jidAvatarMap[jid] << " " << jid << "\n";
}

#include "moc_AvatarCache.cpp"

AvatarWatcher::AvatarWatcher(QObject *parent)
    : QObject(parent)
{
    connect(AvatarCache::instance(), &AvatarCache::avatarIdsChanged, this, &AvatarWatcher::urlChanged);
}

QString AvatarWatcher::jid() const
{
    return m_jid;
}

void AvatarWatcher::setJid(const QString &jid)
{
    if (m_jid != jid) {
        m_jid = jid;
        Q_EMIT jidChanged();
        Q_EMIT urlChanged();
    }
}

QUrl AvatarWatcher::url()
{
    return AvatarCache::instance()->getAvatarUrl(m_jid);
}
