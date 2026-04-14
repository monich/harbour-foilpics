/*
 * Copyright (C) 2017-2026 Slava Monich <slava@monich.com>
 * Copyright (C) 2017-2018 Jolla Ltd.
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

#include "FoilPicsModelWatch.h"

#include "FoilPicsRole.h"

#include "HarbourDebug.h"
#include "HarbourParentSignalQueueObject.h"

#include <QtCore/QAbstractItemModel>

// ==========================================================================
// FoilPicsModelWatch::Private
// ==========================================================================

// s(SignalName,signalName)
#define QUEUED_SIGNALS(s) \
    s(Model,model) \
    s(KeyRole,keyRole) \
    s(KeyValue,keyValue) \
    s(WatchRole,watchRole) \
    s(WatchValue,watchValue) \
    s(Index,index)

enum FoilPicsModelWatchSignal {
    #define SIGNAL_ENUM_(Name,name) Signal##Name##Changed,
    QUEUED_SIGNALS(SIGNAL_ENUM_)
    #undef SIGNAL_ENUM_
    FoilPicsModelWatchSignalCount
};

typedef HarbourParentSignalQueueObject<FoilPicsModelWatch,
    FoilPicsModelWatchSignal, FoilPicsModelWatchSignalCount>
    FoilPicsModelWatchPrivateBase;

class FoilPicsModelWatch::Private :
    public FoilPicsModelWatchPrivateBase
{
    Q_OBJECT

    static const SignalEmitter gSignalEmitters [];

public:
    Private(FoilPicsModelWatch*);

    bool isUsable() const;
    void setModel(QAbstractItemModel*);
    void setKeyRole(const QString&);
    void setKeyValue(const QString&);
    void setWatchRole(const QString&);
    void updateKeyRole();
    void updateWatchRole();
    void updateWatchValue();
    void reset();
    void searchModel();
    void searchModel(int, int);

public Q_SLOTS:
    void onModelDestroyed();
    void onModelReset();
    void onModelRowsInserted(const QModelIndex&, int, int);
    void onModelRowsRemoved(const QModelIndex&, int, int);
    void onModelRowsMoved(const QModelIndex&, int, int, const QModelIndex&, int);
    void onModelDataChanged(const QModelIndex&, const QModelIndex&,
        const QVector<int>&);

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

const FoilPicsModelWatch::Private::SignalEmitter
    FoilPicsModelWatch::Private::gSignalEmitters [] = {
    #define SIGNAL_EMITTER_(Name,name) &FoilPicsModelWatch::name##Changed,
    QUEUED_SIGNALS(SIGNAL_EMITTER_)
    #undef  SIGNAL_EMITTER_
};

FoilPicsModelWatch::Private::Private(
    FoilPicsModelWatch* aParent) :
    FoilPicsModelWatchPrivateBase(aParent, gSignalEmitters),
    iModel(NULL),
    iKeyRole(-1),
    iColumn(0),
    iWatchRole(-1),
    iIndex(-1)
{}

bool
FoilPicsModelWatch::Private::isUsable() const
{
    return iKeyRole >= 0 && !iKeyRoleName.isEmpty() && !iKeyValue.isEmpty() &&
        iModel && iModel->rowCount() > 0;
}

void
FoilPicsModelWatch::Private::setModel(
    QAbstractItemModel* aModel)
{
    if (iModel != aModel) {
        if (iModel) {
            iModel->disconnect(this);
        }
        iModel = aModel;
        if (iModel) {
            connect(iModel,
                SIGNAL(destroyed(QObject*)),
                SLOT(onModelDestroyed()));
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
        queueSignal(SignalModelChanged);
        updateKeyRole();
        updateWatchRole();
    }
}

void
FoilPicsModelWatch::Private::updateKeyRole()
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

void
FoilPicsModelWatch::Private::updateWatchRole()
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
            queueSignal(SignalWatchValueChanged);
        }
    }
}

void
FoilPicsModelWatch::Private::updateWatchValue()
{
    QVariant value;

    if (iWatchRole >= 0 && iIndex >= 0) {
        const QModelIndex index = iModel->index(iIndex, iColumn);

        value = iModel->data(index, iWatchRole);
    }
    if (iWatchValue != value) {
        iWatchValue = value;
        HDEBUG(iWatchRoleName << "=" << iWatchValue);
        queueSignal(SignalWatchValueChanged);
    }
}

void
FoilPicsModelWatch::Private::setKeyRole(
    const QString& aRole)
{
    if (iKeyRoleName != aRole) {
        iKeyRoleName = aRole;
        queueSignal(SignalKeyRoleChanged);
        updateKeyRole();
    }
}

void
FoilPicsModelWatch::Private::setKeyValue(
    const QString& aValue)
{
    if (iKeyValue != aValue) {
        iKeyValue = aValue;
        queueSignal(SignalKeyValueChanged);
        searchModel();
    }
}

void
FoilPicsModelWatch::Private::setWatchRole(
    const QString& aRole)
{
    if (iWatchRoleName != aRole) {
        iWatchRoleName = aRole;
        queueSignal(SignalWatchRoleChanged);
        updateWatchRole();
    }
}

void
FoilPicsModelWatch::Private::reset()
{
    if (iIndex >= 0) {
        iIndex = -1;
        queueSignal(SignalIndexChanged);
    }
    if (iWatchValue.isValid()) {
        iWatchValue.clear();
        queueSignal(SignalWatchValueChanged);
    }
}

void
FoilPicsModelWatch::Private::searchModel()
{
    if (isUsable()) {
        searchModel(0, iModel->rowCount() - 1);
    } else {
        reset();
    }
}

void
FoilPicsModelWatch::Private::searchModel(
    int aFirstRow,
    int aLastRow)
{
    if (isUsable()) {
        for (int row = aFirstRow; row <= aLastRow; row++) {
            const QModelIndex index(iModel->index(row, iColumn));

            if (iKeyValue == iModel->data(index, iKeyRole).toString()) {
                if (iIndex != row) {
                    iIndex = row;
                    HDEBUG(iKeyValue << "at" << row);
                    queueSignal(SignalIndexChanged);
                }
                updateWatchValue();
                return;
            }
        }
    }
    reset();
}

void
FoilPicsModelWatch::Private::onModelDestroyed()
{
    HDEBUG("");
    iModel = Q_NULLPTR;
    queueSignal(SignalModelChanged);
    reset();
    emitQueuedSignals();
}

void
FoilPicsModelWatch::Private::onModelReset()
{
    HDEBUG("");
    updateKeyRole();
    updateWatchRole();
    searchModel();
    emitQueuedSignals();
}

void
FoilPicsModelWatch::Private::onModelRowsInserted(
    const QModelIndex&,
    int aStart,
    int aEnd)
{
    HDEBUG(aStart << aEnd);
    searchModel(aStart, aEnd);
    emitQueuedSignals();
}

void
FoilPicsModelWatch::Private::onModelRowsRemoved(
    const QModelIndex&,
    int aStart,
    int aEnd)
{
    HDEBUG(aStart << aEnd);
    if (iIndex >= aStart) {
        searchModel(aStart, iModel->rowCount() - 1);
        emitQueuedSignals();
    }
}

void
FoilPicsModelWatch::Private::onModelRowsMoved(
    const QModelIndex&,
    int aSourceStart,
    int aSourceEnd,
    const QModelIndex&,
    int aDest)
{
    HDEBUG(aSourceStart << aSourceEnd << "->" << aDest);
    searchModel(qMin(aSourceStart, aDest),
        qMax(aSourceEnd, aDest + aSourceEnd - aSourceStart));
    emitQueuedSignals();
}

void
FoilPicsModelWatch::Private::onModelDataChanged(
    const QModelIndex& aTopLeft,
    const QModelIndex& aBottomRight,
    const QVector<int>&)
{
    const int top = aTopLeft.row();
    const int bottom = aBottomRight.row();

    HDEBUG(top << bottom);
    if (iIndex < 0 || (iIndex >= top && iIndex <= bottom)) {
        searchModel(top, bottom);
        emitQueuedSignals();
    }
}

// ==========================================================================
// FoilPicsModelWatch
// ==========================================================================

FoilPicsModelWatch::FoilPicsModelWatch(
    QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{}

QAbstractItemModel*
FoilPicsModelWatch::model() const
{
    return iPrivate->iModel;
}

void
FoilPicsModelWatch::setModel(
    QObject* aModel)
{
    iPrivate->setModel(qobject_cast<QAbstractItemModel*>(aModel));
    iPrivate->emitQueuedSignals();
}

QString
FoilPicsModelWatch::keyRole() const
{
    return iPrivate->iKeyRoleName;
}

void
FoilPicsModelWatch::setKeyRole(
    QString aRole)
{
    iPrivate->setKeyRole(aRole);
    iPrivate->emitQueuedSignals();
}

QString
FoilPicsModelWatch::keyValue() const
{
    return iPrivate->iKeyValue;
}

void
FoilPicsModelWatch::setKeyValue(
    QString aValue)
{
    iPrivate->setKeyValue(aValue);
    iPrivate->emitQueuedSignals();
}

QString
FoilPicsModelWatch::watchRole() const
{
    return iPrivate->iWatchRoleName;
}

void
FoilPicsModelWatch::setWatchRole(
    QString aRole)
{
    iPrivate->setWatchRole(aRole);
    iPrivate->emitQueuedSignals();
}

QVariant
FoilPicsModelWatch::watchValue() const
{
    return iPrivate->iWatchValue;
}

int
FoilPicsModelWatch::index() const
{
    return iPrivate->iIndex;
}

#include "FoilPicsModelWatch.moc"
