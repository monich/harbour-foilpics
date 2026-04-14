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

#ifndef FOILPICS_GROUP_MODEL_H
#define FOILPICS_GROUP_MODEL_H

#include <QtCore/QAbstractListModel>
#include <QtQml>

class FoilPicsModel;

class FoilPicsGroupModel :
    public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    typedef QPair<QByteArray,QByteArray> GroupInfo; // <groups,collapsed>

    class Group {
    public:
        QByteArray iId;
        QString iName;
        bool iExpanded;
    public:
        class Private;
        Group();
        Group(QByteArray, QString);
        Group(const Group&);
        Group& operator = (const Group&);
        bool operator == (const Group&) const;
        bool operator != (const Group&) const;
        bool equals(const Group&) const;
        bool isDefault() const;
        static QList<Group> decodeList(const char*, const char*);
        static GroupInfo encodeList(QList<Group>);
    };

    typedef QList<Group> GroupList;

    FoilPicsGroupModel(FoilPicsModel*);

    GroupList groups() const;
    void setGroups(GroupList);
    bool isKnownGroup(QByteArray) const;
    int indexOfGroup(QByteArray) const;
    void clear();

    // QAbstractListModel
    Qt::ItemFlags flags(const QModelIndex&) const Q_DECL_OVERRIDE;
    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& aParent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex&, int) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex&, const QVariant&, int) Q_DECL_OVERRIDE;

    Q_INVOKABLE QString groupName(QString) const;
    Q_INVOKABLE QString groupNameAt(int) const;
    Q_INVOKABLE QString groupId(int) const;
    Q_INVOKABLE bool defaultGroupAt(int) const;
    Q_INVOKABLE void clearGroupAt(int);
    Q_INVOKABLE int groupPicsCountAt(int) const;
    Q_INVOKABLE int offsetWithinGroup(int, int) const;

    Q_INVOKABLE void addGroup(QString);
    Q_INVOKABLE void removeGroupAt(int);
    Q_INVOKABLE void moveGroup(int, int);

    Q_INVOKABLE void setDragIndex(int);
    Q_INVOKABLE void setDragPos(int);

Q_SIGNALS:
    void countChanged();
    void rowsActuallyMoved();
    void groupRenamed(int index, const FoilPicsGroupModel::Group& group);

private:
    class ProxyModel;
    class ModelData;
    class Private;
    Private* iPrivate;
};

QML_DECLARE_TYPE(FoilPicsGroupModel)
Q_DECLARE_METATYPE(FoilPicsGroupModel::Group)
Q_DECLARE_METATYPE(FoilPicsGroupModel::GroupList)

#endif // FOILPICS_GROUP_MODEL_H
