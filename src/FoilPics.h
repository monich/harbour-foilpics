/*
 * Copyright (C) 2017-2022 Jolla Ltd.
 * Copyright (C) 2017-2022 Slava Monich <slava@monich.com>
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
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FOILPICS_H
#define FOILPICS_H

#include <QtQml>

class FoilPics : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FoilPics)
    Q_PROPERTY(bool otherFoilAppsInstalled READ otherFoilAppsInstalled NOTIFY otherFoilAppsInstalledChanged)

public:
    FoilPics(QObject* aParent = Q_NULLPTR);
    ~FoilPics();

    static QObject* createSingleton(QQmlEngine* aEngine, QJSEngine* aScript);

    bool otherFoilAppsInstalled() const;

    Q_INVOKABLE bool deleteFile(QString aPath);
    Q_INVOKABLE bool deleteLocalFile(QUrl aUrl);
    Q_INVOKABLE void deleteLocalFilesFromModel(QObject* aModel, QString aRole,
        QList<int> aRows);
    Q_INVOKABLE QString formatFileSize(qlonglong aBytes);

    static void mediaDeleted(QString aFilename);
    void mediaDeleted(QUrl aUrl);

Q_SIGNALS:
    void otherFoilAppsInstalledChanged();

private:
    class Private;
    class TrackerProxy;
    Private* iPrivate;
};

#endif // FOILPICS_H
