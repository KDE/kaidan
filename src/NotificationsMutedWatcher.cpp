// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NotificationsMutedWatcher.h"

#include "Kaidan.h"

NotificationsMutedWatcher::NotificationsMutedWatcher(QObject *parent)
    : QObject{parent}
{
}

QString NotificationsMutedWatcher::jid() const
{
	return m_jid;
}

void NotificationsMutedWatcher::setJid(const QString &jid)
{
	m_jid = jid;
	m_muted = Kaidan::instance()->notificationsMuted(jid);

	connect(Kaidan::instance(), &Kaidan::notificationsMutedChanged, this, [this](const QString &jid) {
		if (jid == m_jid) {
			m_muted = Kaidan::instance()->notificationsMuted(jid);
			Q_EMIT mutedChanged();
		}
	});
	Q_EMIT jidChanged();
}

bool NotificationsMutedWatcher::muted() const
{
	return m_muted;
}

void NotificationsMutedWatcher::setMuted(bool muted)
{
	Kaidan::instance()->setNotificationsMuted(m_jid, muted);
}
