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
#include <QStandardPaths>

#undef THUMBNAILER_DEBUG

#ifdef THUMBNAILER_DEBUG
#define TDEBUG qDebug
#else
#define TDEBUG if(false)qDebug
#endif

namespace {

inline QString cachePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/.nemothumbs";
}

inline QString rawCachePath()
{
    return cachePath() + QDir::separator() + "raw";
}

void setupCache()
{
    QDir d(cachePath());
    if (!d.exists())
        d.mkpath(cachePath());
    d.mkdir("raw");
}

QString cacheFileName(const QByteArray &hashKey, bool makePath = false)
{
    QString subfolder = QString(hashKey.left(2));
    if (makePath) {
        QDir d(rawCachePath());
        d.mkdir(subfolder);
    }

    return rawCachePath() +
           QDir::separator() +
           subfolder +
           QDir::separator() +
           hashKey;
}

QByteArray cacheKey(const QString &id, const QSize &requestedSize, bool crop)
{
    const QByteArray baId = id.toUtf8(); // is there a more efficient way than a copy?

    // check if we have it in cache
    QCryptographicHash hash(QCryptographicHash::Sha1);

    hash.addData(baId.constData(), baId.length());
    return hash.result().toHex() + "nemo" +
           QString::number(requestedSize.width()).toLatin1() + "x" +
           QString::number(requestedSize.height()).toLatin1() +
           (crop ? "" : "F");
}

QString attemptCachedServe(const QString &id, const QByteArray &hashKey)
{
    QFile fi(cacheFileName(hashKey));
    QFileInfo info(fi);
    if (info.exists() && info.lastModified() >= QFileInfo(id).lastModified()) {
        if (fi.open(QIODevice::ReadOnly)) {
            // cached file exists! hooray.
            return fi.fileName();
        }
    }

    return QString();
}

QString writeCacheFile(const QByteArray &hashKey, const QImage &img)
{
    const QString thumbnailPath(cacheFileName(hashKey, true));
    QFile thumbnailFile(thumbnailPath);
    if (!thumbnailFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't cache to " << thumbnailFile.fileName();
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

NemoThumbnailCache::ThumbnailData generateImageThumbnail(const QString &path, const QByteArray &hashData, const QSize &requestedSize, bool crop)
{
    QString thumbnailPath;
    QImage img;

    // image was not in cache thus we read it
    QImageReader ir(path);
    if (ir.canRead()) {
        const QSize originalSize = ir.size();
        const QByteArray format = ir.format();

        if (originalSize != requestedSize && originalSize.isValid()) {
            if (crop) {
                // scales arbitrary sized source image to requested size scaling either up or down
                // keeping aspect ratio of the original image intact by maximizing either width or height
                // and cropping the rest of the image away
                QSize scaledSize(originalSize);

                // now scale it filling the original rectangle by keeping aspect ratio, but expand if needed.
                scaledSize.scale(requestedSize, Qt::KeepAspectRatioByExpanding);

                // set the adjusted clipping rectangle in the center of the scaled image
                QPoint center((scaledSize.width() - 1) / 2, (scaledSize.height() - 1) / 2);
                QRect cr(0,0,requestedSize.width(), requestedSize.height());
                cr.moveCenter(center);
                ir.setScaledClipRect(cr);

                // set requested target size of a thumbnail
                ir.setScaledSize(scaledSize);
            } else {
                // Maintains correct aspect ratio without cropping, as such the final image may
                // be smaller than requested in one dimension.
                QSize scaledSize(originalSize);
                scaledSize.scale(requestedSize, Qt::KeepAspectRatio);
                ir.setScaledSize(scaledSize);
            }
        }
        img = ir.read();

        NemoImageMetadata meta(path, format);
        if (meta.orientation() != NemoImageMetadata::TopLeft)
            img = rotate(img, meta.orientation());

        // write the scaled image to cache
        if (meta.orientation() != NemoImageMetadata::TopLeft ||
            (originalSize != requestedSize && originalSize.isValid())) {
            thumbnailPath = writeCacheFile(hashData, img);
            TDEBUG() << Q_FUNC_INFO << "Wrote " << path << " to cache";
        } else {
            // Return the original file path
            thumbnailPath = path;
        }
    }

    return NemoThumbnailCache::ThumbnailData(thumbnailPath, img);
}

typedef QImage (*CreateThumbnailFunc)(const QString &path, const QSize &requestedSize, bool crop);

NemoThumbnailCache::ThumbnailData generateVideoThumbnail(const QString &path, const QByteArray &hashData, const QSize &requestedSize, bool crop)
{
    static CreateThumbnailFunc createVideoThumbnail = (CreateThumbnailFunc)QLibrary::resolve(
                QLatin1String(NEMO_THUMBNAILER_DIR "/libvideothumbnailer.so"), "createThumbnail");

    QString thumbnailPath;
    QImage img;
    if (createVideoThumbnail) {
        img = createVideoThumbnail(path, requestedSize, crop);
        if (!img.isNull()) {
            thumbnailPath = writeCacheFile(hashData, img);
            TDEBUG() << Q_FUNC_INFO << "Wrote " << path << " to cache";
        }
    } else {
        qWarning("Cannot generate video thumbnail, thumbnailer function not available.");
    }

    return NemoThumbnailCache::ThumbnailData(thumbnailPath, img);
}

NemoThumbnailCache::ThumbnailData generateThumbnail(const QString &path, const QByteArray &hashData, const QSize &requestedSize, bool crop, const QString &mimeType)
{
    if (mimeType.startsWith("video/")) {
        return generateVideoThumbnail(path, hashData, requestedSize, crop);
    }

    // Assume image data
    return generateImageThumbnail(path, hashData, requestedSize, crop);
}

thread_local NemoThumbnailCache *cacheInstance = 0;

}

NemoThumbnailCache::ThumbnailData::ThumbnailData(const QString &path, const QImage &image)
    : path_(path)
    , image_(image)
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


NemoThumbnailCache *NemoThumbnailCache::instance()
{
    if (!cacheInstance) {
        setupCache();
        cacheInstance = new NemoThumbnailCache;
    }
    return cacheInstance;
}

QByteArray NemoThumbnailCache::requestId(const QString &uri, const QSize &requestedSize, bool crop) const
{
    const QString path(imagePath(uri));
    return cacheKey(path, requestedSize, crop);
}

NemoThumbnailCache::ThumbnailData NemoThumbnailCache::requestThumbnail(const QString &uri, const QSize &requestedSize, bool crop, const QString &mimeType)
{
    const QString path(imagePath(uri));
    if (!path.isEmpty()) {
        const QByteArray hashData = cacheKey(path, requestedSize, crop);
        QString thumbnailPath = attemptCachedServe(path, hashData);
        if (!thumbnailPath.isEmpty()) {
            TDEBUG() << Q_FUNC_INFO << "Read " << path << " from cache";
            return ThumbnailData(thumbnailPath, QImage());
        }

        return generateThumbnail(path, hashData, requestedSize, crop, mimeType);
    }

    return ThumbnailData(QString(), QImage());
}

QString NemoThumbnailCache::existingThumbnail(const QString &uri, const QSize &requestedSize, bool crop) const
{
    const QString path(imagePath(uri));
    if (!path.isEmpty()) {
        const QByteArray hashData = cacheKey(path, requestedSize, crop);
        QString thumbnailPath = attemptCachedServe(path, hashData);
        if (!thumbnailPath.isEmpty()) {
            TDEBUG() << Q_FUNC_INFO << "Read " << path << " from cache";
            return thumbnailPath;
        }
    }

    return QString();
}

