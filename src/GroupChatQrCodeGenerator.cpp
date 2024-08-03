// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatQrCodeGenerator.h"

#include "qxmpp-exts/QXmppUri.h"

GroupChatQrCodeGenerator::GroupChatQrCodeGenerator(QObject *parent)
	: AbstractQrCodeGenerator(parent)
{
	connect(this, &GroupChatQrCodeGenerator::jidChanged, this, &GroupChatQrCodeGenerator::updateText);
}

void GroupChatQrCodeGenerator::updateText()
{
	QXmppUri uri;

	uri.setJid(jid());
	uri.setAction(QXmppUri::Join);

	setText(uri.toString());
}
