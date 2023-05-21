// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class NotificationsMutedWatcher : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
	Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)

public:
	explicit NotificationsMutedWatcher(QObject *parent = nullptr);

	QString jid() const;
	void setJid(const QString &jid);
	Q_SIGNAL void jidChanged();

	bool muted() const;
	void setMuted(bool muted);
	Q_SIGNAL void mutedChanged();

private:
	QString m_jid;
	bool m_muted = false;
};

