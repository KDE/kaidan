// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImageProvider.h"

// Qt
#include <QClipboard>
#include <QFuture>
#include <QGuiApplication>
#include <QIcon>
#include <QImageReader>
#include <QMutex>
#include <QPointer>
// KDE
#include <KFileItem>
#include <KIO/PreviewJob>
#include <Prison/barcode.h>
// QXmpp
#include <QXmppBitsOfBinaryContentId.h>
#include <QXmppBitsOfBinaryData.h>
// Kaidan
#include "FutureUtils.h"
#include "Globals.h"
#include "KaidanCoreLog.h"
#include "MainController.h"
#include "MediaUtils.h"
#include "Message.h"

using namespace Qt::Literals::StringLiterals;

ImageProvider *ImageProvider::s_instance = nullptr;

QMutex m_cacheMutex;
std::vector<QXmppBitsOfBinaryData> m_cache;

const QString IMAGE_SCHEME = QStringLiteral("image");
const QString IMAGE_PROVIDER_PREFIX = QStringLiteral("%1://%2/").arg(IMAGE_SCHEME, IMAGE_PROVIDER_NAME);
const QString LOCAL_FILE_PATH_SEGMENT = QStringLiteral("local-file");
const QString ICON_PATH_SEGMENT = QStringLiteral("icon");
const QString QR_CODE_PATH_SEGMENT = QStringLiteral("qr-code");
const QString BASE64_PATH_SEGMENT = QStringLiteral("base64");
const QString SEPARATOR = QStringLiteral(":");

class AsyncImageResponse : public QQuickImageResponse
{
public:
    explicit AsyncImageResponse(const QString &id, const QSize &requestedSize, qreal devicePixelRatio)
    {
        // A Bits of Binary image has an unpredictable size and its corresponding QML image does not set its sourceSize.
        // Thus, it is separately handled.
        if (requestedSize.isEmpty() && !QXmppBitsOfBinaryContentId::isBitsOfBinaryContentId(id, true)) {
            QMetaObject::invokeMethod(this, &AsyncImageResponse::finished, Qt::QueuedConnection);
        } else {
            auto future = ImageProvider::generateImage(id, requestedSize, devicePixelRatio, this);
            future.then([this](QImage &&image) {
                m_image = std::move(image);
                Q_EMIT finished();
            });
        }
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    QImage m_image;
};

ImageProvider *ImageProvider::instance()
{
    return s_instance;
}

ImageProvider::ImageProvider()
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

ImageProvider::~ImageProvider()
{
    s_instance = nullptr;
}

QQuickImageResponse *ImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    return new AsyncImageResponse(id, requestedSize, m_screenDevicePixelRatio);
}

qreal ImageProvider::screenDevicePixelRatio() const
{
    return m_screenDevicePixelRatio;
}

void ImageProvider::setScreenDevicePixelRatio(qreal devicePixelRatio)
{
    if (m_screenDevicePixelRatio != devicePixelRatio) {
        m_screenDevicePixelRatio = devicePixelRatio;
        Q_EMIT screenDevicePixelRatioChanged();
    }
}

QFuture<QImage> ImageProvider::generateImage(const QUrl &localFileUrl, int edgePixelCount)
{
    return generateImageWithDevicePixelRatio(localFileUrl, m_screenDevicePixelRatio, edgePixelCount);
}

QFuture<QByteArray> ImageProvider::generateImageData(const QUrl &localFileUrl, int edgePixelCount)
{
    return generateImageDataWithDevicePixelRatio(localFileUrl, m_screenDevicePixelRatio, edgePixelCount);
}

void ImageProvider::copySourceToClipboard(const QUrl &source, const QSize &size)
{
    if (source.scheme().compare(IMAGE_SCHEME, Qt::CaseInsensitive) == 0 && source.host() == IMAGE_PROVIDER_NAME) {
        auto id = source.toString(QUrl::RemoveScheme | QUrl::RemoveAuthority);

        if (id.startsWith(QLatin1Char('/'))) {
            id.remove(0, 1);
        }

        auto future = generateImage(id, size, m_screenDevicePixelRatio, this);

        future.then(this, [=, this](QImage &&image) {
            qApp->clipboard()->setImage(image);
            Q_EMIT sourceCopiedToClipboard(source, size);
        });
    } else {
        Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not copy invalid source to clipboard: %1").arg(source.toString()));
    }
}

bool ImageProvider::addImage(const QXmppBitsOfBinaryData &data)
{
    QMutexLocker locker(&m_cacheMutex);

    if (!QImageReader::supportedMimeTypes().contains(data.contentType().name().toUtf8())) {
        return false;
    }

    m_cache.push_back(data);
    return true;
}

bool ImageProvider::removeImage(const QXmppBitsOfBinaryContentId &cid)
{
    QMutexLocker locker(&m_cacheMutex);

    auto itr = std::remove_if(m_cache.begin(), m_cache.end(), [&](const QXmppBitsOfBinaryData &item) {
        return item.cid() == cid;
    });

    if (itr == m_cache.end()) {
        return false;
    }

    m_cache.erase(itr, m_cache.end());

    return true;
}

QUrl ImageProvider::generatedFileImageUrl(const File &file)
{
    if (file.locallyAvailable()) {
        return generatedLocalFileImageUrl(file.localFilePath);
    }

    if (file.thumbnail.isEmpty()) {
        return generatedIconImageUrl(file.mimeTypeIcon());
    }

    return generatedBase64ImageUrl(file.thumbnail);
}

QUrl ImageProvider::generatedQrCodeImageUrl(const QString &text)
{
    return generatedImageUrl(QR_CODE_PATH_SEGMENT, text);
}

QUrl ImageProvider::generatedBitsOfBinaryImageUrl(const QUrl &cidUrl)
{
    return QUrl(QStringLiteral("%1%2").arg(IMAGE_PROVIDER_PREFIX, cidUrl.toString()));
}

QFuture<QImage> ImageProvider::generateImageWithDevicePixelRatio(const QUrl &localFileUrl, qreal devicePixelRatio, int edgePixelCount)
{
    auto promise = std::make_shared<QPromise<QImage>>();

    static auto allPlugins = KIO::PreviewJob::availablePlugins();

    auto *job = new KIO::PreviewJob({KFileItem(localFileUrl)}, effectiveSize(edgePixelCount), &allPlugins);
    job->setDevicePixelRatio(devicePixelRatio);
    job->setAutoDelete(true);

    QObject::connect(job, &KIO::PreviewJob::gotPreview, [promise](auto, const QPixmap &preview) {
        reportFinishedResult(*promise, preview.toImage());
    });

    QObject::connect(job, &KIO::PreviewJob::failed, [promise](const KFileItem &item) {
        qCDebug(KAIDAN_CORE_LOG) << "Could not generate thumbnail for" << item.url();
        reportFinishedResult(*promise, {});
    });

    job->start();

    return promise->future();
}

QFuture<QByteArray> ImageProvider::generateImageDataWithDevicePixelRatio(const QUrl &localFileUrl, qreal devicePixelRatio, int edgePixelCount)
{
    auto future = generateImageWithDevicePixelRatio(localFileUrl, devicePixelRatio, edgePixelCount);

    return future.then([](QImage &&image) {
        if (image.isNull()) {
            return QByteArray();
        }

        return MediaUtils::encodeImageThumbnail(std::move(image));
    });
}

QUrl ImageProvider::generatedLocalFileImageUrl(const QString &localFilePath)
{
    return generatedImageUrl(LOCAL_FILE_PATH_SEGMENT, localFilePath);
}

QUrl ImageProvider::generatedIconImageUrl(const QString &name)
{
    return generatedImageUrl(ICON_PATH_SEGMENT, name);
}

QUrl ImageProvider::generatedBase64ImageUrl(const QByteArray &data)
{
    return generatedImageUrl(BASE64_PATH_SEGMENT, QString::fromLatin1(data.toBase64()));
}

QUrl ImageProvider::generatedImageUrl(const QString &imageTypeUrlPathSegment, const QString &imageIdentifierUrlPathSegment)
{
    return QUrl(QStringLiteral("%1%2%3%4").arg(IMAGE_PROVIDER_PREFIX, imageTypeUrlPathSegment, SEPARATOR, imageIdentifierUrlPathSegment));
}

QSize ImageProvider::effectiveSize(int edgePixelCount)
{
    const auto edge = effectiveEdge(edgePixelCount);
    return QSize(edge, edge);
}

QSize ImageProvider::effectiveDevicePixelRatioSize(int edgePixelCount, qreal devicePixelRatio)
{
    const auto edge = effectiveEdge(edgePixelCount);
    return (QSizeF(edge, edge) * devicePixelRatio).toSize();
}

int ImageProvider::effectiveEdge(int edgePixelCount)
{
    return edgePixelCount <= 0 ? VIDEO_THUMBNAIL_EDGE_PIXEL_COUNT : edgePixelCount;
}

QFuture<QImage> ImageProvider::generateImage(const QString &id, const QSize &requestedSize, qreal devicePixelRatio, QObject *context)
{
    const int requestedEdge = std::max(requestedSize.width(), requestedSize.height());
    auto then = [id](QImage &&image) {
        auto url = QUrl(QStringLiteral("%1%2").arg(IMAGE_PROVIDER_PREFIX, id));
        QMetaObject::invokeMethod(ImageProvider::instance(),
                                  &ImageProvider::imageAlphaChannelChanged,
                                  Qt::QueuedConnection,
                                  std::move(url),
                                  image.hasAlphaChannel());
        return std::move(image);
    };

    if (QXmppBitsOfBinaryContentId::isBitsOfBinaryContentId(id, true)) {
        QMutexLocker locker(&m_cacheMutex);

        const auto item = std::ranges::find_if(m_cache, [&](const QXmppBitsOfBinaryData &item) {
            return item.cid().toCidUrl() == id;
        });

        if (item != m_cache.cend()) {
            return generateBitsOfBinaryImage(*item, requestedEdge).then(context, then);
        }
    }

    const QString key = id.section(SEPARATOR, 0, 0);
    const QString value = id.section(SEPARATOR, 1);

    if (key == LOCAL_FILE_PATH_SEGMENT) {
        const auto mimeTypeName = MediaUtils::mimeTypeName(value);
        const bool isImage = mimeTypeName.startsWith(QStringLiteral("image/"));
        const bool isVideo = mimeTypeName.startsWith(QStringLiteral("video/"));

        if (isImage || isVideo) {
            return generateLocalFileImage(value, std::max(requestedEdge, VIDEO_THUMBNAIL_EDGE_PIXEL_COUNT), devicePixelRatio, context).then(context, then);
        }

        return generateIconImage(MediaUtils::iconName(value), requestedEdge, devicePixelRatio).then(context, then);
    } else if (key == BASE64_PATH_SEGMENT) {
        return generateBase64Image(value.toLatin1()).then(context, then);
    } else if (key == ICON_PATH_SEGMENT) {
        return generateIconImage(value, requestedEdge, devicePixelRatio).then(context, then);
    } else if (key == QR_CODE_PATH_SEGMENT) {
        return generateQrCodeImage(value, requestedEdge, devicePixelRatio).then(context, then);
    }

    return QtFuture::makeReadyValueFuture(QImage()).then(context, then);
}

QFuture<QImage> ImageProvider::generateLocalFileImage(const QString &localFilePath, int edgePixelCount, qreal devicePixelRatio, QObject *context)
{
    auto promise = std::make_shared<QPromise<QImage>>();
    promise->start();

    if (QFile::exists(localFilePath)) {
        ImageProvider::generateImageWithDevicePixelRatio(QUrl::fromLocalFile(localFilePath), devicePixelRatio, edgePixelCount)
            .then(context, [promise](QImage &&thumbnail) {
                promise->addResult(std::move(thumbnail));
                promise->finish();
            });
    } else {
        Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not generate thumbnail: %1 does not exist").arg(localFilePath));
        promise->addResult({});
        promise->finish();
    }

    return promise->future();
}

QFuture<QImage> ImageProvider::generateIconImage(const QString &iconName, int edgePixelCount, qreal devicePixelRatio)
{
    auto promise = std::make_shared<QPromise<QImage>>();
    promise->start();
    promise->addResult(QIcon::fromTheme(iconName).pixmap(effectiveSize(edgePixelCount), devicePixelRatio).toImage());
    promise->finish();
    return promise->future();
}

QFuture<QImage> ImageProvider::generateQrCodeImage(const QString &text, int edgePixelCount, qreal devicePixelRatio)
{
    auto promise = std::make_shared<QPromise<QImage>>();
    promise->start();

    if (!text.isEmpty()) {
        try {
            auto qrCodeGenerator = Prison::Barcode::create(Prison::BarcodeType::QRCode);
            qrCodeGenerator->setData(text);
            auto image = qrCodeGenerator->toImage(effectiveDevicePixelRatioSize(edgePixelCount, devicePixelRatio));
            image.setDevicePixelRatio(devicePixelRatio);
            promise->addResult(std::move(image));
        } catch (const std::invalid_argument &e) {
            Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not generate QR code: %1").arg(QString::fromUtf8(e.what())));
            promise->addResult({});
        }
    }

    promise->finish();
    return promise->future();
}

QFuture<QImage> ImageProvider::generateBase64Image(const QByteArray &data)
{
    auto promise = std::make_shared<QPromise<QImage>>();
    promise->start();
    promise->addResult(QImage::fromData(QByteArray::fromBase64(data)));
    promise->finish();
    return promise->future();
}

QFuture<QImage> ImageProvider::generateBitsOfBinaryImage(const QXmppBitsOfBinaryData &data, int edgePixelCount)
{
    auto promise = std::make_shared<QPromise<QImage>>();
    promise->start();
    auto image = QImage::fromData(data.data());
    if (edgePixelCount != -1) {
        image = image.scaled(effectiveSize(edgePixelCount));
    }
    promise->addResult(std::move(image));
    promise->finish();
    return promise->future();
}

#include "moc_ImageProvider.cpp"
