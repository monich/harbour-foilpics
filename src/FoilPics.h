/*
 * Copyright (C) 2017-2026 Slava Monich <slava@monich.com>
 * Copyright (C) 2017-2022 Jolla Ltd.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer
 *     in the documentation and/or other materials provided with the
 *     distribution.
 *
 *  3. Neither the names of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#ifndef FOILPICS_H
#define FOILPICS_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QUrl>

class QQmlEngine;
class QJSEngine;

class FoilPics :
    public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool otherFoilAppsInstalled READ otherFoilAppsInstalled NOTIFY otherFoilAppsInstalledChanged)
    Q_PROPERTY(QString documentGalleryItemQml READ documentGalleryItemQml CONSTANT)
    Q_PROPERTY(QString documentGalleryModelQml READ documentGalleryModelQml CONSTANT)
    Q_PROPERTY(QString thumbnailQml READ thumbnailQml CONSTANT)

public:
    FoilPics(QObject* aParent = Q_NULLPTR);

    // Callback for qmlRegisterSingletonType<FoilPics>
    static QObject* createSingleton(QQmlEngine*, QJSEngine*);

    bool otherFoilAppsInstalled() const;
    QString documentGalleryItemQml() const;
    QString documentGalleryModelQml() const;
    QString thumbnailQml() const;

    Q_INVOKABLE bool deleteFile(QString);
    Q_INVOKABLE bool deleteLocalFile(QUrl);
    Q_INVOKABLE void deleteLocalFilesFromModel(QObject*, QString, QList<int>);
    Q_INVOKABLE QString formatFileSize(qlonglong);
    Q_INVOKABLE void mediaDeleted(QUrl);

Q_SIGNALS:
    void otherFoilAppsInstalledChanged();

private:
    class Private;
    class TrackerProxy;
    Private* iPrivate;
};

#endif // FOILPICS_H
