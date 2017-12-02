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

#include "FoilPicsHints.h"
#include "FoilPicsDefs.h"
#include "HarbourDebug.h"

#include <MGConfItem>

#define DCONF_PATH                      FOILPICS_DCONF_ROOT "hints/"
#define KEY_LEFT_SWIPE_TO_GALLERY       DCONF_PATH "leftSwipeToGallery"
#define KEY_LEFT_SWIPE_TO_DECRYPTED     DCONF_PATH "leftSwipeToDecrypted"
#define KEY_RIGHT_SWIPE_TO_ENCRYPTED    DCONF_PATH "rightSwipeToEncrypted"
#define KEY_LETS_ENCRYPT_SOMETHING      DCONF_PATH "letsEncryptSomething"

FoilPicsHints::FoilPicsHints(QObject* aParent) :
    QObject(aParent),
    iLeftSwipeToGallery(new MGConfItem(KEY_LEFT_SWIPE_TO_GALLERY, this)),
    iLeftSwipeToDecrypted(new MGConfItem(KEY_LEFT_SWIPE_TO_DECRYPTED, this)),
    iRightSwipeToEncrypted(new MGConfItem(KEY_RIGHT_SWIPE_TO_ENCRYPTED, this)),
    iLetsEncryptSomething(new MGConfItem(KEY_LETS_ENCRYPT_SOMETHING, this))
{
    connect(iLeftSwipeToGallery, SIGNAL(valueChanged()),
        SIGNAL(leftSwipeToGalleryChanged()));
    connect(iLeftSwipeToDecrypted, SIGNAL(valueChanged()),
        SIGNAL(leftSwipeToDecryptedChanged()));
    connect(iRightSwipeToEncrypted, SIGNAL(valueChanged()),
        SIGNAL(rightSwipeToEncryptedChanged()));
    connect(iLetsEncryptSomething, SIGNAL(valueChanged()),
        SIGNAL(letsEncryptSomethingChanged()));
}

// leftSwipeToGallery

int
FoilPicsHints::leftSwipeToGallery() const
{
    return iLeftSwipeToGallery->value(0).toInt();
}

void
FoilPicsHints::setLeftSwipeToGallery(
    int aValue)
{
    HDEBUG(aValue);
    iLeftSwipeToGallery->set(aValue);
}

// leftSwipeToDecrypted

int
FoilPicsHints::leftSwipeToDecrypted() const
{
    return iLeftSwipeToDecrypted->value(0).toInt();
}

void
FoilPicsHints::setLeftSwipeToDecrypted(
    int aValue)
{
    HDEBUG(aValue);
    iLeftSwipeToDecrypted->set(aValue);
}

// rightSwipeToEncrypted

int
FoilPicsHints::rightSwipeToEncrypted() const
{
    return iRightSwipeToEncrypted->value(0).toInt();
}

void
FoilPicsHints::setRightSwipeToEncrypted(
    int aValue)
{
    HDEBUG(aValue);
    iRightSwipeToEncrypted->set(aValue);
}

// letsEncryptSomething

int
FoilPicsHints::letsEncryptSomething() const
{
    return iLetsEncryptSomething->value(0).toInt();
}

void
FoilPicsHints::setLetsEncryptSomething(
    int aValue)
{
    HDEBUG(aValue);
    iLetsEncryptSomething->set(aValue);
}
