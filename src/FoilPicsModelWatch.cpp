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

#include "FoilPicsModelWatch.h"
#include "FoilPicsRole.h"
#include "HarbourDebug.h"

// ==========================================================================
// FoilPicsModelWatch::Private
// ==========================================================================

class FoilPicsModelWatch::Private : public QObject {
    Q_OBJECT

public:
    Private(FoilPicsModelWatch* aParent);

    FoilPicsModelWatch* watcher() const;
    bool isUsable() const;
    void setModel(QAbstractItemModel* aModel);
    void setKeyRole(QString aRole);
    void setKeyValue(QString aValue);
    void setWatchRole(QString aRole);
    void updateKeyRole();
    void updateWatchRole();
    void updateWatchValue();
    void reset();
    void searchModel();
    void searchModel(int aFirstRow, int aLastRow);

public Q_SLOTS:
    void onModelDestroyed(QObject* aModel);
    void onModelReset();
    void onModelRowsInserted(const QModelIndex& aParent, int aStart, int aEnd);
    void onModelRowsRemoved(const QModelIndex& aParent, int aStart, int aEnd);
    void onModelRowsMoved(const QModelIndex& aSourceParent, int aSourceStart,
        int aSourceEnd, const QModelIndex& aDestParent, int aDest);
    void onModelDataChanged(const QModelIndex& aTopLeft,
        const QModelIndex& aBottomRight, const QVector<int>& aRoles);

public:
    QAbstractItemModel* iModel;
    QString iKeyRoleName;
    QString iKeyValue;
    int iKeyRole;
    int iColumn;
    QString iWatchRoleName;
    int iWatchRole;
    QVariant iWatchValue;
    int iIndex;
};

FoilPicsModelWatch::Private::Private(FoilPicsModelWatch* aParent) :
    QObject(aParent),
    iModel(NULL),
    iKeyRole(-1),
    iColumn(0),
    iWatchRole(-1),
    iIndex(-1)
{
}

inline FoilPicsModelWatch* FoilPicsModelWatch::Private::watcher() const
{
    return qobject_cast<FoilPicsModelWatch*>(parent());
}

bool FoilPicsModelWatch::Private::isUsable() const
{
    return iKeyRole >= 0 && !iKeyRoleName.isEmpty() && !iKeyValue.isEmpty() &&
        iModel && iModel->rowCount() > 0;
}

void FoilPicsModelWatch::Private::setModel(QAbstractItemModel* aModel)
{
    if (iModel != aModel) {
        if (iModel) {
            iModel->disconnect(this);
        }
        iModel = aModel;
        if (iModel) {
            connect(iModel,
                SIGNAL(destroyed(QObject*)),
                SLOT(onModelDestroyed(QObject*)));
            connect(iModel,
                SIGNAL(modelReset()),
                SLOT(onModelReset()));
            connect(iModel,
                SIGNAL(rowsInserted(QModelIndex,int,int)),
                SLOT(onModelRowsInserted(QModelIndex,int,int)));
            connect(iModel,
                SIGNAL(rowsRemoved(QModelIndex,int,int)),
                SLOT(onModelRowsRemoved(QModelIndex,int,int)));
            connect(iModel,
                SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
                SLOT(onModelRowsMoved(QModelIndex,int,int,QModelIndex,int)));
            connect(iModel,
                SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),
                SLOT(onModelDataChanged(QModelIndex,QModelIndex,QVector<int>)));
        }
        Q_EMIT watcher()->modelChanged();
        updateKeyRole();
        updateWatchRole();
    }
}

void FoilPicsModelWatch::Private::updateKeyRole()
{
    const int role = FoilPicsRole::find(iModel, iKeyRoleName);
    if (role >= 0) {
        if (iKeyRole != role) {
            iKeyRole = role;
            HDEBUG(iKeyRoleName << role);
            searchModel();
        }
    } else {
        reset();
    }
}

void FoilPicsModelWatch::Private::updateWatchRole()
{
    const int role = FoilPicsRole::find(iModel, iWatchRoleName);
    if (role >= 0) {
        if (iWatchRole != role) {
            iWatchRole = role;
            HDEBUG(iWatchRoleName << role);
            updateWatchValue();
        }
    } else {
        if (iWatchValue.isValid()) {
            iWatchValue = QVariant();
            Q_EMIT watcher()->watchValueChanged();
        }
    }
}

void FoilPicsModelWatch::Private::updateWatchValue()
{
    QVariant value;
    if (iWatchRole >= 0 && iIndex >= 0) {
        QModelIndex index = iModel->index(iIndex, iColumn);
        value = iModel->data(index, iWatchRole);
    }
    if (iWatchValue != value) {
        iWatchValue = value;
        HDEBUG(iWatchRoleName << "=" << iWatchValue);
        Q_EMIT watcher()->watchValueChanged();
    }
}

void FoilPicsModelWatch::Private::setKeyRole(QString aRole)
{
    if (iKeyRoleName != aRole) {
        iKeyRoleName = aRole;
        Q_EMIT watcher()->keyRoleChanged();
        updateKeyRole();
    }
}

void FoilPicsModelWatch::Private::setKeyValue(QString aValue)
{
    if (iKeyValue != aValue) {
        iKeyValue = aValue;
        Q_EMIT watcher()->keyValueChanged();
        searchModel();
    }
}

void FoilPicsModelWatch::Private::setWatchRole(QString aRole)
{
    if (iWatchRoleName != aRole) {
        iWatchRoleName = aRole;
        Q_EMIT watcher()->watchRoleChanged();
        updateWatchRole();
    }
}

void FoilPicsModelWatch::Private::reset()
{
    if (iIndex >= 0) {
        iIndex = -1;
        Q_EMIT watcher()->indexChanged();
    }
    if (iWatchValue.isValid()) {
        iWatchValue = QVariant();
        Q_EMIT watcher()->watchValueChanged();
    }
}

void FoilPicsModelWatch::Private::searchModel()
{
    if (isUsable()) {
        searchModel(0, iModel->rowCount() - 1);
    } else {
        reset();
    }
}

void FoilPicsModelWatch::Private::searchModel(int aFirstRow, int aLastRow)
{
    if (isUsable()) {
        for (int row = aFirstRow; row <= aLastRow; row++) {
            QModelIndex index = iModel->index(row, iColumn);
            if (iKeyValue == iModel->data(index, iKeyRole).toString()) {
                if (iIndex != row) {
                    iIndex = row;
                    HDEBUG(iKeyValue << "at" << row);
                    Q_EMIT watcher()->indexChanged();
                }
                updateWatchValue();
                return;
            }
        }
    }
    reset();
}

void FoilPicsModelWatch::Private::onModelDestroyed(QObject* aModel)
{
    HDEBUG("");
    iModel = NULL;
    reset();
    Q_EMIT watcher()->modelChanged();
}

void FoilPicsModelWatch::Private::onModelReset()
{
    HDEBUG("");
    updateKeyRole();
    updateWatchRole();
    searchModel();
}

void FoilPicsModelWatch::Private::onModelRowsInserted(const QModelIndex& aParent,
    int aStart, int aEnd)
{
    HDEBUG(aStart << aEnd);
    searchModel(aStart, aEnd);
}

void FoilPicsModelWatch::Private::onModelRowsRemoved(const QModelIndex& aParent,
    int aStart, int aEnd)
{
    HDEBUG(aStart << aEnd);
    if (iIndex >= aStart) {
        searchModel(aStart, iModel->rowCount() - 1);
    }
}

void FoilPicsModelWatch::Private::onModelRowsMoved(const QModelIndex& aSourceParent,
    int aSourceStart, int aSourceEnd, const QModelIndex& aDestParent, int aDest)
{
    HDEBUG(aSourceStart << aSourceEnd << "->" << aDest);
    searchModel(qMin(aSourceStart, aDest),
        qMax(aSourceEnd, aDest + aSourceEnd - aSourceStart));
}

void FoilPicsModelWatch::Private::onModelDataChanged(const QModelIndex& aTopLeft,
    const QModelIndex& aBottomRight, const QVector<int>& aRoles)
{
    const int top = aTopLeft.row();
    const int bottom = aBottomRight.row();
    HDEBUG(top << bottom);
    if (iIndex < 0 || (iIndex >= top && iIndex <= bottom)) {
        searchModel(top, bottom);
    }
}

// ==========================================================================
// FoilPicsModelWatch
// ==========================================================================

FoilPicsModelWatch::FoilPicsModelWatch(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

QAbstractItemModel* FoilPicsModelWatch::model() const
{
    return iPrivate->iModel;
}

void FoilPicsModelWatch::setModel(QAbstractItemModel* aModel)
{
    iPrivate->setModel(aModel);
}

QString FoilPicsModelWatch::keyRole() const
{
    return iPrivate->iKeyRoleName;
}

void FoilPicsModelWatch::setKeyRole(QString aRole)
{
    iPrivate->setKeyRole(aRole);
}

QString FoilPicsModelWatch::keyValue() const
{
    return iPrivate->iKeyValue;
}

void FoilPicsModelWatch::setKeyValue(QString aValue)
{
    iPrivate->setKeyValue(aValue);
}

QString FoilPicsModelWatch::watchRole() const
{
    return iPrivate->iWatchRoleName;
}

void FoilPicsModelWatch::setWatchRole(QString aRole)
{
    iPrivate->setWatchRole(aRole);
}

QVariant FoilPicsModelWatch::watchValue() const
{
    return iPrivate->iWatchValue;
}

int FoilPicsModelWatch::index() const
{
    return iPrivate->iIndex;
}

#include "FoilPicsModelWatch.moc"
