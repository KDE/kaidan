// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "TrustMessageUriGenerator.h"

class ContactTrustMessageUriGenerator : public TrustMessageUriGenerator
{
	Q_OBJECT

	Q_PROPERTY(QString accountJid MEMBER m_accountJid WRITE setAccountJid)

public:
	explicit ContactTrustMessageUriGenerator(QObject *parent = nullptr);

	void setAccountJid(const QString &accountJid);

private:
	void handleJidChanged();
	void setUp();
	void handleKeysChanged(const QString &accountJid, const QList<QString> &jids);
	void updateKeys();

	QString m_accountJid;
};
