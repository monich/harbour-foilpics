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

#include "FileRemover.h"

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

FileRemover* FileRemover::gInstance = NULL;

FileRemover* FileRemover::instance()
{
    HASSERT(gInstance);
    return gInstance;
}

// ==========================================================================
// FileRemover::TrackerProxy
// ==========================================================================

class FileRemover::TrackerProxy: public QDBusAbstractInterface
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
// FileRemover::Private
// ==========================================================================

FileRemover::FileRemover(QObject* aParent) :
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

FileRemover::~FileRemover()
{
    HASSERT(this == gInstance);
    gInstance = NULL;
}

void FileRemover::onTrackerRegistered()
{
    HDEBUG("Tracker is here");
    delete iTrackerProxy;
    iTrackerProxy = new TrackerProxy(this);
}

void FileRemover::onTrackerUnregistered()
{
    HDEBUG("Tracker gone");
    delete iTrackerProxy;
    iTrackerProxy = NULL;
}

void FileRemover::mediaDeleted(QString aFilename)
{
    mediaDeleted(QUrl::fromLocalFile(aFilename));
}

void FileRemover::mediaDeleted(QUrl aUrl)
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

bool FileRemover::deleteMedia(QUrl aUrl)
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

bool FileRemover::deleteFile(QString aPath)
{
    if (QFile::remove(aPath)) {
        HDEBUG(aPath);
        return true;
    } else {
        HWARN("Failed to delete" << aPath);
        return false;
    }
}

#include "FileRemover.moc"
