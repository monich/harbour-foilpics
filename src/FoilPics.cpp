/*
 * Copyright (C) 2017-2022 Jolla Ltd.
 * Copyright (C) 2017-2022 Slava Monich <slava@monich.com>
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

#include "FoilPics.h"
#include "FoilPicsRole.h"

#include "HarbourBase45.h"
#include "HarbourDebug.h"

#include <QFile>

#include <QAbstractItemModel>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusServiceWatcher>
#include <QDBusAbstractInterface>
#include <QDBusConnectionInterface>

#define TRACKER_BUS QDBusConnection::sessionBus()
#define TRACKER_SERVICE "org.freedesktop.Tracker1"
#define TRACKER_PATH "/org/freedesktop/Tracker1/Resources"
#define TRACKER_INTERFACE "org.freedesktop.Tracker1.Resources"

#define FOILAPPS_DIR    "/usr/bin"
#define FOILAUTH_PATH   FOILAPPS_DIR "/harbour-foilauth"
#define FOILNOTES_PATH  FOILAPPS_DIR "/harbour-foilnotes"

// ==========================================================================
// FoilPics::TrackerProxy
// ==========================================================================

class FoilPics::TrackerProxy: public QDBusAbstractInterface
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
// FoilPics::Private
// ==========================================================================

class FoilPics::Private : public QObject {
    Q_OBJECT
public:
    Private(FoilPics* aParent);

    static bool foilAuthInstalled();
    static bool foilNotesInstalled();
    static bool otherFoilAppsInstalled();
    FoilPics* owner() const;

public Q_SLOTS:
    void onTrackerRegistered();
    void onTrackerUnregistered();
    void checkFoilAppsInstalled();

public:
    QLocale iLocale;
    TrackerProxy* iTrackerProxy;
    QFileSystemWatcher* iFileWatcher;
    bool iOtherFoilAppsInstalled;
    QString iDocumentGalleryItemQml;
    QString iDocumentGalleryModelQml;
    QString iThumbnailQml;
};

FoilPics::Private::Private(FoilPics* aParent) :
    QObject(aParent),
    iTrackerProxy(NULL),
    iFileWatcher(new QFileSystemWatcher(this))
{
    connect(iFileWatcher, SIGNAL(directoryChanged(QString)),
        SLOT(checkFoilAppsInstalled()));
    if (!iFileWatcher->addPath(FOILAPPS_DIR)) {
        HWARN("Failed to watch " FOILAPPS_DIR);
    }
    iOtherFoilAppsInstalled = otherFoilAppsInstalled();

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

inline FoilPics* FoilPics::Private::owner() const
{
    return qobject_cast<FoilPics*>(parent());
}

bool FoilPics::Private::foilAuthInstalled()
{
    const bool installed = QFile::exists(FOILAUTH_PATH);
    HDEBUG("FoilAuth is" << (installed ? "installed" : "not installed"));
    return installed;
}

bool FoilPics::Private::foilNotesInstalled()
{
    const bool installed = QFile::exists(FOILNOTES_PATH);
    HDEBUG("FoilNotes is" << (installed ? "installed" : "not installed"));
    return installed;
}

bool FoilPics::Private::otherFoilAppsInstalled()
{
    return foilAuthInstalled() || foilNotesInstalled();
}

void FoilPics::Private::checkFoilAppsInstalled()
{
    const bool haveOtherFoilApps = otherFoilAppsInstalled();
    if (iOtherFoilAppsInstalled != haveOtherFoilApps) {
        iOtherFoilAppsInstalled = haveOtherFoilApps;
        Q_EMIT owner()->otherFoilAppsInstalledChanged();
    }
}

void FoilPics::Private::onTrackerRegistered()
{
    HDEBUG("Tracker is here");
    delete iTrackerProxy;
    iTrackerProxy = new TrackerProxy(this);
}

void FoilPics::Private::onTrackerUnregistered()
{
    HDEBUG("Tracker gone");
    delete iTrackerProxy;
    iTrackerProxy = NULL;
}

// ==========================================================================
// FoilPics::Private
// ==========================================================================

FoilPics::FoilPics(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

FoilPics::~FoilPics()
{
}

QObject* FoilPics::createSingleton(QQmlEngine*, QJSEngine*)
{
    return new FoilPics;
}

bool FoilPics::otherFoilAppsInstalled() const
{
    return iPrivate->iOtherFoilAppsInstalled;
}

const QString FoilPics::documentGalleryItemQml() const
{
    if (iPrivate->iDocumentGalleryItemQml.isEmpty()) {
        // import Sailfish.Silica 1.0;import QtDocGallery 5.0;DocumentGalleryItem{
        //   readonly property int statusFinished:DocumentGalleryItem.Finished;
        //   autoUpdate:false;properties:["filePath","fileSize","mimeType","width","height",
        //   "dateTaken","cameraManufacturer","cameraModel","latitude","longitude","altitude"]}
        static const char base45[] =
            "YEDS9E5LE+347ECUVD+EDU7DNOAXVDGPC634Y$5:M7Q$D04ESUEHDAET8ZOCAEC"
            "TVDALEA34Y$57M7U3E1%E1$CMVEAECTVDALE*C90$CCRFX CGVC8/D3EFV9E/3E"
            "5$CRWEH44G/DR443WENWEZPEZED+ED+8D8UCET8 PCF$DG/D319 VD5$C:EF7WE"
            "5 DP+85/DOQE  C M78%EG3EH9EIEC*ZCJ.C/VD+ZCV9E/3E5$CBWE6$C:G7VF4"
            "XEDG CIECI7D3Q5R.CTVDNOADLFIE4%F4YEDK C%FFFZC3Q5E4FLVCI7D3Q5+8D"
            "SEDA9DIE4TF4IECK C9EC1$CIE4SF4BEC5$CODCCEC*$E1ECNWEZKEDJE3Q5GPC"
            "F$DVKENZ96VC7UD3Q5PVDBWENWE6VCIE4$F4-3ED3DNWE6VCIE4QF4:VD-ED$$E"
            "FZC% B";
        iPrivate->iDocumentGalleryItemQml = HarbourBase45::fromBase45
            (QString::fromLatin1(base45));
        HDEBUG(iPrivate->iDocumentGalleryItemQml);
    }
    return iPrivate->iDocumentGalleryItemQml;
}

const QString FoilPics::documentGalleryModelQml() const
{
    if (iPrivate->iDocumentGalleryModelQml.isEmpty()) {
        // import Sailfish.Silica 1.0;import QtDocGallery 5.0;DocumentGalleryModel{
        //   rootType:DocumentGallery.Image;
        //   properties:["url","mimeType","title","orientation","dateTaken","width","height",
        //   "cameraManufacturer","cameraModel","latitude","longitude","altitude"];
        //   sortProperties:["-dateTaken"];autoUpdate:true;
        //   filter:GalleryStartsWithFilter{property:"filePath";value:StandardPaths.music;negated:true}}
        static const char base45[] =
            "YEDS9E5LE+347ECUVD+EDU7DNOAXVDGPC634Y$5:M7Q$D04ESUEHDAET8ZOCAEC"
            "TVDALEA34Y$57M7U3E1%E1$CMVEAECTVDALENZ96VC6WD0LE24EPUAI9ELG7U3E"
            "1%E1$CMVEAECTVDALEE%5B$D93D6N70LEI9E5LEQEDNPEKNB6%E7UD3Q5J$DF$D"
            "PUAI9EIE40G4-EDTVDIE4+F4*KE1$C3WEBWE-3EIE4TF4IECK C9EC1$CIE43G4"
            "PEDAWEIE4XF4- CC3DUUE3Q5GPCF$DVKE9Z9H/DJ.CZPC6%E5$CIE4SF4BEC5$C"
            "ODCV3E: CIE4$F4IEC-ED$$EFZC3Q5*VD3/D-ED$$EFZC3Q5AECBWENWE6VCMF4"
            "9N704EVVE0LEI9E5LEQEDNPEKNBAX5IECK C9EC1$CMF4 M78%EG3EH9EIEC*ZC"
            "KWE%$E-M7XED7WE$JE319 VD5$C9FF3WE5LE7QE-ED98DXED7WECLEV9E/3E5$C"
            "RWEWF7R.CTVDC7AAWEXE4K/E0WD*ZCYOACEC2VCYKEC7AAWEBPEV$DPQENOC1/D"
            "53D7WE8UCKWE%$E+%F";
        iPrivate->iDocumentGalleryModelQml = HarbourBase45::fromBase45
            (QString::fromLatin1(base45));
        HDEBUG(iPrivate->iDocumentGalleryModelQml);
    }
    return iPrivate->iDocumentGalleryModelQml;
}

const QString FoilPics::thumbnailQml() const
{
    if (iPrivate->iThumbnailQml.isEmpty()) {
        // import org.nemomobile.thumbnailer 1.0;Thumbnail{
        //   readonly property int normalPriority:Thumbnail.NormalPriority;
        //   readonly property int highPriority:Thumbnail.HighPriority;
        //   readonly property int lowPriority:Thumbnail.LowPriority;
        //   readonly property int errorStatus:Thumbnail.Error;
        //   priority: Thumbnail.NormalPriority;
        //   anchors.fill:parent}
        static const char base45[] =
            "YEDS9E5LEN44$KE6*50$C+3ET3EXEDRZCAWE1%E/JC7ECTVDBJEZ96H468UA1%E"
            "/JC7EC6WDZKE2EC-3E4WDO440LEI9E5LE3EFZEDSUEB/D/KEAECT7A ED*KERWE"
            "$G7B9DC$D..DXEDJ%504EB$D8VD*KE04E-EDUEFZKE2EC-3E4WDO440LEI9E5LE"
            "3EFZEDSUE:8DC3DT7A ED*KERWE$G7B9DC$D..DXEDD%5SEDJ8D*KE04E-EDUEF"
            "ZKE2EC-3E4WDO440LEI9E5LE3EFZEDSUE*VDY3F*KE04E-EDTEF8UA1%E/JC7EC"
            "JUD$T9Y3F*KE04E-EDUEFZKE2EC-3E4WDO440LEI9E5LE3EFZEDSUE5$C0LEHKE"
            "3WENWENPE8UA1%E/JC7ECJUD3Z80LE%JEV9E ED*KERWEUF78UA1%E/JC7ECJUD"
            "9-9/KEAECT7A ED*KERWE M7:.D59D4LE/%5XEDVUDE9EZKEG/DZ2";
        iPrivate->iThumbnailQml = HarbourBase45::fromBase45
            (QString::fromLatin1(base45));
        HDEBUG(iPrivate->iThumbnailQml);
    }
    return iPrivate->iThumbnailQml;
}

QString FoilPics::formatFileSize(qlonglong aBytes)
{
    const qlonglong kB = Q_INT64_C(1024);
    const qlonglong MB = kB*1024;
    const qlonglong GB = MB*1024;
    const qlonglong TB = GB*1024;
    if (aBytes < 0) {
        return QString::number(aBytes);
    } else if (aBytes < kB) {
        return qtTrId("foilpics-file_size-bytes").arg(aBytes);
    } else if (aBytes < 1000*kB) {
        const int precision = (aBytes < 10*kB) ? 2 : 1;
        return qtTrId("foilpics-file_size-kilobytes").
            arg(iPrivate->iLocale.toString((float)aBytes/kB, 'f', precision));
    } else if (aBytes < 1000*MB) {
        const int precision = (aBytes < 10*MB) ? 2 : 1;
        return qtTrId("foilpics-file_size-megabytes").
            arg(iPrivate->iLocale.toString((float)aBytes/MB, 'f', precision));
    } else if (aBytes < 1000*GB) {
        const int precision = (aBytes < 10*GB) ? 2 : 1;
        return qtTrId("foilpics-file_size-gigabytes").
            arg(iPrivate->iLocale.toString((float)aBytes/GB, 'f', precision));
    } else {
        return qtTrId("foilpics-file_size-terabytes").
            arg(iPrivate->iLocale.toString((float)aBytes/TB, 'f', 1));
    }
}

void FoilPics::mediaDeleted(QUrl aUrl)
{
    if (iPrivate->iTrackerProxy) {
        QString url(aUrl.toString());
        HDEBUG(url);
        // Black magic
        iPrivate->iTrackerProxy->SparqlUpdate("DELETE {\n"
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

bool FoilPics::deleteLocalFile(QUrl aUrl)
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

void FoilPics::deleteLocalFilesFromModel(QObject* aModel,
    QString aRole, QList<int> aRows)
{
    HDEBUG(aRows);
    const int n = aRows.count();
    if (n > 0) {
        QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(aModel);
        HASSERT(model);
        if (model) {
            const int role = FoilPicsRole::find(model, aRole);
            if (role >= 0) {
                int i;
                const int column = 0;
                const int rowCount = model->rowCount();
                QList<QUrl> urls;
                urls.reserve(n);
                for (i = 0; i < n; i++) {
                    const int row = aRows.at(i);
                    if (row >= 0 && row < rowCount) {
                        QModelIndex index(model->index(row, column));
                        QUrl url(model->data(index, role).toUrl());
                        if (url.isValid()) {
                            HDEBUG(url);
                            urls.append(url);
                        }
                    }
                }
                const int urlCount = urls.count();
                for (i = 0; i < urlCount; i++) {
                    deleteLocalFile(urls.at(i));
                }
            }
        }
    }
}

bool FoilPics::deleteFile(QString aPath)
{
    if (QFile::remove(aPath)) {
        HDEBUG(aPath);
        return true;
    } else {
        HWARN("Failed to delete" << aPath);
        return false;
    }
}

#include "FoilPics.moc"
