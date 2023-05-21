// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QQuickImageProvider>
#include <QMutex>
#include <QVector>
// QXmpp
#include <QXmppBitsOfBinaryData.h>

/**
 * Provider for images received via XEP-0231: Bits of Binary
 *
 * @note This class is thread-safe.
 */
class BitsOfBinaryImageProvider : public QQuickImageProvider
{
public:
	static BitsOfBinaryImageProvider *instance();

	BitsOfBinaryImageProvider();
	~BitsOfBinaryImageProvider();

	/**
	 * Creates a QImage from the cached data.
	 *
	 * @param id BitsOfBinary content URL of the requested image (starting with "cid:").
	 * @param size size of the cached image.
	 * @param requestedSize size the image should be scaled to. If this is invalid the image
	 * is not scaled.
	 */
	QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

	/**
	 * Adds @c QXmppBitsOfBinaryData to the cache used to provide images to QML.
	 *
	 * @param data Image in form of a Bits of Binary data element.
	 *
	 * @return True if the data has a supported image format. False if it could not be
	 * added to the cache.
	 */
	bool addImage(const QXmppBitsOfBinaryData &data);

	/**
	 * Removes the BitsOfBinary data associated with the given content ID from the cache.
	 *
	 * @param cid BitsOfBinary content ID to search for.
	 *
	 * @return True if the content ID could be found and the image was removed, False
	 * otherwise.
	 */
	bool removeImage(const QXmppBitsOfBinaryContentId &cid);

private:
	static BitsOfBinaryImageProvider *s_instance;
	QMutex m_cacheMutex;
	std::vector<QXmppBitsOfBinaryData> m_cache;
};
