// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QQuickAsyncImageProvider>

template<typename T>
class QFuture;

class QXmppBitsOfBinaryContentId;
class QXmppBitsOfBinaryData;

struct File;

/**
 * Asynchronous provider for all images.
 */
class ImageProvider : public QQuickAsyncImageProvider
{
    Q_OBJECT

    Q_PROPERTY(qreal screenDevicePixelRatio READ screenDevicePixelRatio WRITE setScreenDevicePixelRatio NOTIFY screenDevicePixelRatioChanged)

public:
    static ImageProvider *instance();

    ImageProvider();
    ~ImageProvider() override;

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    qreal screenDevicePixelRatio() const;
    void setScreenDevicePixelRatio(qreal devicePixelRatio);
    Q_SIGNAL void screenDevicePixelRatioChanged();

    Q_SIGNAL void imageAlphaChannelChanged(const QUrl &source, bool hasAlphaChannel);

    QFuture<QImage> generateImage(const QUrl &localFileUrl, int edgePixelCount);
    QFuture<QByteArray> generateImageData(const QUrl &localFileUrl, int edgePixelCount);

    Q_INVOKABLE void copySourceToClipboard(const QUrl &source, const QSize &size);
    Q_SIGNAL void sourceCopiedToClipboard(const QUrl &source, const QSize &size);

    bool addImage(const QXmppBitsOfBinaryData &data);
    bool removeImage(const QXmppBitsOfBinaryContentId &cid);

    Q_INVOKABLE static QUrl generatedFileImageUrl(const File &file);
    Q_INVOKABLE static QUrl generatedQrCodeImageUrl(const QString &text);
    static QUrl generatedBitsOfBinaryImageUrl(const QUrl &cidUrl);

    static QFuture<QImage> generateImageWithDevicePixelRatio(const QUrl &localFileUrl, qreal devicePixelRatio, int edgePixelCount);
    static QFuture<QByteArray> generateImageDataWithDevicePixelRatio(const QUrl &localFileUrl, qreal devicePixelRatio, int edgePixelCount);

private:
    static QUrl generatedLocalFileImageUrl(const QString &localFilePath);
    static QUrl generatedIconImageUrl(const QString &name);
    static QUrl generatedBase64ImageUrl(const QByteArray &data);
    static QUrl generatedImageUrl(const QString &imageTypeUrlPathSegment, const QString &imageIdentifierUrlPathSegment);

    static QSize effectiveSize(int edgePixelCount);
    static QSize effectiveDevicePixelRatioSize(int edgePixelCount, qreal devicePixelRatio);
    static int effectiveEdge(int edgePixelCount);

    static QFuture<QImage> generateImage(const QString &id, const QSize &requestedSize, qreal devicePixelRatio, QObject *context = nullptr);
    static QFuture<QImage> generateLocalFileImage(const QString &localFilePath, int edgePixelCount, qreal devicePixelRatio, QObject *context = nullptr);
    static QFuture<QImage> generateIconImage(const QString &iconName, int edgePixelCount, qreal devicePixelRatio);
    static QFuture<QImage> generateQrCodeImage(const QString &text, int edgePixelCount, qreal devicePixelRatio);
    static QFuture<QImage> generateBase64Image(const QByteArray &data);
    static QFuture<QImage> generateBitsOfBinaryImage(const QXmppBitsOfBinaryData &data, int edgePixelCount);

private:
    qreal m_screenDevicePixelRatio = 1.0;

    static ImageProvider *s_instance;

    friend class AsyncImageResponse;
};
