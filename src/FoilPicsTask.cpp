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

#include "FoilPicsTask.h"

#include "HarbourDebug.h"

#include <QCoreApplication>

FoilPicsTask::FoilPicsTask(QThreadPool* aPool) :
    QObject(aPool),
    iAboutToQuit(false),
    iSubmitted(false),
    iStarted(false),
    iReleased(false),
    iDone(false)
{
    setAutoDelete(false);
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(onAboutToQuit()));
    connect(this, SIGNAL(runFinished()), SLOT(onRunFinished()),
        Qt::QueuedConnection);
}

FoilPicsTask::~FoilPicsTask()
{
    HASSERT(iReleased);
    if (iSubmitted) wait();
}

void FoilPicsTask::submit()
{
    HASSERT(!iSubmitted);
    iSubmitted = true;
    qobject_cast<QThreadPool*>(parent())->start(this);
}

void FoilPicsTask::submit(QObject* aTarget, const char* aSlot)
{
    aTarget->connect(this, SIGNAL(done()), aSlot);
    submit();
}

void FoilPicsTask::release(QObject* aHandler)
{
    disconnect(aHandler);
    released();
}

void FoilPicsTask::release()
{
    disconnect();
    released();
}

void FoilPicsTask::released()
{
    iReleased = true;
    if (!iSubmitted || iDone) {
        delete this;
    }
}

void FoilPicsTask::run()
{
    HASSERT(!iStarted);
    iStarted = true;
    performTask();
    Q_EMIT runFinished();
}

void FoilPicsTask::onRunFinished()
{
    HASSERT(!iDone);
    if (!iReleased) {
        Q_EMIT done();
    }
    iDone = true;
    if (iReleased) {
        delete this;
    }
}

void FoilPicsTask::onAboutToQuit()
{
    HDEBUG("OK");
    iAboutToQuit = true;
}
