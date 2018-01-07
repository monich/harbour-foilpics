/*
 * Copyright (C) 2018 Jolla Ltd.
 * Copyright (C) 2018 Slava Monich <slava@monich.com>
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

#include "FoilPicsSelectionState.h"
#include "FoilPicsSelection.h"

// ==========================================================================
// FoilPicsSelectionState::Private
// ==========================================================================

class FoilPicsSelectionState::Private : public QObject {
    Q_OBJECT

public:
    Private(FoilPicsSelectionState* aParent);

    FoilPicsSelectionState* parentObject() const;
    void setModel(FoilPicsSelection* aModel);
    void setKey(QString aKey);
    void updateSelected(bool aWasSelected);

public Q_SLOTS:
    void onSelectionChanged();
    void onModelDestroyed();

public:
    FoilPicsSelection* iModel;
    QString iKey;
    bool iSelected;
};

FoilPicsSelectionState::Private::Private(FoilPicsSelectionState* aParent) :
    QObject(aParent),
    iModel(NULL),
    iSelected(false)
{
}

FoilPicsSelectionState* FoilPicsSelectionState::Private::parentObject() const
{
    return qobject_cast<FoilPicsSelectionState*>(parent());
}

void FoilPicsSelectionState::Private::setModel(FoilPicsSelection* aModel)
{
    if (iModel != aModel) {
        const bool wasSelected = iSelected;
        if (iModel) {
            iModel->disconnect(this);
        }
        iModel = aModel;
        if (iModel) {
            connect(iModel, SIGNAL(destroyed(QObject*)), SLOT(onModelDestroyed()));
            connect(iModel, SIGNAL(selectionCleared()), SLOT(onSelectionChanged()));
            connect(iModel, SIGNAL(selectionChanged(QString)), SLOT(onSelectionChanged()));
        }
        updateSelected(wasSelected);
    }
}

void FoilPicsSelectionState::Private::setKey(QString aKey)
{
    if (iKey != aKey) {
        const bool wasSelected = iSelected;
        iKey = aKey;
        Q_EMIT parentObject()->keyChanged();
        updateSelected(wasSelected);
    }
}

void FoilPicsSelectionState::Private::updateSelected(bool aWasSelected)
{
    iSelected = iModel && iModel->selected(iKey);
    if (iSelected != aWasSelected) {
        Q_EMIT parentObject()->selectedChanged();
    }
}

void FoilPicsSelectionState::Private::onSelectionChanged()
{
    updateSelected(iSelected);
}

void FoilPicsSelectionState::Private::onModelDestroyed()
{
    const bool wasSelected = iSelected;
    iModel = NULL;
    Q_EMIT parentObject()->modelChanged();
    updateSelected(wasSelected);
}

// ==========================================================================
// FoilPicsSelectionState
// ==========================================================================

FoilPicsSelectionState::FoilPicsSelectionState(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

QObject* FoilPicsSelectionState::model() const
{
    return iPrivate->iModel;
}

void FoilPicsSelectionState::setModel(QObject* aModel)
{
    iPrivate->setModel(qobject_cast<FoilPicsSelection*>(aModel));
}

QString FoilPicsSelectionState::key() const
{
    return iPrivate->iKey;
}

void FoilPicsSelectionState::setKey(QString aKey)
{
    iPrivate->setKey(aKey);
}

bool FoilPicsSelectionState::selected() const
{
    return iPrivate->iSelected;
}

#include "FoilPicsSelectionState.moc"
