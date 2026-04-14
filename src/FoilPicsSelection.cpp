/*
 * Copyright (C) 2018-2026 Slava Monich <slava@monich.com>
 * Copyright (C) 2018-2019 Jolla Ltd.
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

#include "FoilPicsSelection.h"

#include "FoilPicsRole.h"

#include "HarbourDebug.h"
#include "HarbourParentSignalQueueObject.h"

#include <QtCore/QAbstractItemModel>

// ==========================================================================
// FoilPicsSelection::Private
// ==========================================================================

// s(SignalName,signalName)
#define QUEUED_SIGNALS(s) \
    s(Model,model) \
    s(Role,role) \
    s(DuplicatesAllowed,duplicatesAllowed) \
    s(SelectionCount,selectionCount) \
    s(BusyCount,busyCount)

enum FoilPicsSelectionSignal {
    #define SIGNAL_ENUM_(Name,name) Signal##Name##Changed,
    QUEUED_SIGNALS(SIGNAL_ENUM_)
    #undef SIGNAL_ENUM_
    FoilPicsSelectionSignalCount
};

typedef HarbourParentSignalQueueObject<FoilPicsSelection,
    FoilPicsSelectionSignal, FoilPicsSelectionSignalCount>
    FoilPicsSelectionPrivateBase;

class FoilPicsSelection::Private :
    public FoilPicsSelectionPrivateBase
{
    Q_OBJECT

    static const SignalEmitter gSignalEmitters [];

public:
    Private(FoilPicsSelection*);

    void emitSelectionChanged(const QStringList&);
    void emitBusyChanged(const QStringList&);
    QString keyAt(int);
    void updateRows(int);
    void setKeyRole(const QString&);
    void setModel(QAbstractItemModel*);
    void refreshModelAndEmitSignals();
    void selectAll();
    void clearSelection();
    void makeBusy(const QList<int>&);

public Q_SLOTS:
    void onModelDestroyed();
    void onModelReset();
    void onModelRowsInserted(const QModelIndex&, int, int);
    void onModelRowsRemoved(const QModelIndex&, int, int);
    void onModelRowsMoved(const QModelIndex&, int, int,
        const QModelIndex&, int);
    void onModelDataChanged(const QModelIndex&, const QModelIndex&,
        const QVector<int>&);

public:
    QStringList iKeyList;
    QHash<QString,int> iKeys;
    QSet<QString> iSelected;
    QSet<QString> iBusy;
    QAbstractItemModel* iModel;
    QString iKeyRoleName;
    bool iDuplicatesAllowed;
    int iKeyRole;
    int iColumn;
};

const FoilPicsSelection::Private::SignalEmitter
FoilPicsSelection::Private::gSignalEmitters [] = {
    #define SIGNAL_EMITTER_(Name,name) &FoilPicsSelection::name##Changed,
    QUEUED_SIGNALS(SIGNAL_EMITTER_)
    #undef  SIGNAL_EMITTER_
};

FoilPicsSelection::Private::Private(
    FoilPicsSelection* aParent) :
    FoilPicsSelectionPrivateBase(aParent, gSignalEmitters),
    iModel(Q_NULLPTR),
    iDuplicatesAllowed(true),
    iKeyRole(-1),
    iColumn(0)
{}

void
FoilPicsSelection::Private::emitSelectionChanged(
    const QStringList& aChanged)
{
    if (!aChanged.isEmpty()) {
        FoilPicsSelection* obj = parentObject();

        if (iSelected.isEmpty()) {
            Q_EMIT obj->selectionCleared();
        } else {
            const int k = aChanged.count();

            for (int j = 0; j < k; j++) {
                Q_EMIT obj->selectionChanged(aChanged.at(j));
            }
        }
    }
}

void
FoilPicsSelection::Private::emitBusyChanged(
    const QStringList& aChanged)
{
    if (!aChanged.isEmpty()) {
        FoilPicsSelection* obj = parentObject();
        const int k = aChanged.count();

        for (int j = 0; j < k; j++) {
            Q_EMIT obj->busyChanged(aChanged.at(j));
        }
    }
}

QString
FoilPicsSelection::Private::keyAt(
    int aIndex)
{
    if (iModel && aIndex >= 0 && aIndex < iModel->rowCount()) {
        QVariant var(iModel->data(iModel->index(aIndex, iColumn), iKeyRole));

        if (var.isValid()) {
            return var.toString();
        }
    }
    return QString();
}

void
FoilPicsSelection::Private::updateRows(
    int aFrom)
{
    const int n = iModel->rowCount();

    HASSERT(iKeyList.count() == n);
    for (int i = aFrom; i < n; i++) {
        const QString key(iKeyList.at(i));

        if (!key.isEmpty()) {
            HDEBUG(key << i);
            iKeys.insert(key, i);
        }
    }
}

void
FoilPicsSelection::Private::setKeyRole(
    const QString& aRole)
{
    if (iKeyRoleName != aRole) {
        iKeyRoleName = aRole;
        HDEBUG(aRole);
        queueSignal(SignalRoleChanged);
        refreshModelAndEmitSignals();
    }
}

void
FoilPicsSelection::Private::setModel(
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
        refreshModelAndEmitSignals();
    }
}

void
FoilPicsSelection::Private::refreshModelAndEmitSignals()
{
    QStringList selectionChanged;
    QStringList busyChanged;

    iKeyList.clear();
    iKeys.clear();
    iKeyRole = FoilPicsRole::find(iModel, iKeyRoleName);
    if (iKeyRole >= 0) {
        // Collect all available values
        const int n = iModel->rowCount();
        for (int i = 0; i < n; i++) {
            const QString key(keyAt(i));
            if (!key.isEmpty()) {
                iKeyList.append(key);
                iKeys.insert(key, i);
            }
        }

        // Remove stale entries
        const QStringList selected(iSelected.values());
        const int k = selected.count();

        for (int j = k - 1; j >= 0; j--) {
            const QString key(selected.at(j));
            if (!iKeys.contains(key)) {
                HDEBUG("selected" << key << "is gone");
                iSelected.remove(key);
                selectionChanged.append(key);
                queueSignal(SignalSelectionCountChanged);
            }
        }

        const QStringList busy(iBusy.values());
        const int m = busy.count();

        for (int l = m - 1; l >= 0; l--) {
            const QString key(busy.at(l));

            if (!iKeys.contains(key)) {
                HDEBUG("busy" << key << "is gone");
                iBusy.remove(key);
                busyChanged.append(key);
                queueSignal(SignalBusyCountChanged);
            }
        }
    } else {
        if (!iSelected.isEmpty()) {
            selectionChanged = iSelected.values();
            iSelected.clear();
            queueSignal(SignalSelectionCountChanged);
        }
        if (!iBusy.isEmpty()) {
            busyChanged = iBusy.values();
            iBusy.clear();
            queueSignal(SignalBusyCountChanged);
        }
    }

    HDEBUG(iKeys);
    emitSelectionChanged(selectionChanged);
    emitBusyChanged(busyChanged);
    emitQueuedSignals();
}

void
FoilPicsSelection::Private::selectAll()
{
    QStringList changed;
    const int n = iKeyList.count();

    for (int i = 0; i < n; i++) {
        const QString key(iKeyList.at(i));

        if (!iSelected.contains(key)) {
            iSelected.insert(key);
            changed.append(key);
            HDEBUG("+" << key << iSelected.count());
            queueSignal(SignalSelectionCountChanged);
        }
    }
    emitSelectionChanged(changed);
    emitQueuedSignals();
}

void
FoilPicsSelection::Private::clearSelection()
{
    if (!iSelected.isEmpty()) {
        iSelected.clear();
        HDEBUG("");
        queueSignal(SignalSelectionCountChanged);
        Q_EMIT parentObject()->selectionCleared();
        emitQueuedSignals();
    }
}

void
FoilPicsSelection::Private::makeBusy(
    const QList<int>& aList)
{
    const int n = aList.count();
    const int count = iKeyList.count();
    QStringList busyChanged, selectionChanged;

    for (int i = 0; i < n; i++) {
        const int pos = aList.at(i);

        if (pos >= 0 && pos < count) {
            const QString key = iKeyList.at(pos);

            if (!iBusy.contains(key)) {
                iBusy.insert(key);
                busyChanged.append(key);
                queueSignal(SignalBusyCountChanged);
            }
            if (iSelected.contains(key)) {
                iSelected.remove(key);
                selectionChanged.append(key);
                queueSignal(SignalSelectionCountChanged);
            }
        } else {
            HWARN("Invalid index" << pos);
        }
    }
    emitSelectionChanged(selectionChanged);
    emitBusyChanged(busyChanged);
    emitQueuedSignals();
}

void
FoilPicsSelection::Private::onModelDestroyed()
{
    HDEBUG("");
    iModel = Q_NULLPTR;
    queueSignal(SignalModelChanged);
    refreshModelAndEmitSignals();
}

void
FoilPicsSelection::Private::onModelReset()
{
    HDEBUG("");
    refreshModelAndEmitSignals();
}

void
FoilPicsSelection::Private::onModelRowsInserted(
    const QModelIndex&,
    int aStart,
    int aEnd)
{
    HDEBUG(aStart << aEnd);
    if (iKeyRole < 0) {
        // In case of DocumentGalleryModel not all roles show up right away,
        // they become available by the time the model gets populated.
        refreshModelAndEmitSignals();
    } else {
        // Insert the new rows
        for (int i = aStart; i <= aEnd; i++) {
            const QString key(keyAt(i));

            iKeyList.insert(i, key);
            if (!key.isEmpty()) {
                HDEBUG(key << i);
                iKeys.insert(key, i);
            }
        }
    }

    // Shift the remaining rows
    updateRows(aEnd + 1);
}

void
FoilPicsSelection::Private::onModelRowsMoved(
    const QModelIndex&,
    int aSourceStart,
    int aSourceEnd,
    const QModelIndex&,
    int aDest)
{
    HDEBUG(aSourceStart << aSourceEnd << "->" << aDest);
    updateRows(qMin(aSourceStart, aDest));
    HASSERT(!haveQueuedSignals()); // No signals are expected
}

void
FoilPicsSelection::Private::onModelRowsRemoved(
    const QModelIndex&,
    int aStart,
    int aEnd)
{
    QStringList removed;

    HDEBUG(aStart << aEnd);
    HASSERT(aEnd < iKeyList.count());

    // Create the full list of removed keys
    for (int i = qMin(aEnd, iKeyList.count() - 1); i >= aStart; i--) {
        const QString key(iKeyList.takeAt(i));

        HDEBUG(key << i);
        if (!removed.contains(key)) {
            removed.append(key);
        }
    }

    if (iDuplicatesAllowed) {
        // The case of DocumentGalleryModel. It adds duplicates and then
        // removes it, meaning that if the entry is removed it doesn't
        // necessarily mean that the key is no longer valid. We have to
        // check if they are still there or not. That's bad.
        for (int j = removed.count() - 1; j >= 0; j--) {
            const QString key(removed.at(j));
            const int pos = iKeyList.indexOf(key);

            if (pos >= 0) {
                // Update the index
                HDEBUG(key << "is still there at" << pos);
                HDEBUG(iKeyList);
                removed.removeAt(j);
                iKeys.insert(key, pos);
            }
        }
    }

    // Remove the stale rows from the map
    QStringList selectionChanged, busyChanged;

    for (int j = removed.count() - 1; j >= 0; j--) {
        const QString key(removed.at(j));

        iKeys.remove(key);
        if (iSelected.contains(key)) {
            HDEBUG(key << "was selected");
            iSelected.remove(key);
            selectionChanged.append(key);
            queueSignal(SignalSelectionCountChanged);
        }
        if (iBusy.contains(key)) {
            HDEBUG(key << "was busy");
            iBusy.remove(key);
            busyChanged.append(key);
            queueSignal(SignalBusyCountChanged);
        }
    }

    // Shift the affected rows and emit the events
    updateRows(aStart);
    emitSelectionChanged(selectionChanged);
    emitBusyChanged(busyChanged);
    emitQueuedSignals();
}

void
FoilPicsSelection::Private::onModelDataChanged(
    const QModelIndex& aTopLeft,
    const QModelIndex& aBottomRight,
    const QVector<int>& aRoles)
{
    // This happens very rarely, if ever
    HDEBUG(aTopLeft.row() << aBottomRight.row() << aRoles);
    refreshModelAndEmitSignals();
}

// ==========================================================================
// FoilPicsSelection
// ==========================================================================

FoilPicsSelection::FoilPicsSelection(
    QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
    HDEBUG("created");
}

FoilPicsSelection::~FoilPicsSelection()
{
    HDEBUG("destroyed");
}

int
FoilPicsSelection::selectionCount() const
{
    return iPrivate->iSelected.count();
}

int
FoilPicsSelection::busyCount() const
{
    return iPrivate->iBusy.count();
}

QAbstractItemModel*
FoilPicsSelection::model() const
{
    return iPrivate->iModel;
}

void
FoilPicsSelection::setModel(
    QObject* aModel)
{
    iPrivate->setModel(qobject_cast<QAbstractItemModel*>(aModel));
}

QString
FoilPicsSelection::role() const
{
    return iPrivate->iKeyRoleName;
}

void
FoilPicsSelection::setRole(
    QString aRole)
{
    iPrivate->setKeyRole(aRole);
}

bool
FoilPicsSelection::duplicatesAllowed() const
{
    return iPrivate->iDuplicatesAllowed;
}

void
FoilPicsSelection::setDuplicatesAllowed(
    bool aValue)
{
    if (iPrivate->iDuplicatesAllowed != aValue) {
        iPrivate->iDuplicatesAllowed = aValue;
        HDEBUG(aValue);
        Q_EMIT duplicatesAllowedChanged();
    }
}

bool
FoilPicsSelection::busy(
    QString aValue) const
{
    return iPrivate->iBusy.contains(aValue);
}

bool
FoilPicsSelection::selected(
    QString aValue) const
{
    return iPrivate->iSelected.contains(aValue);
}

void
FoilPicsSelection::select(
    QString aValue)
{
    if (iPrivate->iKeys.contains(aValue) &&
        !iPrivate->iSelected.contains(aValue) &&
        !iPrivate->iBusy.contains(aValue)) {
        iPrivate->iSelected.insert(aValue);
        HDEBUG("+" << aValue << selectionCount());
        Q_EMIT selectionChanged(aValue);
        Q_EMIT selectionCountChanged();
    } else {
        HDEBUG(aValue << "is not selectable:" <<
            iPrivate->iKeys.contains(aValue) <<
            iPrivate->iSelected.contains(aValue) <<
            iPrivate->iBusy.contains(aValue));
    }
}

void
FoilPicsSelection::unselect(
    QString aValue)
{
    if (iPrivate->iSelected.remove(aValue)) {
        HDEBUG("-" << aValue << selectionCount());
        Q_EMIT selectionChanged(aValue);
        Q_EMIT selectionCountChanged();
    } else {
        HDEBUG(aValue << "is not selected");
    }
}

void
FoilPicsSelection::toggleSelection(
    QString aValue)
{
    if (iPrivate->iKeys.contains(aValue)) {
        if (iPrivate->iSelected.remove(aValue)) {
            HDEBUG("-" << aValue << selectionCount());
            Q_EMIT selectionChanged(aValue);
            Q_EMIT selectionCountChanged();
        } else if (!iPrivate->iBusy.contains(aValue)) {
            iPrivate->iSelected.insert(aValue);
            HDEBUG("+" << aValue << selectionCount());
            Q_EMIT selectionChanged(aValue);
            Q_EMIT selectionCountChanged();
        } else {
            HDEBUG(aValue << "can't be selected because it's busy");
        }
    } else {
        HDEBUG(aValue << "is not in the model");
    }
}

void
FoilPicsSelection::clearSelection()
{
    iPrivate->clearSelection();
}

void
FoilPicsSelection::selectAll()
{
    iPrivate->selectAll();
}

QList<int>
FoilPicsSelection::getSelectedRows()
{
    QList<int> selection;
    const QStringList keys = iPrivate->iSelected.values();
    const int n = keys.count();

    if (n > 0) {
        selection.reserve(n);
        for (int i = 0; i < n; i++) {
            selection.append(iPrivate->iKeys.value(keys.at(i)));
        }
        qSort(selection);
        HDEBUG(selection);
    }
    return selection;
}

void
FoilPicsSelection::makeBusy(
    QList<int> aList)
{
    return iPrivate->makeBusy(aList);
}

#include "FoilPicsSelection.moc"
