// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QGeoCoordinate>
#include <QMimeDatabase>
#include <QObject>
#include <QFuture>

#include <QXmppFileSharingManager.h>

#include "Enums.h"

class MediaUtils : public QObject
{
	Q_OBJECT

public:
	using QObject::QObject;

	Q_INVOKABLE static QString prettyDuration(int msecs);
	Q_INVOKABLE static QString prettyDuration(int msecs, int durationMsecs);
	Q_INVOKABLE static bool isHttp(const QString &content);
	Q_INVOKABLE static bool isGeoLocation(const QString &content);
	Q_INVOKABLE static QGeoCoordinate locationCoordinate(const QString &content);
	Q_INVOKABLE static bool localFileAvailable(const QString &filePath);
	Q_INVOKABLE static QUrl fromLocalFile(const QString &filePath);
	Q_INVOKABLE static QMimeType mimeType(const QString &filePath);
	Q_INVOKABLE static QMimeType mimeType(const QUrl &url);
	Q_INVOKABLE static QString iconName(const QString &filePath);
	Q_INVOKABLE static QString iconName(const QUrl &url);

	static QString mediaTypeName(Enums::MessageType mediaType);
	Q_INVOKABLE static QString newMediaLabel(Enums::MessageType hint);
	Q_INVOKABLE static QString newMediaIconName(Enums::MessageType hint);
	Q_INVOKABLE static QString label(Enums::MessageType hint);
	Q_INVOKABLE static QString iconName(Enums::MessageType hint);
	Q_INVOKABLE static QString filterName(Enums::MessageType hint);
	Q_INVOKABLE static QString filter(Enums::MessageType hint);
	Q_INVOKABLE static QString namedFilter(Enums::MessageType hint);
	Q_INVOKABLE static QList<QMimeType> mimeTypes(Enums::MessageType hint);
	Q_INVOKABLE static Enums::MessageType messageType(const QString &filePath);
	Q_INVOKABLE static Enums::MessageType messageType(const QUrl &url);
	Q_INVOKABLE static Enums::MessageType messageType(const QMimeType &mimeType);

	// Qml sucks... :)
	Q_INVOKABLE inline static QString mimeTypeName(const QString &filePath)
	{ return mimeType(filePath).name(); }

	Q_INVOKABLE inline static QString mimeTypeName(const QUrl &url)
	{ return mimeType(url).name(); }

	static QByteArray encodeImageThumbnail(const QPixmap &pixmap);
	static QByteArray encodeImageThumbnail(const QImage &image);

	static QFuture<std::shared_ptr<QXmppFileSharingManager::MetadataGeneratorResult>> generateMetadata(std::unique_ptr<QIODevice>);

	static const QMimeDatabase &mimeDatabase()
	{
		return s_mimeDB;
	}

private:
	static const QMimeDatabase s_mimeDB;
	static const QList<QMimeType> s_allMimeTypes;
	static const QList<QMimeType> s_imageTypes;
	static const QList<QMimeType> s_audioTypes;
	static const QList<QMimeType> s_videoTypes;
	static const QList<QMimeType> s_documentTypes;
	static const QList<QMimeType> s_geoTypes;
	static const QRegularExpression s_geoLocationRegExp;
};
