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

#include "FoilPicsBusyState.h"
#include "FoilPicsSelection.h"

// ==========================================================================
// FoilPicsBusyState::Private
// ==========================================================================

class FoilPicsBusyState::Private : public QObject {
    Q_OBJECT

public:
    Private(FoilPicsBusyState* aParent);

    FoilPicsBusyState* parentObject() const;
    void setModel(FoilPicsSelection* aModel);
    void setKey(QString aKey);
    void updateBusy(bool aWasBusy);

public Q_SLOTS:
    void onBusyChanged();
    void onModelDestroyed();

public:
    FoilPicsSelection* iModel;
    QString iKey;
    bool iBusy;
};

FoilPicsBusyState::Private::Private(FoilPicsBusyState* aParent) :
    QObject(aParent),
    iModel(NULL),
    iBusy(false)
{
}

FoilPicsBusyState* FoilPicsBusyState::Private::parentObject() const
{
    return qobject_cast<FoilPicsBusyState*>(parent());
}

void FoilPicsBusyState::Private::setModel(FoilPicsSelection* aModel)
{
    if (iModel != aModel) {
        const bool wasBusy = iBusy;
        if (iModel) {
            iModel->disconnect(this);
        }
        iModel = aModel;
        if (iModel) {
            connect(iModel, SIGNAL(destroyed(QObject*)), SLOT(onModelDestroyed()));
            connect(iModel, SIGNAL(busyChanged(QString)), SLOT(onBusyChanged()));
        }
        updateBusy(wasBusy);
    }
}

void FoilPicsBusyState::Private::setKey(QString aKey)
{
    if (iKey != aKey) {
        const bool wasBusy = iBusy;
        iKey = aKey;
        Q_EMIT parentObject()->keyChanged();
        updateBusy(wasBusy);
    }
}

void FoilPicsBusyState::Private::updateBusy(bool aWasBusy)
{
    iBusy = iModel && iModel->busy(iKey);
    if (iBusy != aWasBusy) {
        Q_EMIT parentObject()->busyChanged();
    }
}

void FoilPicsBusyState::Private::onBusyChanged()
{
    updateBusy(iBusy);
}

void FoilPicsBusyState::Private::onModelDestroyed()
{
    const bool wasBusy = iBusy;
    iModel = NULL;
    Q_EMIT parentObject()->modelChanged();
    updateBusy(wasBusy);
}

// ==========================================================================
// FoilPicsBusyState
// ==========================================================================

FoilPicsBusyState::FoilPicsBusyState(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

QObject* FoilPicsBusyState::model() const
{
    return iPrivate->iModel;
}

void FoilPicsBusyState::setModel(QObject* aModel)
{
    iPrivate->setModel(qobject_cast<FoilPicsSelection*>(aModel));
}

QString FoilPicsBusyState::key() const
{
    return iPrivate->iKey;
}

void FoilPicsBusyState::setKey(QString aKey)
{
    iPrivate->setKey(aKey);
}

bool FoilPicsBusyState::busy() const
{
    return iPrivate->iBusy;
}

#include "FoilPicsBusyState.moc"
