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

#ifndef FOILPICS_IMAGE_PROVIDER_H
#define FOILPICS_IMAGE_PROVIDER_H

#include <QMutex>
#include <QImage>
#include <QQuickImageProvider>

class QQmlEngine;

class FoilPicsImageProvider : public QQuickImageProvider
{
private:
    // This object is deleted by unregistering it
    FoilPicsImageProvider(QObject* aObject, QQmlEngine* aEngine);
    ~FoilPicsImageProvider();

public:
    static FoilPicsImageProvider* createForObject(QObject* aObject);
    void release();

    QString addImage(QString aId, QString aPath);
    void releaseImage(QString aId);

    virtual QImage requestImage(const QString& aId, QSize* aSize,
        const QSize& aRequestedSize);

private:
    QString iId;
    QString iPrefix;
    QObject* iObject;
    QQmlEngine* iEngine;
    QMutex iMutex;
    QHash<QString, QString> iPathMap;
};

#endif // FOILPICS_IMAGE_PROVIDER_H
