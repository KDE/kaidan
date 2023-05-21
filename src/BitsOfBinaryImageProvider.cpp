// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "BitsOfBinaryImageProvider.h"

// Qt
#include <QImageReader>
#include <QMimeType>
#include <QMutexLocker>
// QXmpp
#include <QXmppBitsOfBinaryContentId.h>

BitsOfBinaryImageProvider *BitsOfBinaryImageProvider::s_instance;

BitsOfBinaryImageProvider *BitsOfBinaryImageProvider::instance()
{
	if (s_instance == nullptr)
		s_instance = new BitsOfBinaryImageProvider();

	return s_instance;
}

BitsOfBinaryImageProvider::BitsOfBinaryImageProvider()
	: QQuickImageProvider(QQuickImageProvider::Image)
{
	Q_ASSERT(!s_instance);
	s_instance = this;
}

BitsOfBinaryImageProvider::~BitsOfBinaryImageProvider()
{
	s_instance = nullptr;
}

QImage BitsOfBinaryImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
	QMutexLocker locker(&m_cacheMutex);

	const auto item = std::find_if(m_cache.begin(), m_cache.end(), [&](const QXmppBitsOfBinaryData &item) {
		return item.cid().toCidUrl() == id;
	});

	if (item == m_cache.end())
		return {};

	QImage image = QImage::fromData((*item).data());
	size->setWidth(image.width());
	size->setHeight(image.height());

	if (requestedSize.isValid())
		image = image.scaled(requestedSize);

	return image;
}

bool BitsOfBinaryImageProvider::addImage(const QXmppBitsOfBinaryData &data)
{
	QMutexLocker locker(&m_cacheMutex);

	if (!QImageReader::supportedMimeTypes().contains(data.contentType().name().toUtf8())) {
		return false;
	}

	m_cache.push_back(data);
	return true;
}

bool BitsOfBinaryImageProvider::removeImage(const QXmppBitsOfBinaryContentId &cid)
{
	QMutexLocker locker(&m_cacheMutex);

	auto endItr = std::remove_if(m_cache.begin(), m_cache.end(), [&](const QXmppBitsOfBinaryData &item) {
		return item.cid() == cid;
	});
	// check whether anything was removed
	bool success = endItr != m_cache.end();
	// resize container
	m_cache.erase(endItr, m_cache.end());
	return success;
}
