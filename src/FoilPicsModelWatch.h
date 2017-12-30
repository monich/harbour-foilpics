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

#ifndef FOILPICS_MODEL_WATCH_H
#define FOILPICS_MODEL_WATCH_H

#include <QtQml>
#include <QAbstractItemModel>

class FoilPicsModelWatch : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString keyRole READ keyRole WRITE setKeyRole NOTIFY keyRoleChanged)
    Q_PROPERTY(QString keyValue READ keyValue WRITE setKeyValue NOTIFY keyValueChanged)
    Q_PROPERTY(QString role READ watchRole WRITE setWatchRole NOTIFY watchRoleChanged)
    Q_PROPERTY(QVariant value READ watchValue NOTIFY watchValueChanged)
    Q_PROPERTY(int index READ index NOTIFY indexChanged)

public:
    explicit FoilPicsModelWatch(QObject* aParent = NULL);

    QAbstractItemModel* model() const;
    void setModel(QAbstractItemModel* aModel);

    QString keyRole() const;
    void setKeyRole(QString aRole);

    QString keyValue() const;
    void setKeyValue(QString aValue);

    QString watchRole() const;
    void setWatchRole(QString aRole);

    QVariant watchValue() const;
    int index() const;

Q_SIGNALS:
    void modelChanged();
    void keyRoleChanged();
    void keyValueChanged();
    void watchRoleChanged();
    void watchValueChanged();
    void indexChanged();

private:
    class Private;
    Private* iPrivate;
};

QML_DECLARE_TYPE(FoilPicsModelWatch)

#endif // FOILPICS_MODEL_WATCH_H
