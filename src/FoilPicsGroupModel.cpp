/*
 * Copyright (C) 2017-2018 Jolla Ltd.
 * Copyright (C) 2017-2018 Slava Monich <slava@monich.com>
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

#include "FoilPicsGroupModel.h"
#include "FoilPicsModel.h"

#include "foil_random.h"

#include "HarbourDebug.h"

#define GROUP_ID_SEPARATOR ':'
#define GROUP_LIST_SEPARATOR ','

// ==========================================================================
// FoilPicsGroupModel::Group::Private
// ==========================================================================

class FoilPicsGroupModel::Group::Private {
public:
    static void encode(QByteArray* aDest, QByteArray aSrc);
    static const char* decode(const char* aStart, char aStop, QByteArray* aBuf);
};

void FoilPicsGroupModel::Group::Private::encode(QByteArray* aDest, QByteArray aSrc)
{
    for (const char* ptr = aSrc.constData(); *ptr; ptr++) {
        switch (*ptr) {
        case GROUP_ID_SEPARATOR:
        case GROUP_LIST_SEPARATOR:
        case '\\':
            aDest->append('\\');
            // no break
        default:
            aDest->append(*ptr);
            break;
        }
    }
}

const char* FoilPicsGroupModel::Group::Private::decode(const char* aStart,
    char aStop, QByteArray* aBuf)
{
    const char* ptr = aStart;
    aBuf->clear();
    while (*ptr && *ptr != aStop) {
        if (*ptr == '\\') {
            if (!*++ptr) break;
        }
        aBuf->append(*ptr++);
    }
    return ptr;
}

// ==========================================================================
// FoilPicsGroupModel::Group
// ==========================================================================

FoilPicsGroupModel::Group::Group(QByteArray aId, QString aName) :
    iId(aId), iName(aName)
{
}

FoilPicsGroupModel::Group::Group(const Group& aGroup) :
    iId(aGroup.iId), iName(aGroup.iName)
{
}

FoilPicsGroupModel::Group& FoilPicsGroupModel::Group::operator=(
    const Group& aGroup)
{
    iId = aGroup.iId;
    iName = aGroup.iName;
    return *this;
}

bool FoilPicsGroupModel::Group::operator==(const Group& aGroup) const
{
    return equals(aGroup);
}

bool FoilPicsGroupModel::Group::operator!=(const Group& aGroup) const
{
    return !equals(aGroup);
}

bool FoilPicsGroupModel::Group::equals(const Group& aGroup) const
{
    return iId == aGroup.iId && iName == aGroup.iName;
}

bool FoilPicsGroupModel::Group::isDefault() const
{
    return iId.isEmpty();
}

// Serialization format: "id1:name1,id2:name2"

FoilPicsGroupModel::GroupList
FoilPicsGroupModel::Group::decodeList(const char* aString)
{
    QByteArray buf;
    GroupList groups;
    const char* ptr = aString;
    while (*ptr) {
        ptr = Private::decode(ptr, GROUP_ID_SEPARATOR, &buf);
        if (*ptr == GROUP_ID_SEPARATOR) {
            QByteArray id(buf);
            ptr = Private::decode(++ptr, GROUP_LIST_SEPARATOR, &buf);
            groups.append(Group(id, QString::fromUtf8(buf)));
            if (*ptr == GROUP_LIST_SEPARATOR) ptr++;
        }
    }
    return groups;
}

QByteArray FoilPicsGroupModel::Group::encodeList(QList<Group> aList)
{
    QByteArray buf;
    const int n = aList.count();
    for (int i = 0; i < n; i++) {
        const Group& group = aList.at(i);
        if (!buf.isEmpty()) buf.append(GROUP_LIST_SEPARATOR);
        Private::encode(&buf, group.iId);
        buf.append(GROUP_ID_SEPARATOR);
        Private::encode(&buf, group.iName.toUtf8());
    }
    return buf;
}

// ==========================================================================
// FoilPicsGroupModel::ProxyModel
// ==========================================================================

class FoilPicsGroupModel::ProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    ProxyModel(FoilPicsModel* aPicsModel, QString aId, QObject* aParent);
    bool filterAcceptsRow(int aRow, const QModelIndex& aParent) const;

    FoilPicsModel* sourcePicsModel() const;
    Q_INVOKABLE int mapToSource(int aIndex) const;
    Q_INVOKABLE void setTitleAt(int aIndex, QString aTitle);
    Q_INVOKABLE void setGroupIdAt(int aIndex, QString aId);
    Q_INVOKABLE QVariantMap get(int aIndex) const;

Q_SIGNALS:
    void countChanged();

public Q_SLOTS:
    void onModelReset();

public:
    int iLastKnownCount;
};

FoilPicsGroupModel::ProxyModel::ProxyModel(FoilPicsModel* aPicsModel,
    QString aId, QObject* aParent) : QSortFilterProxyModel(aParent)
{
    setSourceModel(aPicsModel);
    setFilterRole(FoilPicsModel::groupIdRole());
    setFilterFixedString(aId);
    setDynamicSortFilter(true);
    iLastKnownCount = rowCount();
    connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)), SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)), SIGNAL(countChanged()));
    connect(this, SIGNAL(modelReset()), SLOT(onModelReset()));
}

bool FoilPicsGroupModel::ProxyModel::filterAcceptsRow(int aSourceRow,
    const QModelIndex& aParent) const
{
    QRegExp re(filterRegExp());
    if (re.isEmpty()) {
        // QSortFilterProxyModel cannot select empty strings.
        // If the filter is an empty string, it just accepts
        // everything - and that's not what we want.
        QAbstractItemModel* model = sourceModel();
        QModelIndex index = model->index(aSourceRow, 0, aParent);
        QString value = model->data(index, filterRole()).toString();
        return value.isEmpty();
    } else {
        return QSortFilterProxyModel::filterAcceptsRow(aSourceRow, aParent);
    }
}

inline FoilPicsModel* FoilPicsGroupModel::ProxyModel::sourcePicsModel() const
{
    return qobject_cast<FoilPicsModel*>(sourceModel());
}

int FoilPicsGroupModel::ProxyModel::mapToSource(int aIndex) const
{
    QModelIndex source(QSortFilterProxyModel::mapToSource(index(aIndex, 0)));
    return source.isValid() ? source.row() : -1;
}

void FoilPicsGroupModel::ProxyModel::setTitleAt(int aIndex, QString aTitle)
{
    sourcePicsModel()->setTitleAt(mapToSource(aIndex), aTitle);
}

void FoilPicsGroupModel::ProxyModel::setGroupIdAt(int aIndex, QString aId)
{
    sourcePicsModel()->setGroupIdAt(mapToSource(aIndex), aId);
}

QVariantMap FoilPicsGroupModel::ProxyModel::get(int aIndex) const
{
    return sourcePicsModel()->get(mapToSource(aIndex));
}

void FoilPicsGroupModel::ProxyModel::onModelReset()
{
    const int count = rowCount();
    if (iLastKnownCount != count) {
        iLastKnownCount = count;
        Q_EMIT countChanged();
    }
}

// ==========================================================================
// FoilPicsGroupModel::ModelData
// ==========================================================================

class FoilPicsGroupModel::ModelData : public QObject {
    Q_OBJECT

public:
    enum Role {
        GroupIdRole = Qt::UserRole,
        GroupNameRole,
        GroupPicsModelRole,
        GroupPicsCountRole,
        FirstGroupRole,
        DefaultGroupRole
    };

    ModelData(FoilPicsModel* aPicsModel);
    ModelData(FoilPicsModel* aPicsModel, const Group& aGroup);
    ModelData(FoilPicsModel* aPicsModel, QByteArray aId, QString aName);
    void init();

    QVariant get(Role aRole) const;
    ProxyModel* createProxyModel() const;
    QAbstractProxyModel* proxyModel() const;
    bool isFirstGroup() const;
    bool firstGroupMayHaveChanged() const;

Q_SIGNALS:
    void proxyModelDestroyed();
    void proxyModelCountChanged();

public Q_SLOTS:
    void onProxyModelDestroyed(QObject* aModel);

public:
    Group iGroup;
    FoilPicsModel* iPicsModel;
    mutable ProxyModel* iProxyModel;
    mutable bool iLastKnownFirstGroup;
};

FoilPicsGroupModel::ModelData::ModelData(FoilPicsModel* aPicsModel) :
    QObject(aPicsModel), iPicsModel(aPicsModel)
{
    init();
}

FoilPicsGroupModel::ModelData::ModelData(FoilPicsModel* aPicsModel,
    const Group& aGroup) : QObject(aPicsModel),
    iGroup(aGroup), iPicsModel(aPicsModel)
{
    init();
}

FoilPicsGroupModel::ModelData::ModelData(FoilPicsModel* aPicsModel,
    QByteArray aId, QString aName) : QObject(aPicsModel),
    iGroup(aId, aName), iPicsModel(aPicsModel)
{
    init();
}

void FoilPicsGroupModel::ModelData::init()
{
    iProxyModel = NULL;
    iLastKnownFirstGroup = false;
}

QAbstractProxyModel* FoilPicsGroupModel::ModelData::proxyModel() const
{
    if (!iProxyModel) {
        iProxyModel = createProxyModel();
    }
    return iProxyModel;
}

FoilPicsGroupModel::ProxyModel* FoilPicsGroupModel::ModelData::createProxyModel() const
{
    // Using parent() here to avoid cast, because this is const
    ProxyModel* model = new ProxyModel(iPicsModel, iGroup.iId, parent());
    connect(model, SIGNAL(destroyed(QObject*)), SLOT(onProxyModelDestroyed(QObject*)));
    connect(model, SIGNAL(countChanged()), SIGNAL(proxyModelCountChanged()));
    HDEBUG(model->rowCount() << iGroup.iId.data());
    return model;
}

bool FoilPicsGroupModel::ModelData::isFirstGroup() const
{
    QAbstractProxyModel* model = proxyModel();
    return model->rowCount() && !model->mapToSource(model->index(0, 0)).row();
}

bool FoilPicsGroupModel::ModelData::firstGroupMayHaveChanged() const
{
    return iLastKnownFirstGroup != isFirstGroup();
}

QVariant FoilPicsGroupModel::ModelData::get(Role aRole) const
{
    switch (aRole) {
    case GroupIdRole: return QString::fromLatin1(iGroup.iId);
    case GroupNameRole: return iGroup.iName;
    case GroupPicsModelRole: return QVariant::fromValue(proxyModel());
    case GroupPicsCountRole: return proxyModel()->rowCount();
    case FirstGroupRole: return (iLastKnownFirstGroup = isFirstGroup());
    case DefaultGroupRole: return iGroup.isDefault();
    // No default to make sure that we get "warning: enumeration value
    // not handled in switch" if we forget to handle a role.
    }
    return QVariant();
}

void FoilPicsGroupModel::ModelData::onProxyModelDestroyed(QObject* aModel)
{
    iProxyModel = NULL;
    Q_EMIT proxyModelDestroyed();
}

// ==========================================================================
// FoilPicsGroupModel::Private
// ==========================================================================

class FoilPicsGroupModel::Private : public QObject {
    Q_OBJECT

public:
    Private(FoilPicsGroupModel* aParent, FoilPicsModel* aPicsModel);
    ~Private();

    FoilPicsGroupModel* parentModel() const;
    ModelData* dataAt(int aIndex) const;
    ModelData* createDefaultData() const;
    ModelData* createData(const Group& aGroup) const;
    ModelData* createData(QByteArray aId, QString aName) const;
    ModelData* connectData(ModelData* aData) const;

    static QByteArray generateRandomId();
    int findId(QByteArray aId) const;
    void appendDefaultGroup();
    void clear();
    QByteArray generateUniqueId() const;
    GroupList groups() const;
    void setGroups(GroupList aGroups);
    bool equalGroups(GroupList aGroups);
    void dataChanged(int aRow, QVector<int> aRoles);
    void dataChanged(int aRow, ModelData::Role aRole);

public Q_SLOTS:
    void onProxyModelDestroyed();
    void onProxyModelCountChanged();
    void onParentModelReset();
    void onPicsModelReset();

public:
    FoilPicsModel* iPicsModel;
    QList<ModelData*> iData;
    QHash<QByteArray,int> iMap;
    int iLastKnownCount;
};

FoilPicsGroupModel::Private::Private(FoilPicsGroupModel* aParent,
    FoilPicsModel* aPicsModel) : QObject(aParent), iPicsModel(aPicsModel),
    iLastKnownCount(0)
{
    appendDefaultGroup();
    connect(aParent, SIGNAL(modelReset()), SLOT(onParentModelReset()));
    connect(aPicsModel, SIGNAL(modelReset()), SLOT(onPicsModelReset()));
}

FoilPicsGroupModel::Private::~Private()
{
    qDeleteAll(iData);
}

inline FoilPicsGroupModel* FoilPicsGroupModel::Private::parentModel() const
{
    return qobject_cast<FoilPicsGroupModel*>(parent());
}

FoilPicsGroupModel::ModelData* FoilPicsGroupModel::Private::dataAt(int aIndex) const
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        return iData.at(aIndex);
    } else {
        return NULL;
    }
}

inline FoilPicsGroupModel::ModelData*
FoilPicsGroupModel::Private::createDefaultData() const
{
    return connectData(new ModelData(iPicsModel));
}

inline FoilPicsGroupModel::ModelData*
FoilPicsGroupModel::Private::createData(const Group& aGroup) const
{
    return connectData(new ModelData(iPicsModel, aGroup));
}

inline FoilPicsGroupModel::ModelData*
FoilPicsGroupModel::Private::createData(QByteArray aId, QString aName) const
{
    return connectData(new ModelData(iPicsModel, aId, aName));
}

inline FoilPicsGroupModel::ModelData*
FoilPicsGroupModel::Private::connectData(ModelData* aData) const
{
    connect(aData,
        SIGNAL(proxyModelDestroyed()),
        SLOT(onProxyModelDestroyed()));
    connect(aData,
        SIGNAL(proxyModelCountChanged()),
        SLOT(onProxyModelCountChanged()));
    return aData;
}

QList<FoilPicsGroupModel::Group> FoilPicsGroupModel::Private::groups() const
{
    GroupList list;
    const int n = iData.count();
    for (int i = 0; i < n; i++) {
        list.append(iData.at(i)->iGroup);
    }
    return list;
}

bool FoilPicsGroupModel::Private::equalGroups(GroupList aGroups)
{
    const int n = iData.count();
    if (aGroups.count() == n) {
        for (int i = 0; i < n; i++) {
            if (!iData.at(i)->iGroup.equals(aGroups.at(i))) {
                return false;
            }
        }
        return true;
    }
    return false;
}

void FoilPicsGroupModel::Private::setGroups(GroupList aGroups)
{
    if (!equalGroups(aGroups)) {
        FoilPicsGroupModel* model = parentModel();
        model->beginResetModel();
        qDeleteAll(iData);
        iData.clear();
        iMap.clear();
        ModelData* defaultData = NULL;
        const int n = aGroups.count();
        for (int i = 0; i < n; i++) {
            const Group& group = aGroups.at(i);
            bool isDefault = group.isDefault();
            if (!defaultData || !isDefault) {
                ModelData* data = createData(group);
                if (isDefault) {
                    defaultData = data;
                }
                if (!iMap.contains(group.iId)) {
                    iMap.insert(group.iId, iData.size());
                    iData.append(data);
                }
            }
        }
        if (!defaultData) {
            appendDefaultGroup();
        }
        model->endResetModel();
    }
}

void FoilPicsGroupModel::Private::appendDefaultGroup()
{
    ModelData* defaultItem = createDefaultData();
    iMap.insert(defaultItem->iGroup.iId, 0);
    iData.insert(0, defaultItem);
    // Fix damaged indeces (if any)
    const int k = iData.count();
    for (int i = 1; i < k; i++) {
        iMap.insert(iData.at(i)->iGroup.iId, i);
    }
}

void FoilPicsGroupModel::Private::clear()
{
    ModelData* defaultData = NULL;
    for (int i = iData.count() - 1; i >= 0; i--) {
        ModelData* data = iData.at(i);
        if (!defaultData && data->iGroup.isDefault()) {
            defaultData = data;
        } else {
            iData.removeAt(i);
            iMap.remove(data->iGroup.iId);
            delete data;
        }
    }
    if (!defaultData) {
        appendDefaultGroup();
    }
}

QByteArray FoilPicsGroupModel::Private::generateRandomId()
{
    guint8 data[8];
    char buf[2*sizeof(data)+1];
    foil_random_generate(FOIL_RANDOM_DEFAULT, data, sizeof(data));
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X",
        data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
    return QByteArray(buf);
}

int FoilPicsGroupModel::Private::findId(QByteArray aId) const
{
    int i = iMap.value(aId);
    if (i > 0) {
        // It's definitely there
        return i;
    } else {
        // Zero is also returned if key is not there
        return iMap.contains(aId) ? 0 : -1;
    }
}

QByteArray FoilPicsGroupModel::Private::generateUniqueId() const
{
    QByteArray id(generateRandomId());
    while (findId(id) >= 0) {
        id = generateRandomId();
    }
    return id;
}

void FoilPicsGroupModel::Private::dataChanged(int aRow, ModelData::Role aRole)
{
    QVector<int> roles;
    roles.append(aRole);
    dataChanged(aRow, roles);
}

void FoilPicsGroupModel::Private::dataChanged(int aRow, QVector<int> aRoles)
{
    FoilPicsGroupModel* model = parentModel();
    QModelIndex modelIndex(model->index(aRow));
    Q_EMIT model->dataChanged(modelIndex, modelIndex, aRoles);
}

void FoilPicsGroupModel::Private::onProxyModelDestroyed()
{
    ModelData* data = qobject_cast<ModelData*>(sender());
    const int row = iData.indexOf(data);
    HDEBUG(row);
    if (row >= 0) {
        QVector<int> roles;
        roles.append(ModelData::GroupPicsModelRole);
        roles.append(ModelData::GroupPicsCountRole);
        dataChanged(row, roles);
    }
}

void FoilPicsGroupModel::Private::onProxyModelCountChanged()
{
    ModelData* data = qobject_cast<ModelData*>(sender());
    const int row = iData.indexOf(data);
    HDEBUG(row << data->proxyModel()->rowCount());
    if (row >= 0) {
        dataChanged(row, ModelData::GroupPicsCountRole);
    }
}

void FoilPicsGroupModel::Private::onParentModelReset()
{
    FoilPicsGroupModel* model = parentModel();
    const int count = model->rowCount();
    if (iLastKnownCount != count) {
        iLastKnownCount = count;
        Q_EMIT model->countChanged();
    }
}

void FoilPicsGroupModel::Private::onPicsModelReset()
{
    for (int i = 0; i < iData.count(); i++) {
        ModelData* data = iData.at(i);
        if (data->firstGroupMayHaveChanged()) {
            dataChanged(i, ModelData::FirstGroupRole);
        }
    }
}

// ==========================================================================
// FoilPicsGroupModel
// ==========================================================================

FoilPicsGroupModel::FoilPicsGroupModel(FoilPicsModel* aParent) :
    QAbstractListModel(aParent),
    iPrivate(new Private(this, aParent))
{
    qRegisterMetaType<GroupList>("FoilPicsGroupModel::GroupList");
    connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)), SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)), SIGNAL(countChanged()));
}

QHash<int,QByteArray> FoilPicsGroupModel::roleNames() const
{
    QHash<int,QByteArray> roles;
    roles.insert(ModelData::GroupIdRole, "groupId");
    roles.insert(ModelData::GroupNameRole, "groupName");
    roles.insert(ModelData::GroupPicsModelRole, "groupPicsModel");
    roles.insert(ModelData::GroupPicsCountRole, "groupPicsCount");
    roles.insert(ModelData::FirstGroupRole, "firstGroup");
    roles.insert(ModelData::DefaultGroupRole, "defaultGroup");
    return roles;
}

int FoilPicsGroupModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->iData.count();
}

QVariant FoilPicsGroupModel::data(const QModelIndex& aIndex, int aRole) const
{
    ModelData* data = iPrivate->dataAt(aIndex.row());
    return data ? data->get((ModelData::Role)aRole) : QVariant();
}

QList<FoilPicsGroupModel::Group> FoilPicsGroupModel::groups() const
{
    GroupList list;
    const int n = iPrivate->iData.count();
    for (int i = 0; i < n; i++) {
        list.append(iPrivate->iData.at(i)->iGroup);
    }
    return list;
}

void FoilPicsGroupModel::setGroups(GroupList aGroups)
{
    iPrivate->setGroups(aGroups);
}

bool FoilPicsGroupModel::isKnownGroup(QByteArray aId) const
{
    return iPrivate->iMap.contains(aId);
}

int FoilPicsGroupModel::indexOfGroup(QByteArray aId) const
{
    return iPrivate->findId(aId);
}

void FoilPicsGroupModel::clear()
{
    // There's always at least one group (the default one)
    if (iPrivate->iData.count() > 1) {
        beginResetModel();
        iPrivate->clear();
        endResetModel();
    }
}

QString FoilPicsGroupModel::groupName(QString aId) const
{
    return groupNameAt(iPrivate->findId(aId.toLatin1()));
}

QString FoilPicsGroupModel::groupNameAt(int aIndex) const
{
    ModelData* data = iPrivate->dataAt(aIndex);
    return data ? data->iGroup.iName : QString();
}

QString FoilPicsGroupModel::groupId(int aIndex) const
{
    ModelData* data = iPrivate->dataAt(aIndex);
    return data ? QString::fromLatin1(data->iGroup.iId) : QString();
}

bool FoilPicsGroupModel::defaultGroupAt(int aIndex) const
{
    ModelData* data = iPrivate->dataAt(aIndex);
    return !data || data->iGroup.isDefault();
}

int FoilPicsGroupModel::groupPicsCountAt(int aIndex) const
{
    ModelData* data = iPrivate->dataAt(aIndex);
    return data ? data->proxyModel()->rowCount() : 0;
}

void FoilPicsGroupModel::clearGroupAt(int aIndex)
{
    ModelData* data = iPrivate->dataAt(aIndex);
    if (data && !data->iGroup.isDefault()) {
        iPrivate->iPicsModel->clearGroup(data->iGroup.iId);
    }
}

int FoilPicsGroupModel::offsetWithinGroup(int aIndex, int aSource) const
{
    ModelData* data = iPrivate->dataAt(aIndex);
    if (data) {
        QAbstractProxyModel* model = data->proxyModel();
        const int n = model->rowCount();
        const int first = model->mapToSource(model->index(0, 0)).row();
        if (first >= 0 && aSource >= first && aSource < (first + n)) {
            const int off = aSource - first;
            HASSERT(model->mapToSource(model->index(off, 0)).row() == aSource);
            return off;
        }
    }
    return -1;
}

void FoilPicsGroupModel::addGroup(QString aName)
{
    const int pos = iPrivate->iData.count();
    beginInsertRows(QModelIndex(), pos, pos);
    QByteArray id(iPrivate->generateUniqueId());
    ModelData* data = iPrivate->createData(id, aName);
    iPrivate->iData.insert(pos, data);
    iPrivate->iMap.insert(id, pos);
    endInsertRows();
}

void FoilPicsGroupModel::renameGroupAt(int aIndex, QString aName)
{
    ModelData* data = iPrivate->dataAt(aIndex);
    if (data && data->iGroup.iName != aName) {
        data->iGroup.iName = aName;
        QVector<int> roles;
        roles.append(ModelData::GroupNameRole);
        QModelIndex modelIndex(index(aIndex));
        Q_EMIT dataChanged(modelIndex, modelIndex, roles);
    }
}

void FoilPicsGroupModel::removeGroupAt(int aIndex)
{
    // UI makes sure that the group is empty before it's removed
    ModelData* data = iPrivate->dataAt(aIndex);
    if (data && !data->iGroup.isDefault()) {
        beginRemoveRows(QModelIndex(), aIndex, aIndex);
        iPrivate->iData.removeAt(aIndex);
        iPrivate->iMap.remove(data->iGroup.iId);
        delete data;
        endRemoveRows();
    }
}

void FoilPicsGroupModel::moveGroup(int aFrom, int aTo)
{
    const int n = iPrivate->iData.count();
    if (aFrom != aTo && aFrom >= 0 && aFrom < n && aTo >= 0 && aTo < n) {
        QModelIndex parent;
        const int dest = (aTo > aFrom) ? (aTo + 1) : aTo;
        if (beginMoveRows(parent, aFrom, aFrom, parent, dest)) {
            iPrivate->iData.move(aFrom, aTo);
            // Update damaged map entries
            const int i1 = qMin(aFrom, aTo);
            const int i2 = qMax(aFrom, aTo);
            for (int i = i1; i <= i2; i++) {
                iPrivate->iMap.insert(iPrivate->iData.at(i)->iGroup.iId, i);
            }
            endMoveRows();

            // "firstGroup" may have changed
            QVector<int> roles;
            roles.append(ModelData::FirstGroupRole);
            for (int i = i1; i <= i2; i++) {
                QModelIndex modelIndex(index(i));
                Q_EMIT dataChanged(modelIndex, modelIndex, roles);
            }
        } else {
            HDEBUG("oops, can't move" << aFrom << "->" << aTo);
        }
    }
}

#include "FoilPicsGroupModel.moc"
