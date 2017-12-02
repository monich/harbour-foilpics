/*
 * Copyright (C) 2017 Jolla Ltd.
 * Copyright (C) 2017 Slava Monich <slava@monich.com>
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
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the name of Jolla Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
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
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FoilPicsImageProvider.h"
#include "FoilPicsImageRequest.h"
#include "HarbourDebug.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QWaitCondition>

// ==========================================================================
// FoilPicsImageProvider
// ==========================================================================

FoilPicsImageProvider::FoilPicsImageProvider(QObject* aObject, QQmlEngine* aEngine) :
    QQuickImageProvider(Image, ForceAsynchronousImageLoading),
    iId(QString().sprintf("foilpicsimage-%p", this)),
    iPrefix("image://" + iId + "/"),
    iObject(aObject),
    iEngine(aEngine)
{
    HDEBUG(iPrefix);
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

QString FoilPicsImageProvider::prefix() const
{
    return iPrefix;
}

QImage FoilPicsImageProvider::requestImage(const QString& aId, QSize* aSize,
    const QSize& aRequested)
{
    QImage image;
    FoilPicsImageRequest req;
    if (QMetaObject::invokeMethod(iObject, "imageRequest", Qt::QueuedConnection,
        Q_ARG(QString, aId), Q_ARG(FoilPicsImageRequest, req))) {
        HDEBUG("Waiting for" << qPrintable(aId));
        image = req.wait();
    }
    if (aSize) {
        *aSize = image.size();
    }
    if (!image.isNull()) {
        HDEBUG(aId << image.size());
    } else {
        HWARN(aId << "oops!");
    }
    return image;
}
