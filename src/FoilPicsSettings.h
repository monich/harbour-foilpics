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

#ifndef FOILPICS_SETTINGS_H
#define FOILPICS_SETTINGS_H

#include <QObject>

class QQmlEngine;
class QJSEngine;

class FoilPicsSettings : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(FoilPicsSettings)
    Q_PROPERTY(bool sharedKeyWarning READ sharedKeyWarning WRITE setSharedKeyWarning NOTIFY sharedKeyWarningChanged)
    Q_PROPERTY(bool sharedKeyWarning2 READ sharedKeyWarning2 WRITE setSharedKeyWarning2 NOTIFY sharedKeyWarning2Changed)
    Q_PROPERTY(bool autoLock READ autoLock WRITE setAutoLock NOTIFY autoLockChanged)
    Q_PROPERTY(int autoLockTime READ autoLockTime WRITE setAutoLockTime NOTIFY autoLockTimeChanged)

public:
    explicit FoilPicsSettings(QObject* aParent = Q_NULLPTR);
    ~FoilPicsSettings();

    // Callback for qmlRegisterSingletonType<FoilPicsSettings>
    static QObject* createSingleton(QQmlEngine*, QJSEngine*);

    bool sharedKeyWarning() const;
    bool sharedKeyWarning2() const;
    void setSharedKeyWarning(bool);
    void setSharedKeyWarning2(bool);

    bool autoLock() const;
    void setAutoLock(bool);

    int autoLockTime() const;
    void setAutoLockTime(int);

Q_SIGNALS:
    void sharedKeyWarningChanged();
    void sharedKeyWarning2Changed();
    void autoLockChanged();
    void autoLockTimeChanged();

private:
    class Private;
    Private* iPrivate;
};

#endif // FOILPICS_SETTINGS_H
