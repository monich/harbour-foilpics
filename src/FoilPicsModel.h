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

#ifndef FOILPICS_MODEL_H
#define FOILPICS_MODEL_H

#include <QtCore/QAbstractListModel>
#include <QtQml>

#include "foil_types.h"

#include "FoilPicsImageRequest.h"

class FoilPicsModel :
    public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(FoilState)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool keyAvailable READ keyAvailable NOTIFY keyAvailableChanged)
    Q_PROPERTY(FoilState foilState READ foilState NOTIFY foilStateChanged)
    Q_PROPERTY(QSize thumbnailSize READ thumbnailSize WRITE setThumbnailSize NOTIFY thumbnailSizeChanged)
    Q_PROPERTY(bool mayHaveEncryptedPictures READ mayHaveEncryptedPictures NOTIFY mayHaveEncryptedPicturesChanged)
    Q_PROPERTY(QObject* groupModel READ groupModel CONSTANT)

    class Private;
    class ModelData;
    class CheckPicsTask;
    class Task;

public:
    class ModelInfo;
    class DecryptPicsTask;

    enum FoilState {
        FoilKeyMissing,
        FoilKeyInvalid,
        FoilKeyError,
        FoilKeyNotEncrypted,
        FoilGeneratingKey,
        FoilLocked,
        FoilLockedTimedOut,
        FoilDecrypting,
        FoilPicsReady
    };

    explicit FoilPicsModel(QObject* aParent = Q_NULLPTR);

    bool busy() const;
    bool keyAvailable() const;
    FoilState foilState() const;
    bool mayHaveEncryptedPictures() const;
    QSize thumbnailSize() const;
    void setThumbnailSize(QSize);

    static int groupIdRole();
    QAbstractItemModel* groupModel();
    void clearGroup(QByteArray);

    // QAbstractListModel
    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& aParent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex&, int) const Q_DECL_OVERRIDE;

    Q_INVOKABLE void generateKey(int, QString);
    Q_INVOKABLE bool checkPassword(QString);
    Q_INVOKABLE bool changePassword(QString, QString);
    Q_INVOKABLE void lock(bool);
    Q_INVOKABLE bool unlock(QString);
    Q_INVOKABLE bool encryptFile(QUrl, QVariantMap);
    Q_INVOKABLE void encryptFiles(QObject*, QList<int>);
    Q_INVOKABLE void decryptFiles(QList<int>);
    Q_INVOKABLE void decryptAt(int);
    Q_INVOKABLE void decryptAll();
    Q_INVOKABLE void removeAt(int aIndex);
    Q_INVOKABLE void removeFiles(QList<int>);
    Q_INVOKABLE void setTitleAt(int, QString);
    Q_INVOKABLE void setGroupId(QString, QString);
    Q_INVOKABLE void setGroupIdAt(int, QString);
    Q_INVOKABLE void setGroupIdForRows(QList<int>, QString);
    Q_INVOKABLE int groupIndexAt(int) const;
    Q_INVOKABLE QVariantMap get(int) const;

    // Keys for metadata passed to encryptFile:
    static const QString MetaUrl;               // "url" -> QUrl
    static const QString MetaOrientation;       // "orientation" -> int
    static const QString MetaImageDate;         // "imageDate" -> QDateTime
    static const QString MetaCameraManufacturer;// "cameraManufacturer" -> QString
    static const QString MetaCameraModel;       // "cameraModel" -> QString
    static const QString MetaLatitude;          // "latitude" -> double
    static const QString MetaLongitude;         // "longitude" -> double
    static const QString MetaAltitude;          // "altitude" -> double

private Q_SLOTS:
    void imageRequest(QString, FoilPicsImageRequest);

Q_SIGNALS:
    void countChanged();
    void busyChanged();
    void keyAvailableChanged();
    void foilStateChanged();
    void mayHaveEncryptedPicturesChanged();
    void thumbnailSizeChanged();
    void keyGenerated();
    void passwordChanged();
    void decryptionStarted();
    void mediaDeleted(QUrl url);

private:
    Private* iPrivate;
};

QML_DECLARE_TYPE(FoilPicsModel)

#endif // FOILPICS_MODEL_H
