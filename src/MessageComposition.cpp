// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageComposition.h"
#include "MessageHandler.h"
#include "Kaidan.h"

MessageComposition::MessageComposition()
{
}

void MessageComposition::setAccount(const QString &account)
{
	if (m_account != account) {
		m_account = account;
		emit accountChanged();
	}
}

void MessageComposition::setTo(const QString &to)
{
	if (m_to != to) {
		m_to = to;
		emit toChanged();
	}
}

void MessageComposition::setBody(const QString &body)
{
	if (m_body != body) {
		m_body = body;
		emit bodyChanged();
	}
}

void MessageComposition::setSpoiler(bool spoiler)
{
	if (m_spoiler != spoiler) {
		m_spoiler = spoiler;
		emit isSpoilerChanged();
	}
}

void MessageComposition::setSpoilerHint(const QString &spoilerHint)
{
	if (m_spoilerHint != spoilerHint) {
		m_spoilerHint = spoilerHint;
		emit spoilerHintChanged();
	}
}

void MessageComposition::send()
{
	Q_ASSERT(!m_account.isNull());
	Q_ASSERT(!m_to.isNull());
	emit Kaidan::instance()->client()->messageHandler()->sendMessageRequested(m_to, m_body, m_spoiler, m_spoilerHint);
	setSpoiler(false);
}
