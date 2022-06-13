/*
 * Copyright (C) 2019-2022 Jolla Ltd.
 * Copyright (C) 2019-2022 Slava Monich <slava@monich.com>
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
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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

#include "FoilPicsSettings.h"
#include "FoilPicsDefs.h"

#include "HarbourDebug.h"

#include <MGConfItem>

#define DCONF_KEY(x)                FOILPICS_DCONF_ROOT x
#define KEY_SHARED_KEY_WARNING      DCONF_KEY("sharedKeyWarning")
#define KEY_SHARED_KEY_WARNING2     DCONF_KEY("sharedKeyWarning2")
#define KEY_AUTO_LOCK               DCONF_KEY("autoLock")
#define KEY_AUTO_LOCK_TIME          DCONF_KEY("autoLockTime")

#define DEFAULT_SHARED_KEY_WARNING  true
#define DEFAULT_AUTO_LOCK           true
#define DEFAULT_AUTO_LOCK_TIME      15000

// ==========================================================================
// FoilPicsSettings::Private
// ==========================================================================

class FoilPicsSettings::Private {
public:
    Private(FoilPicsSettings* aParent);

public:
    MGConfItem* iSharedKeyWarning;
    MGConfItem* iSharedKeyWarning2;
    MGConfItem* iAutoLock;
    MGConfItem* iAutoLockTime;
};

FoilPicsSettings::Private::Private(FoilPicsSettings* aParent) :
    iSharedKeyWarning(new MGConfItem(KEY_SHARED_KEY_WARNING, aParent)),
    iSharedKeyWarning2(new MGConfItem(KEY_SHARED_KEY_WARNING2, aParent)),
    iAutoLock(new MGConfItem(KEY_AUTO_LOCK, aParent)),
    iAutoLockTime(new MGConfItem(KEY_AUTO_LOCK_TIME, aParent))
{
    QObject::connect(iSharedKeyWarning, SIGNAL(valueChanged()),
        aParent, SIGNAL(sharedKeyWarningChanged()));
    QObject::connect(iSharedKeyWarning2, SIGNAL(valueChanged()),
        aParent, SIGNAL(sharedKeyWarning2Changed()));
    QObject::connect(iAutoLock, SIGNAL(valueChanged()),
        aParent, SIGNAL(autoLockChanged()));
    QObject::connect(iAutoLockTime, SIGNAL(valueChanged()),
        aParent, SIGNAL(autoLockTimeChanged()));
}

// ==========================================================================
// FoilPicsSettings
// ==========================================================================

FoilPicsSettings::FoilPicsSettings(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

FoilPicsSettings::~FoilPicsSettings()
{
    delete iPrivate;
}

// Callback for qmlRegisterSingletonType<FoilPicsSettings>
QObject*
FoilPicsSettings::createSingleton(
    QQmlEngine* aEngine,
    QJSEngine* aScript)
{
    return new FoilPicsSettings();
}

// sharedKeyWarning
// sharedKeyWarning2

bool
FoilPicsSettings::sharedKeyWarning()
const
{
    return iPrivate->iSharedKeyWarning->value(DEFAULT_SHARED_KEY_WARNING).toBool();
}

bool
FoilPicsSettings::sharedKeyWarning2()
const
{
    return iPrivate->iSharedKeyWarning2->value(DEFAULT_SHARED_KEY_WARNING).toBool();
}

void
FoilPicsSettings::setSharedKeyWarning(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iSharedKeyWarning->set(aValue);
}

void
FoilPicsSettings::setSharedKeyWarning2(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iSharedKeyWarning2->set(aValue);
}

// autoLock

bool
FoilPicsSettings::autoLock() const
{
    return iPrivate->iAutoLock->value(DEFAULT_AUTO_LOCK).toBool();
}

void
FoilPicsSettings::setAutoLock(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iAutoLock->set(aValue);
}

// autoLockTime

int
FoilPicsSettings::autoLockTime()
const
{
    QVariant val(iPrivate->iAutoLockTime->value(DEFAULT_AUTO_LOCK_TIME));
    bool ok;
    const int ival(val.toInt(&ok));
    return (ok && ival >= 0) ? ival : DEFAULT_AUTO_LOCK_TIME;
}

void
FoilPicsSettings::setAutoLockTime(
    int aValue)
{
    HDEBUG(aValue);
    iPrivate->iAutoLockTime->set(aValue);
}
