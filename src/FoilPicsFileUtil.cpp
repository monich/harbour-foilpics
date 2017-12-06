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

#include "FoilPicsFileUtil.h"

#include "HarbourDebug.h"

#include <QFile>

#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusServiceWatcher>
#include <QDBusAbstractInterface>
#include <QDBusConnectionInterface>

#define TRACKER_BUS QDBusConnection::sessionBus()
#define TRACKER_SERVICE "org.freedesktop.Tracker1"
#define TRACKER_PATH "/org/freedesktop/Tracker1/Resources"
#define TRACKER_INTERFACE "org.freedesktop.Tracker1.Resources"

FoilPicsFileUtil* FoilPicsFileUtil::gInstance = NULL;

FoilPicsFileUtil* FoilPicsFileUtil::instance()
{
    HASSERT(gInstance);
    return gInstance;
}

// ==========================================================================
// FoilPicsFileUtil::TrackerProxy
// ==========================================================================

class FoilPicsFileUtil::TrackerProxy: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    TrackerProxy(QObject* aParent) :
        QDBusAbstractInterface(TRACKER_SERVICE, TRACKER_PATH,
            TRACKER_INTERFACE, TRACKER_BUS, aParent) {}

public Q_SLOTS: // METHODS
    QDBusPendingCall SparqlUpdate(QString aStatement)
        { return asyncCall("SparqlUpdate", aStatement); }
};

// ==========================================================================
// FoilPicsFileUtil::Private
// ==========================================================================

FoilPicsFileUtil::FoilPicsFileUtil(QObject* aParent) :
    QObject(aParent),
    iTrackerProxy(NULL)
{
    HASSERT(!gInstance);
    gInstance = this;

    QDBusServiceWatcher* trackerWatcher =
        new QDBusServiceWatcher(TRACKER_SERVICE,
            TRACKER_BUS, QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);

    connect(trackerWatcher, SIGNAL(serviceRegistered(QString)),
        this, SLOT(onTrackerRegistered()));
    connect(trackerWatcher, SIGNAL(serviceUnregistered(QString)),
        this, SLOT(onTrackerUnregistered()));

    if (TRACKER_BUS.interface()->isServiceRegistered(TRACKER_SERVICE)) {
        onTrackerRegistered();
    }
}

FoilPicsFileUtil::~FoilPicsFileUtil()
{
    HASSERT(this == gInstance);
    gInstance = NULL;
}

void FoilPicsFileUtil::onTrackerRegistered()
{
    HDEBUG("Tracker is here");
    delete iTrackerProxy;
    iTrackerProxy = new TrackerProxy(this);
}

void FoilPicsFileUtil::onTrackerUnregistered()
{
    HDEBUG("Tracker gone");
    delete iTrackerProxy;
    iTrackerProxy = NULL;
}

void FoilPicsFileUtil::mediaDeleted(QString aFilename)
{
    mediaDeleted(QUrl::fromLocalFile(aFilename));
}

QString FoilPicsFileUtil::formatFileSize(qlonglong aBytes)
{
    const qlonglong kB = Q_INT64_C(1024);
    const qlonglong MB = kB*1024;
    const qlonglong GB = MB*1024;
    const qlonglong TB = GB*1024;
    if (aBytes < 0) {
        return QString::number(aBytes);
    } else if (aBytes < kB) {
        return qtTrId("foilpics-file_size-bytes", aBytes);
    } else if (aBytes < 1000*kB) {
        const int precision = (aBytes < 10*kB) ? 2 : 1;
        return qtTrId("foilpics-file_size-kilobytes").
            arg(iLocale.toString((float)aBytes/kB, 'f', precision));
    } else if (aBytes < 1000*MB) {
        const int precision = (aBytes < 10*MB) ? 2 : 1;
        return qtTrId("foilpics-file_size-megabytes").
            arg(iLocale.toString((float)aBytes/MB, 'f', precision));
    } else if (aBytes < 1000*GB) {
        const int precision = (aBytes < 10*GB) ? 2 : 1;
        return qtTrId("foilpics-file_size-gigabytes").
            arg(iLocale.toString((float)aBytes/GB, 'f', precision));
    } else {
        return qtTrId("foilpics-file_size-terabytes").
            arg(iLocale.toString((float)aBytes/TB, 'f', 1));
    }
}

void FoilPicsFileUtil::mediaDeleted(QUrl aUrl)
{
    if (iTrackerProxy) {
        QString url(aUrl.toString());
        HDEBUG(url);
        // Black magic
        iTrackerProxy->SparqlUpdate("DELETE {\n"
            "   ?y nfo:hasMediaFileListEntry ?x\n"
            "   ?x a nfo:MediaFileListEntry\n"
            "} WHERE {\n"
            "   {?y nfo:hasMediaFileListEntry ?x}\n"
            "   {?x nfo:entryUrl '" + url + "'}\n"
            "}"
            " DELETE {\n"
            "   ?x a nfo:Media\n"
            "} WHERE {\n"
            "   {?x nie:url '" + url + "'}\n"
            "}");
    }
}

bool FoilPicsFileUtil::deleteMedia(QUrl aUrl)
{
    if (aUrl.isLocalFile()) {
        QString path(aUrl.toLocalFile());
        if (QFile::remove(path)) {
            HDEBUG(path);
            mediaDeleted(aUrl);
        } else {
            HWARN("Failed to delete" << path);
        }
    } else {
        HWARN(aUrl << "is not a local file");
    }
    return false;
}

bool FoilPicsFileUtil::deleteFile(QString aPath)
{
    if (QFile::remove(aPath)) {
        HDEBUG(aPath);
        return true;
    } else {
        HWARN("Failed to delete" << aPath);
        return false;
    }
}

#include "FoilPicsFileUtil.moc"
