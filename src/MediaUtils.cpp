// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MediaUtils.h"
#include "Globals.h"
#include "FutureUtils.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QTime>
#include <QUrl>
#include <QBuffer>
#include <QMimeType>
#include <QImage>
#include <QImageReader>
#include <QPixmap>

// KDE
#include <KIO/PreviewJob>
#include <KFileItem>

static QList<QMimeType> mimeTypes(const QList<QMimeType> &mimeTypes, const QString &parent);

const QMimeDatabase MediaUtils::s_mimeDB;
const QList<QMimeType> MediaUtils::s_allMimeTypes(MediaUtils::s_mimeDB.allMimeTypes());
const QList<QMimeType> MediaUtils::s_imageTypes(::mimeTypes(MediaUtils::s_allMimeTypes, QStringLiteral("image")));
const QList<QMimeType> MediaUtils::s_audioTypes(::mimeTypes(MediaUtils::s_allMimeTypes, QStringLiteral("audio")));
const QList<QMimeType> MediaUtils::s_videoTypes(::mimeTypes(MediaUtils::s_allMimeTypes, QStringLiteral("video")));
const QList<QMimeType> MediaUtils::s_documentTypes {
	s_mimeDB.mimeTypeForName(QStringLiteral("application/vnd.oasis.opendocument.presentation")),
	s_mimeDB.mimeTypeForName(QStringLiteral("application/vnd.oasis.opendocument.spreadsheet")),
	s_mimeDB.mimeTypeForName(QStringLiteral("application/vnd.oasis.opendocument.text")),
	s_mimeDB.mimeTypeForName(QStringLiteral("application/vnd.openxmlformats-officedocument.presentationml.presentation")),
	s_mimeDB.mimeTypeForName(QStringLiteral("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet")),
	s_mimeDB.mimeTypeForName(QStringLiteral("application/vnd.openxmlformats-officedocument.wordprocessingml.document")),
	s_mimeDB.mimeTypeForName(QStringLiteral("application/pdf")),
	s_mimeDB.mimeTypeForName(QStringLiteral("text/plain"))
};
const QList<QMimeType> MediaUtils::s_geoTypes {
	s_mimeDB.mimeTypeForName(QStringLiteral("application/geo+json"))
};
const QRegularExpression MediaUtils::s_geoLocationRegExp(QStringLiteral("geo:([-+]?[0-9]*\\.?[0-9]+),([-+]?[0-9]*\\.?[0-9]+)"));

static QList<QMimeType> mimeTypes(const QList<QMimeType> &mimeTypes, const QString &parent) {
	QList<QMimeType> mimes;

	for (const QMimeType &mimeType: mimeTypes) {
		if (mimeType.name().section(QLatin1Char('/'), 0, 0) == parent) {
			mimes << mimeType;
		}
	}

	return  mimes;
}

static QString iconName(const QList<QMimeType> &mimeTypes) {
	if (!mimeTypes.isEmpty()) {
		for (const QMimeType &type: mimeTypes) {
			const QString name = type.iconName();

			if (!name.isEmpty()) {
				return name;
			}
		}
	}

	return QStringLiteral("application-octet-stream");
}

static QString filter(const QList<QMimeType> &mimeTypes)
{
	QStringList filters;

	filters.reserve(mimeTypes.size() * 2);

	for (const QMimeType &mimeType : mimeTypes) {
		QString filter = mimeType.filterString();

		if (filter.isEmpty()) {
			filters << QStringLiteral("*");
			continue;
		}

		const int start = filter.lastIndexOf(QLatin1Char('('));
		const int end   = filter.lastIndexOf(QLatin1Char(')'));
		Q_ASSERT(start != -1);
		Q_ASSERT(end != -1);

		filters << filter.mid(start + 1, filter.size() - start - 1 - 1).split(QLatin1Char(' '));
	}

	if (filters.size() > 1) {
		filters.removeDuplicates();
		filters.removeAll(QStringLiteral("*"));
	}

	filters.reserve(filters.size());

	return filters.join(QLatin1Char(' '));
}

static QString prettyFormat(int durationMsecs, QTime *outTime = nullptr)
{
	const QTime time(QTime(0, 0, 0).addMSecs(durationMsecs));
	QString format;

	if (time.hour() > 0) {
		format.append(QStringLiteral("H"));
	}

	format.append(format.isEmpty() ? QStringLiteral("m") : QStringLiteral(":mm"));
	format.append(format.isEmpty() ? QStringLiteral("s") : QStringLiteral(":ss"));

	if (outTime) {
		*outTime = time;
	}

	return format;
}

QString MediaUtils::prettyDuration(int msecs)
{
	QTime time;
	const QString format = prettyFormat(msecs, &time);
	return time.toString(format);
}

QString MediaUtils::prettyDuration(int msecs, int durationMsecs)
{
	const QString format = prettyFormat(durationMsecs);
	return QTime(0, 0, 0).addMSecs(msecs).toString(format);
}

bool MediaUtils::isHttp(const QString &content)
{
	const QUrl url(content);
	return url.isValid() && url.scheme().startsWith(QStringLiteral("http"));
}

bool MediaUtils::isGeoLocation(const QString &content)
{
	return s_geoLocationRegExp.match(content).hasMatch();
}

QGeoCoordinate MediaUtils::locationCoordinate(const QString &content)
{
	const QRegularExpressionMatch match = s_geoLocationRegExp.match(content);
	return match.hasMatch()
		       ? QGeoCoordinate(match.captured(1).toDouble(), match.captured(2).toDouble())
		       : QGeoCoordinate();
}

bool MediaUtils::localFileAvailable(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return false;
	}

	return QFile::exists(filePath);
}

QUrl MediaUtils::fromLocalFile(const QString &filePath)
{
	return QUrl::fromLocalFile(filePath);
}

QMimeType MediaUtils::mimeType(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return {};
	}

	const QUrl url(filePath);
	return url.isValid() && !url.scheme().isEmpty()
		       ? mimeType(url)
		       : s_mimeDB.mimeTypeForFile(filePath);
}

QMimeType MediaUtils::mimeType(const QUrl &url)
{
	if (!url.isValid()) {
		return {};
	}

	if (url.isLocalFile()) {
		return mimeType(url.toLocalFile());
	}

	if (url.scheme().compare(QStringLiteral("geo")) == 0) {
		return s_geoTypes.first();
	}

	return mimeType(url.fileName());
}

QString MediaUtils::iconName(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return {};
	}

	const QUrl url(filePath);
	return url.isValid() && !url.scheme().isEmpty()
		       ? iconName(url)
		       : ::iconName({ mimeType(filePath) });
}

QString MediaUtils::iconName(const QUrl &url)
{
	if (!url.isValid()) {
		return {};
	}

	if (url.isLocalFile()) {
		return iconName(url.toLocalFile());
	}

	return iconName(url.fileName());
}

QString MediaUtils::mediaTypeName(Enums::MessageType mediaType)
{
	switch (mediaType) {
	case Enums::MessageType::MessageAudio:
		return tr("Audio");
	case Enums::MessageType::MessageDocument:
		return tr("Document");
	case Enums::MessageType::MessageGeoLocation:
		return tr("Location");
	case Enums::MessageType::MessageImage:
		return tr("Image");
	case Enums::MessageType::MessageVideo:
		return tr("Video");
	case Enums::MessageType::MessageFile:
	case Enums::MessageType::MessageText:
	case Enums::MessageType::MessageUnknown:
		return tr("File");
	}
	return {};
}

QString MediaUtils::newMediaLabel(Enums::MessageType hint)
{
	switch (hint) {
	case Enums::MessageType::MessageImage:
		return tr("Take picture");
	case Enums::MessageType::MessageVideo:
		return tr("Record video");
	case Enums::MessageType::MessageAudio:
		return tr("Record voice");
	case Enums::MessageType::MessageGeoLocation:
		return tr("Send location");
	case Enums::MessageType::MessageText:
	case Enums::MessageType::MessageFile:
	case Enums::MessageType::MessageDocument:
	case Enums::MessageType::MessageUnknown:
		break;
	}

	Q_UNREACHABLE();
	return { };
}

QString MediaUtils::newMediaIconName(Enums::MessageType hint)
{
	switch (hint) {
	case Enums::MessageType::MessageImage:
		return QStringLiteral("camera-photo-symbolic");
	case Enums::MessageType::MessageVideo:
		return QStringLiteral("camera-video-symbolic");
	case Enums::MessageType::MessageAudio:
		return QStringLiteral("audio-input-microphone-symbolic");
	case Enums::MessageType::MessageGeoLocation:
		return QStringLiteral("mark-location-symbolic");
	case Enums::MessageType::MessageText:
	case Enums::MessageType::MessageFile:
	case Enums::MessageType::MessageDocument:
	case Enums::MessageType::MessageUnknown:
		break;
	}

	Q_UNREACHABLE();
	return { };
}

QString MediaUtils::label(Enums::MessageType hint)
{
	switch (hint) {
	case Enums::MessageType::MessageFile:
		return tr("Choose file");
	case Enums::MessageType::MessageImage:
		return tr("Choose image");
	case Enums::MessageType::MessageVideo:
		return tr("Choose video");
	case Enums::MessageType::MessageAudio:
		return tr("Choose audio file");
	case Enums::MessageType::MessageDocument:
		return tr("Choose document");
	case Enums::MessageType::MessageGeoLocation:
	case Enums::MessageType::MessageText:
	case Enums::MessageType::MessageUnknown:
		break;
	}

	Q_UNREACHABLE();
	return { };
}

QString MediaUtils::iconName(Enums::MessageType hint)
{
	return ::iconName(mimeTypes(hint));
}

QString MediaUtils::filterName(Enums::MessageType hint)
{
	switch (hint) {
	case Enums::MessageType::MessageText:
		break;
	case Enums::MessageType::MessageFile:
		return tr("All files");
	case Enums::MessageType::MessageImage:
		return tr("Images");
	case Enums::MessageType::MessageVideo:
		return tr("Videos");
	case Enums::MessageType::MessageAudio:
		return tr("Audio files");
	case Enums::MessageType::MessageDocument:
		return tr("Documents");
	case Enums::MessageType::MessageGeoLocation:
	case Enums::MessageType::MessageUnknown:
		break;
	}

	Q_UNREACHABLE();
	return { };
}

QString MediaUtils::filter(Enums::MessageType hint)
{
	return ::filter(mimeTypes(hint));
}

QString MediaUtils::namedFilter(Enums::MessageType hint)
{
	switch (hint) {
	case Enums::MessageType::MessageText:
		break;
	case Enums::MessageType::MessageFile:
	case Enums::MessageType::MessageImage:
	case Enums::MessageType::MessageVideo:
	case Enums::MessageType::MessageAudio:
	case Enums::MessageType::MessageDocument:
		return tr("%1 (%2)").arg(filterName(hint), filter(hint));
	case Enums::MessageType::MessageGeoLocation:
	case Enums::MessageType::MessageUnknown:
		break;
	}

	Q_UNREACHABLE();
	return { };
}

QList<QMimeType> MediaUtils::mimeTypes(Enums::MessageType hint)
{
	switch (hint) {
	case Enums::MessageType::MessageImage:
		return s_imageTypes;
	case Enums::MessageType::MessageVideo:
		return s_videoTypes;
	case Enums::MessageType::MessageAudio:
		return s_audioTypes;
	case Enums::MessageType::MessageDocument:
		return s_documentTypes;
	case Enums::MessageType::MessageGeoLocation:
		return s_geoTypes;
	case Enums::MessageType::MessageText:
	case Enums::MessageType::MessageUnknown:
		break;
	case Enums::MessageType::MessageFile:
		return { s_mimeDB.mimeTypeForName(QStringLiteral("application/octet-stream")) };
	}

	Q_UNREACHABLE();
	return { };
}

Enums::MessageType MediaUtils::messageType(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return Enums::MessageType::MessageUnknown;
	}

	const QUrl url(filePath);
	return url.isValid() && !url.scheme().isEmpty()
		       ? messageType(url)
		       : messageType(s_mimeDB.mimeTypeForFile(filePath));
}

Enums::MessageType MediaUtils::messageType(const QUrl &url)
{
	if (!url.isValid()) {
		return Enums::MessageType::MessageUnknown;
	}

	if (url.isLocalFile()) {
		return messageType(url.toLocalFile());
	}

	QList<QMimeType> mimeTypes;

	if (url.scheme().compare(QStringLiteral("geo")) == 0) {
		mimeTypes = s_geoTypes;
	} else {
		const QFileInfo fileInfo(url.fileName());

		if (fileInfo.completeSuffix().isEmpty()) {
			return Enums::MessageType::MessageUnknown;
		}

		mimeTypes = s_mimeDB.mimeTypesForFileName(fileInfo.fileName());
	}

	for (const QMimeType &mimeType : std::as_const(mimeTypes)) {
		const Enums::MessageType messageType = MediaUtils::messageType(mimeType);

		if (messageType != Enums::MessageType::MessageUnknown) {
			return messageType;
		}
	}

	return Enums::MessageType::MessageUnknown;
}

Enums::MessageType MediaUtils::messageType(const QMimeType &mimeType)
{
	if (!mimeType.isValid()) {
		return Enums::MessageType::MessageUnknown;
	}

	if (mimeTypes(Enums::MessageType::MessageImage).contains(mimeType)) {
		return Enums::MessageType::MessageImage;
	} else if (mimeTypes(Enums::MessageType::MessageAudio).contains(mimeType)) {
		return Enums::MessageType::MessageAudio;
	} else if (mimeTypes(Enums::MessageType::MessageVideo).contains(mimeType)) {
		return Enums::MessageType::MessageVideo;
	} else if (mimeTypes(Enums::MessageType::MessageGeoLocation).contains(mimeType)) {
		return Enums::MessageType::MessageGeoLocation;
	} else if (mimeTypes(Enums::MessageType::MessageDocument).contains(mimeType)) {
		return Enums::MessageType::MessageDocument;
	}

	return Enums::MessageType::MessageFile;
}

QByteArray MediaUtils::encodeImageThumbnail(const QPixmap &pixmap)
{
	QByteArray output;
	QBuffer buffer(&output);
	pixmap.save(&buffer, "PNG");
	return output;
}

QByteArray MediaUtils::encodeImageThumbnail(const QImage &image)
{
	QByteArray output;
	QBuffer buffer(&output);
	image.save(&buffer, "PNG");
	return output;
}

QFuture<std::shared_ptr<QXmppFileSharingManager::MetadataGeneratorResult>> MediaUtils::generateMetadata(std::unique_ptr<QIODevice> f)
{
	using Result = QXmppFileSharingManager::MetadataGeneratorResult;
	using Thumnbnail = QXmppFileSharingManager::MetadataThumbnail;

	thread_local static auto allPlugins = KIO::PreviewJob::availablePlugins();

	auto result = std::make_shared<Result>();

	auto *file = dynamic_cast<QFile *>(f.get());
	if (!file) {
		// other io devices can't be handled
		return makeReadyFuture<std::shared_ptr<Result>>(std::move(result));
	}

	QFutureInterface<std::shared_ptr<Result>> interface;

	// create job
	auto *job = new KIO::PreviewJob(
		{ KFileItem(QUrl::fromLocalFile(file->fileName())) },
		QSize(THUMBNAIL_PIXEL_SIZE, THUMBNAIL_PIXEL_SIZE),
		&allPlugins
	);
	job->setAutoDelete(true);

	connect(job, &KIO::PreviewJob::gotPreview, [=, f = std::move(f)](const KFileItem &, const QPixmap &image) mutable {
		QByteArray thumbnailData;
		QBuffer thumbnailBuffer(&thumbnailData);
		image.save(&thumbnailBuffer, "JPG", JPEG_EXPORT_QUALITY);

		if (const QImageReader reader(file->fileName()); reader.canRead()) {
			if (const QSize size = reader.size(); !size.isEmpty()) {
				result->dimensions = size;
			}
		}

		result->dataDevice = std::move(f);
		result->thumbnails.push_back(Thumnbnail {
			.width = uint32_t(image.size().width()),
			.height = uint32_t(image.size().height()),
			.data = thumbnailData,
			.mimeType = QMimeDatabase().mimeTypeForData(thumbnailData)
		});

		interface.reportResult(result);
		interface.reportFinished();
	});

	connect(job, &KIO::PreviewJob::failed, [interface, result]() mutable {
		interface.reportResult(result);
		interface.reportFinished();
	});

	return interface.future();
}
