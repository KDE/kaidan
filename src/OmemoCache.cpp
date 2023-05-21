// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "OmemoCache.h"

OmemoCache *OmemoCache::s_instance = nullptr;

OmemoCache::OmemoCache(QObject *parent)
	: QObject(parent)
{
	Q_ASSERT(!s_instance);
	s_instance = this;
}

OmemoCache::~OmemoCache()
{
	s_instance = nullptr;
}
