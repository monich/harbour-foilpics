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

#ifndef FOILPICS_GROUP_MODEL_H
#define FOILPICS_GROUP_MODEL_H

#include <QtQml>
#include <QAbstractListModel>

class FoilPicsModel;

class FoilPicsGroupModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    class Group {
    public:
        QByteArray iId;
        QString iName;
    public:
        class Private;
        Group() {}
        Group(QByteArray aId, QString aName);
        Group(const Group& aGroup);
        Group& operator = (const Group& aGroup);
        bool operator == (const Group& aGroup) const;
        bool operator != (const Group& aGroup) const;
        bool equals(const Group& aGroup) const;
        bool isDefault() const;
        static QList<Group> decodeList(const char* aString);
        static QByteArray encodeList(QList<Group> aList);
    };

    typedef QList<Group> GroupList;

    FoilPicsGroupModel(FoilPicsModel* aParent);

    GroupList groups() const;
    void setGroups(GroupList aGroups);
    bool isKnownGroup(QByteArray aId) const;
    int indexOfGroup(QByteArray aId) const;
    void clear();

    // QAbstractListModel
    virtual QHash<int,QByteArray> roleNames() const;
    virtual int rowCount(const QModelIndex& aParent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& aIndex, int aRole) const;

    Q_INVOKABLE QString groupName(QString aId) const;
    Q_INVOKABLE QString groupNameAt(int aIndex) const;
    Q_INVOKABLE QString groupId(int aIndex) const;
    Q_INVOKABLE bool defaultGroupAt(int aIndex) const;
    Q_INVOKABLE void clearGroupAt(int aIndex);
    Q_INVOKABLE int groupPicsCountAt(int aIndex) const;
    Q_INVOKABLE int offsetWithinGroup(int aGroupIndex, int aSourceIndex) const;

    Q_INVOKABLE void addGroup(QString aName);
    Q_INVOKABLE void renameGroupAt(int aIndex, QString aName);
    Q_INVOKABLE void removeGroupAt(int aIndex);
    Q_INVOKABLE void moveGroup(int aFrom, int aTo);

Q_SIGNALS:
    void countChanged();

private:
    class ProxyModel;
    class ModelData;
    class Private;
    Private* iPrivate;
};

QML_DECLARE_TYPE(FoilPicsGroupModel)
Q_DECLARE_METATYPE(FoilPicsGroupModel::GroupList)

#endif // FOILPICS_GROUP_MODEL_H
