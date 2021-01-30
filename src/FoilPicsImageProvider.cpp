/*
 * Copyright (C) 2017-2021 Jolla Ltd.
 * Copyright (C) 2017-2021 Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FoilPicsImageProvider.h"
#include "FoilPicsImageRequest.h"
#include "HarbourDebug.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QWaitCondition>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOffscreenSurface>

// ==========================================================================
// FoilPicsImageProvider
// ==========================================================================

FoilPicsImageProvider::FoilPicsImageProvider(QObject* aObject, QQmlEngine* aEngine) :
    QQuickImageProvider(Image, ForceAsynchronousImageLoading),
    iMaxSize(3264),
    iId(QString().sprintf("foilpicsimage-%p", this)),
    iPrefix("image://" + iId + "/"),
    iObject(aObject),
    iEngine(aEngine)
{
    QOpenGLContext ctx;
    if (ctx.create()) {
        GLint maxSize = 0;
        QOffscreenSurface surface;
        surface.setFormat( ctx.format() );
        surface.create();
        ctx.makeCurrent(&surface);
        glEnable(GL_TEXTURE_2D);
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
        if (maxSize > 0) {
            iMaxSize = maxSize;
        }
        ctx.doneCurrent();
        surface.destroy();
    }
    HDEBUG(iPrefix << iMaxSize);
    HASSERT(iEngine);
    qRegisterMetaType<FoilPicsImageRequest>("FoilPicsImageRequest");
    if (iEngine) {
        iEngine->addImageProvider(iId, this);
    }
}

FoilPicsImageProvider::~FoilPicsImageProvider()
{
}

FoilPicsImageProvider* FoilPicsImageProvider::createForObject(QObject* aObject)
{
    QQmlContext* context = QQmlEngine::contextForObject(aObject);
    if (context) {
        QQmlEngine* engine = context->engine();
        if (engine) {
            return new FoilPicsImageProvider(aObject, engine);
        }
    }
    return NULL;
}

void FoilPicsImageProvider::release()
{
    if (iEngine) {
        iEngine->removeImageProvider(iId);
    } else {
        delete this;
    }
}

QString FoilPicsImageProvider::addImage(QString aId, QString aPath)
{
    QMutexLocker locker(&iMutex);
    iPathMap.insert(aId, aPath);
    return iPrefix + aId;
}

void FoilPicsImageProvider::releaseImage(QString aId)
{
    QMutexLocker locker(&iMutex);
    iPathMap.remove(aId);
}

QImage FoilPicsImageProvider::requestImage(const QString& aId, QSize* aSize,
    const QSize& aRequested)
{
    QImage image;
    FoilPicsImageRequest req;

    iMutex.lock();
    QString path = iPathMap.value(aId);
    iMutex.unlock();

    if (!path.isEmpty()) {
        if (QMetaObject::invokeMethod(iObject, "imageRequest",
            Qt::QueuedConnection, Q_ARG(QString, path),
            Q_ARG(FoilPicsImageRequest, req))) {
            HDEBUG("Waiting for" << aId << "=>" << qPrintable(path));
            image = req.wait();
        }
        const int width = image.width();
        const int height = image.height();
        if (width > height) {
            if (width > iMaxSize) {
                HDEBUG(aId << image.size() << "(scaling)");
                image = image.scaledToWidth(iMaxSize, Qt::SmoothTransformation);
            }
        } else if (height > iMaxSize) {
            HDEBUG(aId << image.size() << "(scaling)");
            image = image.scaledToHeight(iMaxSize, Qt::SmoothTransformation);
        }
        if (aSize) {
            *aSize = image.size();
        }
    }

    if (!image.isNull()) {
        HDEBUG(aId << image.size());
    } else {
        HWARN(aId << "oops!");
    }
    return image;
}
