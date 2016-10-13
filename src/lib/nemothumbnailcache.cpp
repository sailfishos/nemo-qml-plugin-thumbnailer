/*
 * Copyright (C) 2012 Robin Burchell <robin+nemo@viroteck.net>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include "nemothumbnailcache.h"

#include "nemoimagemetadata.h"

#include <QLibrary>
#include <QFile>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QImageReader>
#include <QDateTime>
#include <QtEndian>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <MGConfItem>
#include <QStandardPaths>
#include <QProcess>

Q_LOGGING_CATEGORY(thumbnailer, "org.nemomobile.thumbnailer", QtWarningMsg)

namespace {

template <typename T, int N> constexpr int lengthOf(const T(&)[N]) { return N; }

bool acceptableUnboundedSize(const QSize &requestedSize, bool crop, unsigned size)
{
    const bool sufficientWidth(size >= (unsigned)requestedSize.width());
    const bool sufficientHeight(size >= (unsigned)requestedSize.height());
    return crop ? (sufficientWidth && sufficientHeight) : (sufficientWidth || sufficientHeight);
}

unsigned selectUnboundedSize(const QSize &requestedSize, unsigned screenWidth, unsigned screenHeight, bool crop)
{
    // Prefer a thumbnail size at least as large as the requested size
    const unsigned candidates[] = { NemoThumbnailCache::Small, NemoThumbnailCache::Medium, NemoThumbnailCache::Large, screenWidth, screenHeight };
    for (unsigned i = 0; i < lengthOf(candidates); ++i) {
        if (acceptableUnboundedSize(requestedSize, crop, candidates[i])) {
            return candidates[i];
        }
    }
    return NemoThumbnailCache::None;
}

bool acceptableBoundedSize(const QSize &requestedSize, unsigned size)
{
    const bool manageableWidth(size <= (unsigned)requestedSize.width());
    const bool manageableHeight(size <= (unsigned)requestedSize.height());
    return manageableWidth && manageableHeight;
}

unsigned selectBoundedSize(const QSize &requestedSize, unsigned screenWidth, unsigned screenHeight)
{
    // Select a size that does not exceed the requested size
    const unsigned candidates[] = { screenHeight, screenWidth, NemoThumbnailCache::Large, NemoThumbnailCache::Medium, NemoThumbnailCache::Small };
    for (unsigned i = 0; i < lengthOf(candidates); ++i) {
        if (acceptableBoundedSize(requestedSize, candidates[i])) {
            return candidates[i];
        }
    }
    return NemoThumbnailCache::None;
}

unsigned selectSize(const QSize &requestedSize, unsigned screenWidth, unsigned screenHeight, bool crop, bool unbounded)
{
    return unbounded
            ? selectUnboundedSize(requestedSize, screenWidth, screenHeight, crop)
            : selectBoundedSize(requestedSize, screenWidth, screenHeight);
}

unsigned increaseSize(unsigned size, unsigned screenWidth, unsigned screenHeight)
{
    const unsigned candidates[] = { NemoThumbnailCache::Small, NemoThumbnailCache::Medium, NemoThumbnailCache::Large, screenWidth, screenHeight };
    for (unsigned i = 0; i < lengthOf(candidates) - 1; ++i) {
        if (candidates[i] == size) {
            return candidates[i] + 1;
        }
    }
    return NemoThumbnailCache::None;
}

unsigned decreaseSize(unsigned size, unsigned screenWidth, unsigned screenHeight)
{
    const unsigned candidates[] = { screenHeight, screenWidth, NemoThumbnailCache::Large, NemoThumbnailCache::Medium, NemoThumbnailCache::Small };
    for (unsigned i = 0; i < lengthOf(candidates) - 1; ++i) {
        if (candidates[i] == size) {
            return candidates[i] + 1;
        }
    }
    return NemoThumbnailCache::None;
}

unsigned nextSize(unsigned size, unsigned screenWidth, unsigned screenHeight, bool unbounded)
{
    return unbounded
            ? increaseSize(size, screenWidth, screenHeight)
            : decreaseSize(size, screenWidth, screenHeight);
}

inline QString cachePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QDir::separator() + "org.nemomobile";
}

inline QString thumbnailsCachePath()
{
    return cachePath() + QDir::separator() + "thumbnails";
}

void setupCache()
{
    QDir d(cachePath());
    if (!d.exists())
        d.mkpath(cachePath());
    d.mkdir("thumbnails");
}

QString cachePath(const QByteArray &key, bool makePath = false)
{
    QString subfolder = QString(key.left(2));
    if (makePath) {
        QDir d(thumbnailsCachePath());
        d.mkdir(subfolder);
    }

    return thumbnailsCachePath() +
           QDir::separator() +
           subfolder +
           QDir::separator() +
           key;
}

QByteArray cacheKey(const QString &id, unsigned size, bool crop)
{
    const QByteArray baId = id.toUtf8(); // is there a more efficient way than a copy?

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(baId.constData(), baId.length());
    return hash.result().toHex() + "-" + QString::number(size).toLatin1() + (crop ? "" : "F");
}

QString attemptCachedServe(const QString &id, const QByteArray &key)
{
    QFile fi(cachePath(key));
    QFileInfo info(fi);
    if (info.exists() && info.lastModified() >= QFileInfo(id).lastModified()) {
        if (fi.open(QIODevice::ReadOnly)) {
            // cached file exists! hooray.
            return fi.fileName();
        }
    }

    return QString();
}

QString writeCacheFile(const QByteArray &key, const QImage &img)
{
    const QString thumbnailPath(cachePath(key, true));
    QFile thumbnailFile(thumbnailPath);
    if (!thumbnailFile.open(QIODevice::WriteOnly)) {
        qCWarning(thumbnailer) << "Couldn't cache to " << thumbnailFile.fileName();
        return QString();
    }
    img.save(&thumbnailFile, img.hasAlphaChannel() ? "PNG" : "JPG");
    thumbnailFile.flush();
    thumbnailFile.close();
    return thumbnailPath;
}

QImage rotate(const QImage &src, NemoImageMetadata::Orientation orientation)
{
    QTransform trans;
    QImage dst, tmp;

    /* For square images 90-degree rotations of the pixel could be
       done in-place, and flips could be done in-place for any image
       instead of using the QImage routines which make copies of the
       data. */

    switch (orientation) {
        case NemoImageMetadata::TopRight:
            /* horizontal flip */
            dst = src.mirrored(true, false);
            break;
        case NemoImageMetadata::BottomRight:
            /* horizontal flip, vertical flip */
            dst = src.mirrored(true, true);
            break;
        case NemoImageMetadata::BottomLeft:
            /* vertical flip */
            dst = src.mirrored(false, true);
            break;
        case NemoImageMetadata::LeftTop:
            /* rotate 90 deg clockwise and flip horizontally */
            trans.rotate(90.0);
            tmp = src.transformed(trans);
            dst = tmp.mirrored(true, false);
            break;
        case NemoImageMetadata::RightTop:
            /* rotate 90 deg anticlockwise */
            trans.rotate(90.0);
            dst = src.transformed(trans);
            break;
        case NemoImageMetadata::RightBottom:
            /* rotate 90 deg anticlockwise and flip horizontally */
            trans.rotate(-90.0);
            tmp = src.transformed(trans);
            dst = tmp.mirrored(true, false);
            break;
        case NemoImageMetadata::LeftBottom:
            /* rotate 90 deg clockwise */
            trans.rotate(-90.0);
            dst = src.transformed(trans);
            break;
        default:
            dst = src;
            break;
    }

    return dst;
}

QString imagePath(const QString &uri)
{
    QString path(uri);
    if (path.startsWith("file://")) {
        path = path.mid(7);
    }
    return path;
}

QImage scaleImage(const QImage &image, const QSize &requestedSize, bool crop, Qt::TransformationMode mode)
{
    const QImage scaledImage(image.size() != requestedSize
            ? image.scaled(requestedSize, crop ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio, mode)
            : image);

    if (crop && scaledImage.size() != requestedSize) {
        QRect cropRect(0, 0, requestedSize.width(), requestedSize.height());
        cropRect.moveCenter(QPoint(image.width() / 2, image.height() / 2));

        return scaledImage.copy(cropRect);
    } else {
        return scaledImage;
    }
}

QImage readImageThumbnail(
        QImageReader *reader,
        const QSize requestedSize,
        NemoImageMetadata::Orientation orientation,
        bool crop,
        Qt::TransformationMode mode)
{
    const QSize originalSize = reader->size();
    const QSize rotatedSize = orientation == NemoImageMetadata::LeftTop
                || orientation == NemoImageMetadata::RightTop
                || orientation == NemoImageMetadata::RightBottom
                || orientation == NemoImageMetadata::LeftBottom
            ? requestedSize.transposed()
            : requestedSize;

    if (originalSize.isValid()) {
        if (crop) {
            // scales arbitrary sized source image to requested size scaling either up or down
            // keeping aspect ratio of the original image intact by maximizing either width or height
            // and cropping the rest of the image away
            QSize scaledSize(originalSize);

            // now scale it filling the original rectangle by keeping aspect ratio, but expand if needed.
            scaledSize.scale(requestedSize, Qt::KeepAspectRatioByExpanding);

            // set the adjusted clipping rectangle in the center of the scaled image
            QPoint center((scaledSize.width() - 1) / 2, (scaledSize.height() - 1) / 2);
            QRect cr(0, 0, rotatedSize.width(), rotatedSize.height());
            cr.moveCenter(center);
            reader->setScaledClipRect(cr);

            // set requested target size of a thumbnail
            reader->setScaledSize(scaledSize);
        } else {
            // Maintains correct aspect ratio without cropping, as such the final image may
            // be smaller than requested in one dimension.
            QSize scaledSize(originalSize);
            scaledSize.scale(rotatedSize, Qt::KeepAspectRatio);
            reader->setScaledSize(scaledSize);
        }
    }

    QImage image(reader->read());

    if (!originalSize.isValid()) {
        image = scaleImage(image, rotatedSize, crop, mode);
    }

    return orientation != NemoImageMetadata::NemoImageMetadata::TopLeft
            ? rotate(image, orientation)
            : image;
}

NemoThumbnailCache::ThumbnailData generateImageThumbnail(const QString &path, const QByteArray &key, const int requestedSize, bool crop)
{
    // image was not in cache thus we read it
    QImageReader ir(path);
    if (ir.canRead()) {
        const QSize originalSize = ir.size();
        const QByteArray format = ir.format();

        NemoImageMetadata meta(path, format);

        if ((meta.orientation() == NemoImageMetadata::TopLeft || requestedSize > NemoThumbnailCache::ExtraLarge)
                && (originalSize.width() * 9 < requestedSize * 10 || originalSize.height() * 9 < requestedSize * 10)) {
            qCDebug(thumbnailer) << Q_FUNC_INFO << "Returning original image path " << path << requestedSize << originalSize;
            return NemoThumbnailCache::ThumbnailData(path, QImage(), requestedSize);
        }

        QImage img = readImageThumbnail(
                    &ir, QSize(requestedSize, requestedSize), meta.orientation(), crop, Qt::FastTransformation);

        // write the scaled image to cache
        QString thumbnailPath = writeCacheFile(key, img);
        qCDebug(thumbnailer) << Q_FUNC_INFO << "Wrote " << path << " to cache";

        return NemoThumbnailCache::ThumbnailData(thumbnailPath, img, requestedSize);
    }

    qCWarning(thumbnailer) << Q_FUNC_INFO << "Could not generateImageThumbnail:" << path << requestedSize << crop;
    return NemoThumbnailCache::ThumbnailData();
}

QStringList generatorArgs(const QString &path, const QString &thumbnailPath, const QSize &requestedSize, bool crop)
{
    QStringList args = {
        QFile::encodeName(path),
        "-w", QString::number(requestedSize.width()),
        "-h", QString::number(requestedSize.height()),
        "-o", QFile::encodeName(thumbnailPath)
    };
    if (crop) {
        args.append("-c");
    }

    return args;
}

NemoThumbnailCache::ThumbnailData generateVideoThumbnail(const QString &path, const QByteArray &key, const QSize &requestedSize, bool crop)
{
    const QString thumbnailPath(cachePath(key, true));

    int rv = QProcess::execute(QStringLiteral("/usr/bin/thumbnaild-video"), generatorArgs(path, thumbnailPath, requestedSize, crop));
    if (rv == 0) {
        qCDebug(thumbnailer) << Q_FUNC_INFO << "Wrote " << path << " to cache";
        return NemoThumbnailCache::ThumbnailData(thumbnailPath, QImage(), requestedSize.width());
    } else {
        qCWarning(thumbnailer) << Q_FUNC_INFO << "Could not generateVideoThumbnail:" << path << requestedSize << crop;
    }

    return NemoThumbnailCache::ThumbnailData();
}

NemoThumbnailCache::ThumbnailData generatePdfThumbnail(const QString &path, const QByteArray &key, const QSize &requestedSize, bool crop)
{
    const QString thumbnailPath(cachePath(key, true));

    int rv = QProcess::execute(QStringLiteral("/usr/bin/thumbnaild-pdf"), generatorArgs(path, thumbnailPath, requestedSize, crop));
    if (rv == 0) {
        qCDebug(thumbnailer) << Q_FUNC_INFO << "Wrote " << path << " to cache";
        return NemoThumbnailCache::ThumbnailData(thumbnailPath, QImage(), requestedSize.width());
    } else {
        qCWarning(thumbnailer) << Q_FUNC_INFO << "Could not generatePdfThumbnail:" << path << requestedSize << crop;
    }

    return NemoThumbnailCache::ThumbnailData();
}

NemoThumbnailCache::ThumbnailData generateThumbnail(const QString &path, const QByteArray &key, unsigned size, bool crop, const QString &mimeType)
{
    const QSize boundsSize(size, size);

    if (mimeType == QStringLiteral("application/pdf")) {
        return generatePdfThumbnail(path, key, boundsSize, crop);
    }

    if (mimeType.startsWith("video/")) {
        return generateVideoThumbnail(path, key, boundsSize, crop);
    }

    // Assume image data
    return generateImageThumbnail(path, key, size, crop);
}

thread_local NemoThumbnailCache *cacheInstance = 0;

}


NemoThumbnailCache::ThumbnailData::ThumbnailData()
    : size_(NemoThumbnailCache::None)
{
}

NemoThumbnailCache::ThumbnailData::ThumbnailData(const QString &path, const QImage &image, unsigned size)
    : path_(path)
    , image_(image)
    , size_(size)
{
}

bool NemoThumbnailCache::ThumbnailData::validPath() const
{
    return !path_.isEmpty();
}

QString NemoThumbnailCache::ThumbnailData::path() const
{
    return path_;
}

bool NemoThumbnailCache::ThumbnailData::validImage() const
{
    return !image_.isNull();
}

QImage NemoThumbnailCache::ThumbnailData::image() const
{
    return image_;
}

unsigned NemoThumbnailCache::ThumbnailData::size() const
{
    return size_;
}

QImage NemoThumbnailCache::ThumbnailData::getScaledImage(const QSize &requestedSize, bool crop, Qt::TransformationMode mode) const
{
    if (!image_.isNull()) {
        return scaleImage(image_, requestedSize, crop, mode);
    } else if (!path_.isEmpty()) {
        QImageReader reader(path_);

        NemoImageMetadata meta(path_, reader.format());

        return readImageThumbnail(&reader, requestedSize, meta.orientation(), crop, mode);
    } else {
        return QImage();
    }
}

NemoThumbnailCache::NemoThumbnailCache()
    : screenWidth_(MGConfItem(QStringLiteral("/lipstick/screen/primary/width")).value(540).toInt())
    , screenHeight_(MGConfItem(QStringLiteral("/lipstick/screen/primary/height")).value(960).toInt())
{
}

NemoThumbnailCache *NemoThumbnailCache::instance()
{
    if (!cacheInstance) {
        setupCache();
        cacheInstance = new NemoThumbnailCache;
    }
    return cacheInstance;
}

NemoThumbnailCache::ThumbnailData NemoThumbnailCache::requestThumbnail(const QString &uri, const QSize &requestedSize, bool crop, bool unbounded, const QString &mimeType)
{
    const QString path(imagePath(uri));
    if (!path.isEmpty()) {
        ThumbnailData existing(existingThumbnail(uri, requestedSize, crop, unbounded));
        if (existing.validPath()) {
            return existing;
        }

        const unsigned size(selectSize(requestedSize, screenWidth_, screenHeight_, crop, unbounded));
        if (size != None) {
            const QByteArray key = cacheKey(path, size, crop);
            return generateThumbnail(path, key, size, crop, mimeType);
        } else {
            qCWarning(thumbnailer) << Q_FUNC_INFO << "Invalid thumbnail size " << requestedSize << " for " << path;
        }
    }

    return ThumbnailData();
}

NemoThumbnailCache::ThumbnailData NemoThumbnailCache::existingThumbnail(const QString &uri, const QSize &requestedSize, bool crop, bool unbounded) const
{
    const QString path(imagePath(uri));
    if (!path.isEmpty()) {
        for (unsigned size(selectSize(requestedSize, screenWidth_, screenHeight_, crop, unbounded));
                size != None;
                size = nextSize(size, screenWidth_, screenHeight_, unbounded)) {
            const QByteArray key = cacheKey(path, size, crop);
            QString thumbnailPath = attemptCachedServe(path, key);
            if (!thumbnailPath.isEmpty()) {
                qCDebug(thumbnailer) << Q_FUNC_INFO << "Read " << path << " from cache";
                return ThumbnailData(thumbnailPath, QImage(), size);
            }
        }
    }

    return ThumbnailData();
}

