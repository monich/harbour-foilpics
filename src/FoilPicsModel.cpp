/*
 * Copyright (C) 2017-2022 Jolla Ltd.
 * Copyright (C) 2017-2022 Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of BSD license as follows:
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

#include "FoilPicsModel.h"
#include "FoilPics.h"
#include "FoilPicsImageProvider.h"
#include "FoilPicsGroupModel.h"
#include "FoilPicsRole.h"
#include "FoilPicsThumbnailProvider.h"

#include "foil_private_key.h"
#include "foil_digest.h"
#include "foil_output.h"
#include "foil_random.h"
#include "foil_util.h"
#include "foilmsg.h"

#include "gutil_misc.h"

#include "HarbourDebug.h"
#include "HarbourParentSignalQueueObject.h"
#include "HarbourTask.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtGui/QImage>

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#define ENCRYPT_KEY_TYPE FOILMSG_KEY_AES_256
#define DIGEST_TYPE FOIL_DIGEST_MD5

#define HEADER_ORIGINAL_PATH        "Original-Path"
#define HEADER_ORIGINAL_SIZE        "Original-Size"
#define HEADER_MODIFICATION_TIME    "Modification-Time"
#define HEADER_ACCESS_TIME          "Access-Time"
#define HEADER_ORIENTATION          "Orientation"
#define HEADER_CAMERA_MANUFACTURER  "Camera-Manufacturer"
#define HEADER_CAMERA_MODEL         "Camera-Model"
#define HEADER_IMAGE_DATE           "Image-Date"
#define HEADER_LATITUDE             "Latitude"
#define HEADER_LONGITUDE            "Longitude"
#define HEADER_ALTITUDE             "Altitude"
#define HEADER_TITLE                "Title"
#define HEADER_GROUP                "Group"

// Thumbnail specific headers
#define HEADER_THUMB_FULL_WIDTH     "Full-Width"
#define HEADER_THUMB_FULL_HEIGHT    "Full-Height"

#define INFO_FILE ".info"
#define INFO_CONTENTS "FoilPics"
#define INFO_ORDER_HEADER "Order"
#define INFO_ORDER_DELIMITER   ','
#define INFO_ORDER_DELIMITER_S ","
#define INFO_ORDER_THUMB_DELIMITER ':'
#define INFO_GROUPS_HEADER "Groups"
#define INFO_COLLAPSED_HEADER "Collapsed"
#define INFO_COLLAPSED_DELIMITER INFO_ORDER_DELIMITER
#define INFO_COLLAPSED_DELIMITER_S INFO_ORDER_DELIMITER_S

// Directories relative to home
#define FOIL_PICS_DIR               "Documents/FoilPics"
#define FOIL_KEY_DIR                ".local/share/foil"
#define FOIL_KEY_FILE               "foil.key"

// Keys for metadata passed to encryptFile:
const QString FoilPicsModel::MetaUrl("url");                 // QUrl
const QString FoilPicsModel::MetaOrientation("orientation"); // int
const QString FoilPicsModel::MetaImageDate("imageDate");     // QDateTime
const QString FoilPicsModel::MetaCameraManufacturer("cameraManufacturer"); // QString
const QString FoilPicsModel::MetaCameraModel("cameraModel"); // QString
const QString FoilPicsModel::MetaLatitude("latitude");       // double
const QString FoilPicsModel::MetaLongitude("longitude");     // double
const QString FoilPicsModel::MetaAltitude("altitude");       // double

// Role names
#define FOILPICS_ROLES(role) \
    role(Url, url) \
    role(ImageId, imageId) \
    role(OriginalFileSize, originalFileSize) \
    role(EncryptedFileSize, encryptedFileSize) \
    role(Thumbnail, thumbnail) \
    role(Orientation, orientation) \
    role(CameraManufacturer, cameraManufacturer) \
    role(CameraModel, cameraModel) \
    role(Latitude, latitude) \
    role(Longitude, longitude) \
    role(Altitude, altitude) \
    role(ImageDate, imageDate) \
    role(MimeType, mimeType) \
    role(FileName, fileName) \
    role(OriginalPath, originalPath) \
    role(Title, title) \
    role(DefaultTitle, defaultTitle) \
    role(ImageWidth, imageWidth) \
    role(ImageHeight, imageHeight) \
    role(GroupId, groupId) \
    role(GroupName, groupName)

// ==========================================================================
// FoilPicsModel::ModelData
// ==========================================================================

class FoilPicsModel::ModelData {
public:
    enum Role {
        FirstRole = Qt::UserRole,
        #define ROLE(X,x) X##Role,
        FOILPICS_ROLES(ROLE)
        #undef ROLE
        LastRole
    };

    #define ROLE(X,x) static const QString RoleName##X;
    FOILPICS_ROLES(ROLE)
    #undef ROLE

    typedef QList<ModelData*> List;
    typedef List::ConstIterator ConstIterator;
    struct FormatMap {
        const char* contentType;
        const char* imageFormat;
    };

    ModelData(const QString&, int, const QSize&, GBytes*, const QString&,
        const QString&, const QImage&, const QString&, const char*,
        const QDateTime&, int, const QString&, const QString&, const char*,
        const char*, const char*, const QDateTime&, const char*);
    ~ModelData();

    QVariant get(Role) const;
    QVariantMap toVariantMap() const;
    void updateVariantMap(Role);

    static QString defaultTitle(const QString&);
    static QString defaultTitle(const QFileInfo&);
    static QImage thumbnail(const QImage&, const QSize&, int);
    static const char* format(const char*);
    static int compareFormatMap(const void*, const void*);

    static double* toDouble(const char*);
    static QString headerString(const FoilMsg*, const char*);
    static QDateTime headerTime(const FoilMsg*, const char*);
    static QDateTime headerSortTime(const FoilMsg*);
    static int headerInt(const FoilMsg*, const char*, int aDef = 0);

    static ModelData* fromFoilMsg(FoilMsg*, const QString&, const QSize&,
        const QString&, const QString&, const QImage&, const char*, int);

public:
    QString iPath;
    QString iOriginalPath;
    QString iFileName;
    QString iThumbFile; // Without path
    QString iDefaultTitle;
    QString iTitle;
    QByteArray iGroupId;
    QSize iFullDimensions;
    QImage iThumbnail;
    QString iThumbSource;
    QString iImageId;
    QString iImageSource;
    QString iContentType;
    QDateTime iSortTime;
    int iEncryptedSize;
    int iOriginalSize;
    int iOrientation;
    QString iCameraManufacturer;
    QString iCameraModel;
    QDateTime iImageDate;
    QByteArray iBytes;
    double* iLatitude;
    double* iLongitude;
    double* iAltitude;
    HarbourTask* iDecryptTask;
    HarbourTask* iSetTitleTask;
    HarbourTask* iSetGroupTask;
    mutable QVariantMap iVariantMap;
};

#define ROLE(X,x) const QString FoilPicsModel::ModelData::RoleName##X(#x);
FOILPICS_ROLES(ROLE)
#undef ROLE

FoilPicsModel::ModelData::ModelData(
    const QString& aOriginalPath,
    int aOriginalSize,
    const QSize& aFullDimensions,
    GBytes* aDigest,
    const QString& aPath,
    const QString& aThumbFile,
    const QImage& aThumbImage,
    const QString& aTitle,
    const char* aContentType,
    const QDateTime& aSortTime,
    int aOrientation,
    const QString& aCameraManufacturer,
    const QString& aCameraModel,
    const char* aLatitude,
    const char* aLongitude,
    const char* aAltitude,
    const QDateTime& aImageDate,
    const char* aGroupId) :
    iPath(aPath),
    iThumbFile(aThumbFile),
    iTitle(aTitle),
    iFullDimensions(aFullDimensions),
    iThumbnail(aThumbImage),
    iSortTime(aSortTime),
    iOriginalSize(aOriginalSize),
    iOrientation(aOrientation),
    iCameraManufacturer(aCameraManufacturer),
    iCameraModel(aCameraModel),
    iImageDate(aImageDate),
    iLatitude(toDouble(aLatitude)),
    iLongitude(toDouble(aLongitude)),
    iAltitude(toDouble(aAltitude)),
    iDecryptTask(Q_NULLPTR),
    iSetTitleTask(Q_NULLPTR),
    iSetGroupTask(Q_NULLPTR)
{
    QFileInfo fileInfo(aOriginalPath);
    iOriginalPath = fileInfo.absoluteFilePath();
    iFileName = fileInfo.fileName();
    iEncryptedSize = QFileInfo(aPath).size();
    iDefaultTitle = defaultTitle(fileInfo);
    if (iTitle.isEmpty()) iTitle = iDefaultTitle;
    if (aContentType) iContentType = QLatin1String(aContentType);
    if (aGroupId && aGroupId[0]) iGroupId = QByteArray(aGroupId);

    // General image id from the digest
    gsize digestSize;
    const uchar* digest = (uchar*)g_bytes_get_data(aDigest, &digestSize);
    GString* buf = g_string_sized_new(digestSize*2);
    for (guint i = 0; i < digestSize; i++) {
        g_string_append_printf(buf, "%02X", digest[i]);
    }
    iImageId = QLatin1String(buf->str);
    HDEBUG(iFileName << buf->str << iOrientation);
    g_string_free(buf, TRUE);
}

FoilPicsModel::ModelData::~ModelData()
{
    if (iDecryptTask) iDecryptTask->release();
    if (iSetTitleTask) iSetTitleTask->release();
    if (iSetGroupTask) iSetGroupTask->release();
    delete iLatitude;
    delete iLongitude;
    delete iAltitude;
}

/* static */
FoilPicsModel::ModelData*
FoilPicsModel::ModelData::fromFoilMsg(
    FoilMsg* aMsg,
    const QString& aOriginalPath,
    const QSize& aFullDimensions,
    const QString& aPath,
    const QString& aThumbFile,
    const QImage& aThumbImage,
    const char* aContentType,
    int aOrientation)
{
    GBytes* digest = foil_digest_bytes(DIGEST_TYPE, aMsg->data);
    ModelData* data = new ModelData(aOriginalPath,
        headerInt(aMsg, HEADER_ORIGINAL_SIZE), aFullDimensions,
        digest, aPath, aThumbFile, aThumbImage,
        headerString(aMsg, HEADER_TITLE), aContentType,
        headerSortTime(aMsg), aOrientation,
        headerString(aMsg, HEADER_CAMERA_MANUFACTURER),
        headerString(aMsg, HEADER_CAMERA_MODEL),
        foilmsg_get_value(aMsg, HEADER_LATITUDE),
        foilmsg_get_value(aMsg, HEADER_LONGITUDE),
        foilmsg_get_value(aMsg, HEADER_ALTITUDE),
        headerTime(aMsg, HEADER_IMAGE_DATE),
        foilmsg_get_value(aMsg, HEADER_GROUP));

    g_bytes_unref(digest);
    return data;
}

QVariantMap
FoilPicsModel::ModelData::toVariantMap() const
{
    // Fill the variant map (groupName renames empty)
    if (iVariantMap.isEmpty()) {
        #define ROLE(X,x) iVariantMap.insert(RoleName##X, get(X##Role));
        FOILPICS_ROLES(ROLE)
        #undef ROLE
    }
    return iVariantMap;
}

void
FoilPicsModel::ModelData::updateVariantMap(
    Role aRole)
{
    if (!iVariantMap.isEmpty()) {
        switch (aRole) {
        #define ROLE(X,x) case X##Role: \
            iVariantMap.insert(RoleName##X, get(X##Role)); break;
        FOILPICS_ROLES(ROLE)
        #undef ROLE
        case ModelData::FirstRole:
        case ModelData::LastRole:
            break;
        }
    }
}

QVariant
FoilPicsModel::ModelData::get(
    Role aRole) const
{
    switch (aRole) {
    case UrlRole: return iImageSource;
    case ImageIdRole: return iImageId;
    case OriginalFileSizeRole: return iOriginalSize ?
            QVariant::fromValue(iOriginalSize) : QVariant();
    case EncryptedFileSizeRole: return iEncryptedSize ?
            QVariant::fromValue(iEncryptedSize) : QVariant();
    case ThumbnailRole: return iThumbSource;
    case OrientationRole: return iOrientation;
    case CameraManufacturerRole: return iCameraManufacturer;
    case CameraModelRole: return iCameraModel;
    case LatitudeRole: return iLatitude ?
            QVariant::fromValue(*iLatitude) : QVariant();
    case LongitudeRole: return iLongitude ?
            QVariant::fromValue(*iLongitude) : QVariant();
    case AltitudeRole: return iAltitude ?
            QVariant::fromValue(*iAltitude) : QVariant();
    case ImageDateRole: return iImageDate.isValid() ?
            QVariant::fromValue(iImageDate) : QVariant();
    case MimeTypeRole: return iContentType;
    case TitleRole: return iTitle;
    case DefaultTitleRole: return iDefaultTitle;
    case FileNameRole: return iFileName;
    case OriginalPathRole: return iOriginalPath;
    case ImageWidthRole: return iFullDimensions.width();
    case ImageHeightRole: return iFullDimensions.height();
    case GroupIdRole: return iGroupId.isEmpty() ?
        QString() : QString::fromLatin1(iGroupId);
    // Group name is maintained by FoilPicsGroupModel and is returned
    // by FoilPicsModel. Ignore it here.
    case GroupNameRole:
    // No default to make sure that we get "warning: enumeration value
    // not handled in switch" if we forget to handle a real role.
    case FirstRole:
    case LastRole:
        break;
    }
    return QVariant();
}

/* static */
double*
FoilPicsModel::ModelData::toDouble(
    const char* aString)
{
    if (aString && aString[0]) {
        const double q = g_ascii_strtod(aString, Q_NULLPTR);

        if (!errno) {
            return new double(q);
        }
    }
    return Q_NULLPTR;
}

/* static */
QString
FoilPicsModel::ModelData::defaultTitle(
    const QString& aPath)
{
    return defaultTitle(QFileInfo(aPath));
}

/* static */
QString
FoilPicsModel::ModelData::defaultTitle(
    const QFileInfo& aFileInfo)
{
    return aFileInfo.baseName();
}

/* static */
QImage
FoilPicsModel::ModelData::thumbnail(
    const QImage& aImage,
    const QSize& aSize,
    int aRotate)
{
    QImage cropped;
    const QSize imageSize(aImage.size());
    const Qt::TransformationMode txMode(Qt::SmoothTransformation);

    if (imageSize.width()*aSize.height() > aSize.width()*imageSize.height()) {
        const QImage scaled(aImage.scaledToHeight(aSize.height(), txMode));
        const int x = (scaled.width() - aSize.width())/2;

        cropped = scaled.copy(x, 0, aSize.width(), aSize.height());
    } else {
        const QImage scaled(aImage.scaledToWidth(aSize.width(), txMode));
        const int y = (scaled.height() - aSize.height())/2;

        cropped = scaled.copy(0, y, aSize.width(), aSize.height());
    }
    if (aRotate) {
        const qreal x = ((qreal)aSize.width())/2;
        const qreal y = ((qreal)aSize.height())/2;

        return cropped.transformed(QTransform::fromTranslate(x, y).
            rotate(-aRotate).translate(-x, -y));
    } else {
        return cropped;
    }
}

/* static */
const char*
FoilPicsModel::ModelData::format(
    const char* aContentType)
{
    static const struct FormatMap {
        const char* contentType;
        const char* imageFormat;
    } formatMap[] = { /* Sorted */
        { "image/bmp", "BMP" },
        { "image/gif", "GIF" },
        { "image/jpeg", "JPEG" },
        { "image/jpg", "JPEG" },
        { "image/png", "PNG" },
        { "image/svg+xml", "SVG" },
        { "image/tif", "TIFF" },
        { "image/tiff", "TIFF" },
        { "image/x-bmp", "BMP" },
        { "image/x-portable-bitmap", "PBM" },
        { "image/x-portable-graymap", "PGM" },
        { "image/x-portable-pixmap", "PPM" }
    };

    if (aContentType && aContentType[0]) {
        FormatMap key;

        key.contentType = aContentType;
        key.imageFormat = Q_NULLPTR;

        const FormatMap* res = (const FormatMap*)bsearch(&key, formatMap,
            G_N_ELEMENTS(formatMap), sizeof(formatMap[0]), compareFormatMap);

        if (res) {
            return res->imageFormat;
        }
        HDEBUG("Unknown content type" << aContentType);
    }
    return Q_NULLPTR;
}

/* static */
int
FoilPicsModel::ModelData::compareFormatMap(
    const void* aElem1,
    const void* aElem2)
{
    return strcmp(((const FormatMap*)aElem1)->contentType,
        ((const FormatMap*)aElem2)->contentType);
}

/* static */
int
FoilPicsModel::ModelData::headerInt(
    const FoilMsg* aMsg,
    const char* aKey,
    int aDefaultValue)
{
    int result = aDefaultValue;

    gutil_parse_int(foilmsg_get_value(aMsg, aKey), 0, &result);
    return result;
}

/* static */
QString
FoilPicsModel::ModelData::headerString(
    const FoilMsg* aMsg,
    const char* aKey)
{
    const char* value = foilmsg_get_value(aMsg, aKey);

    return (value && value[0]) ? QString(value) : QString();
}

/* static */
QDateTime
FoilPicsModel::ModelData::headerTime(
    const FoilMsg* aMsg,
    const char* aKey)
{
    const char* value = foilmsg_get_value(aMsg, aKey);

    return value ? QDateTime::fromString(value, Qt::ISODate) : QDateTime();
}

/* static */
QDateTime
FoilPicsModel::ModelData::headerSortTime(
    const FoilMsg* aMsg)
{
    QDateTime time = headerTime(aMsg, HEADER_IMAGE_DATE);

    return time.isValid() ? time : headerTime(aMsg, HEADER_MODIFICATION_TIME);
}

// ==========================================================================
// FoilPicsModel::ModelInfo
// ==========================================================================

class FoilPicsModel::ModelInfo
{
public:
    ModelInfo() {}
    ModelInfo(const FoilMsg*);
    ModelInfo(const ModelInfo&);
    ModelInfo(const ModelData::List&, const FoilPicsGroupModel::GroupList&);
    ModelInfo& operator=(const ModelInfo&);

    static ModelInfo load(const QString&, FoilPrivateKey*, FoilKey*);
    void save(const QString&, FoilPrivateKey*, FoilKey*) const;

public:
    QStringList iOrder;
    QHash<QString,QString> iThumbMap;
    FoilPicsGroupModel::GroupList iGroups;
};

FoilPicsModel::ModelInfo::ModelInfo(
    const ModelInfo& aInfo) :
    iOrder(aInfo.iOrder),
    iThumbMap(aInfo.iThumbMap),
    iGroups(aInfo.iGroups)
{
}

FoilPicsModel::ModelInfo&
FoilPicsModel::ModelInfo::operator=(
    const ModelInfo& aInfo)
{
    iOrder = aInfo.iOrder;
    iThumbMap = aInfo.iThumbMap;
    iGroups = aInfo.iGroups;
    return *this;
}

FoilPicsModel::ModelInfo::ModelInfo(
    const ModelData::List& aData,
    const FoilPicsGroupModel::GroupList& aGroups) :
    iGroups(aGroups)
{
    const int n = aData.count();

    iOrder.reserve(n);
    for (int i = 0; i < n; i++) {
        const ModelData* data = aData.at(i);
        QString name(QFileInfo(data->iPath).fileName());

        iOrder.append(name);
        if (!data->iThumbFile.isEmpty()) {
            iThumbMap.insert(name, data->iThumbFile);
        }
    }
}

FoilPicsModel::ModelInfo::ModelInfo(
    const FoilMsg* msg)
{
    const char* order = foilmsg_get_value(msg, INFO_ORDER_HEADER);

    if (order) {
        char** strv = g_strsplit(order, INFO_ORDER_DELIMITER_S, -1);

        for (char** ptr = strv; *ptr; ptr++) {
            char* name = g_strstrip(*ptr);

            if (name[0]) {
                const char* d = strchr(name, INFO_ORDER_THUMB_DELIMITER);

                if (d) {
                    QString img(QLatin1String(name, d - name));
                    QString thumb(QLatin1String(d + 1));

                    iOrder.append(img);
                    iThumbMap.insert(img, thumb);
                } else {
                    iOrder.append(name);
                }
            }
        }
        g_strfreev(strv);
        HDEBUG(order);
    }

    const char* groups = foilmsg_get_value(msg, INFO_GROUPS_HEADER);

    if (groups) {
        const char* collapsed = foilmsg_get_value(msg, INFO_COLLAPSED_HEADER);

#if HARBOUR_DEBUG
        HDEBUG(INFO_GROUPS_HEADER << groups);
        if (collapsed) {
            HDEBUG(INFO_COLLAPSED_HEADER << collapsed);
        }
#endif // HARBOUR_DEBUG
        iGroups = FoilPicsGroupModel::Group::decodeList(groups, collapsed);
    }
}

/* static */
FoilPicsModel::ModelInfo
FoilPicsModel::ModelInfo::load(
    const QString& aDir,
    FoilPrivateKey* aPrivate,
    FoilKey* aPublic)
{
    ModelInfo info;
    QString fullPath(aDir + "/" INFO_FILE);
    const QByteArray path(fullPath.toUtf8());
    const char* fname = path.constData();

    HDEBUG("Loading" << fname);

    FoilMsg* msg = foilmsg_decrypt_file(aPrivate, fname, Q_NULLPTR);

    if (msg) {
        if (foilmsg_verify(msg, aPublic)) {
            info = ModelInfo(msg);
        } else {
            HWARN("Could not verify" << fname);
        }
        foilmsg_free(msg);
    }
    return info;
}

void
FoilPicsModel::ModelInfo::save(
    const QString& aDir,
    FoilPrivateKey* aPrivate,
    FoilKey* aPublic) const
{
    QString fullPath(aDir + "/" INFO_FILE);
    const QByteArray path(fullPath.toUtf8());
    const char* fname = path.constData();
    FoilOutput* out = foil_output_file_new_open(fname);

    if (out) {
        QString buf;
        const int n = iOrder.count();

        for (int i = 0; i < n; i++) {
            const QString img(iOrder.at(i));
            const QString thumb(iThumbMap.value(img));

            if (!buf.isEmpty()) buf += QChar(INFO_ORDER_DELIMITER);
            buf += img;
            if (!thumb.isEmpty()) {
                buf += QChar(INFO_ORDER_THUMB_DELIMITER);
                buf += thumb;
            }
        }

        const QByteArray order(buf.toUtf8());
        const FoilPicsGroupModel::GroupInfo groupInfo(FoilPicsGroupModel::Group::encodeList(iGroups));
        const QByteArray groups(groupInfo.first);
        const QByteArray collapsed(groupInfo.second);

        FoilMsgHeaders headers;
        FoilMsgHeader header[3];
        headers.header = header;
        headers.count = 0;

        HDEBUG("Saving" << fname);
        HDEBUG(INFO_ORDER_HEADER ":" << order.constData());
        header[headers.count].name = INFO_ORDER_HEADER;
        header[headers.count].value = order.constData();
        headers.count++;

        HDEBUG(INFO_GROUPS_HEADER ":" << groups.constData());
        header[headers.count].name = INFO_GROUPS_HEADER;
        header[headers.count].value = groups.constData();
        headers.count++;

        if (!collapsed.isEmpty()) {
            HDEBUG(INFO_COLLAPSED_HEADER ":" << collapsed.constData());
            header[headers.count].name = INFO_COLLAPSED_HEADER;
            header[headers.count].value = collapsed.constData();
            headers.count++;
        }

        FoilMsgEncryptOptions opt;
        memset(&opt, 0, sizeof(opt));
        opt.key_type = ENCRYPT_KEY_TYPE;

        FoilBytes data;
        foil_bytes_from_string(&data, INFO_CONTENTS);
        foilmsg_encrypt(out, &data, Q_NULLPTR, &headers, aPrivate, aPublic,
            &opt, Q_NULLPTR);
        foil_output_unref(out);
    } else {
        HWARN("Failed to open" << fname);
    }
}

// ==========================================================================
// FoilPicsModel::Task
// ==========================================================================

class FoilPicsModel::Task :
    public HarbourTask
{
    Q_OBJECT

public:
    class Encrypt;
    class Decrypt;
    class GenerateKey;
    class ImageRequest;
    class SaveInfo;
    class SetHeader;

    Task(QThreadPool*, FoilPrivateKey*, FoilKey*);
    ~Task() Q_DECL_OVERRIDE;

    FoilMsg* decryptAndVerify(const QString&) const;
    FoilMsg* decryptAndVerify(const char*) const;
    QString writeThumb(const QImage&, const FoilMsgHeaders*,
        const char*, const QImage&, const QString&) const;

    static bool removeFile(const QString&);
    static QImage toImage(const FoilMsg*);
    static FoilOutput* createFoilFile(const QString&, GString*);
    static bool getHeader(FoilMsgHeader*, const FoilMsgHeaders*, const char*);

public:
    FoilPrivateKey* iPrivateKey;
    FoilKey* iPublicKey;
};

FoilPicsModel::Task::Task(
    QThreadPool* aPool,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey) :
    HarbourTask(aPool),
    iPrivateKey(foil_private_key_ref(aPrivateKey)),
    iPublicKey(foil_key_ref(aPublicKey))
{}

FoilPicsModel::Task::~Task()
{
    foil_private_key_unref(iPrivateKey);
    foil_key_unref(iPublicKey);
}

/* static */
bool
FoilPicsModel::Task::removeFile(
    const QString& aPath)
{
    if (!aPath.isEmpty()) {
        if (QFile::remove(aPath)) {
            return true;
        }
        HWARN("Failed to delete" << qPrintable(aPath));
    }
    return false;
}

FoilMsg*
FoilPicsModel::Task::decryptAndVerify(
    const QString& aFileName) const
{
    if (!aFileName.isEmpty()) {
        const QByteArray fileNameBytes(aFileName.toUtf8());

        return decryptAndVerify(fileNameBytes.constData());
    } else{
        return Q_NULLPTR;
    }
}

FoilMsg*
FoilPicsModel::Task::decryptAndVerify(
    const char* aFileName) const
{
    if (aFileName) {
        HDEBUG("Decrypting" << aFileName);

        FoilMsg* msg = foilmsg_decrypt_file(iPrivateKey, aFileName, Q_NULLPTR);

        if (msg) {
#if HARBOUR_DEBUG
            for (uint i = 0; i < msg->headers.count; i++) {
                const FoilMsgHeader* header = msg->headers.header + i;
                HDEBUG(" " << header->name << ":" << header->value);
            }
#endif // HARBOUR_DEBUG
            if (foilmsg_verify(msg, iPublicKey)) {
                return msg;
            } else {
                HWARN("Could not verify" << aFileName);
            }
            foilmsg_free(msg);
        }
    }
    return Q_NULLPTR;
}

/* static */
QImage
FoilPicsModel::Task::toImage(
    const FoilMsg* aMsg)
{
    if (aMsg) {
        const char* type = aMsg->content_type;

        if (!type || g_str_has_prefix(type, "image/")) {
            gsize size;
            const uchar* data = (uchar*)g_bytes_get_data(aMsg->data, &size);

            if (data && size) {
                return QImage::fromData(data, size, ModelData::format(type));
            }
        } else {
            HWARN("Unexpected content type" << type);
        }
    }
    return QImage();
}

/* static */
bool
FoilPicsModel::Task::getHeader(
    FoilMsgHeader* aHeader,
    const FoilMsgHeaders* aHeaders,
    const char* aKey)
{
    if (aHeaders) {
        for (uint i = 0; i < aHeaders->count; i++) {
            if (!strcmp(aHeaders->header[i].name, aKey)) {
                aHeader->value = aHeaders->header[i].value;
                aHeader->name = aKey;
                return true;
            }
        }
    }
    return false;
}

/* static */
FoilOutput*
FoilPicsModel::Task::createFoilFile(
    const QString& aDestDir,
    GString* aOutPath)
{
    // Generate random name for the encrypted file
    FoilOutput* out = Q_NULLPTR;
    const QByteArray dir(aDestDir.toUtf8());

    g_string_truncate(aOutPath, 0);
    g_string_append_len(aOutPath, dir.constData(), dir.size());
    g_string_append_c(aOutPath, '/');

    const gsize prefix_len = aOutPath->len;

    for (int i = 0; i < 100 && !out; i++) {
        guint8 data[8];

        foil_random_generate(FOIL_RANDOM_DEFAULT, data, sizeof(data));
        g_string_truncate(aOutPath, prefix_len);
        g_string_append_printf(aOutPath, "%02X%02X%02X%02X%02X%02X%02X%02X",
            data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
        out = foil_output_file_new_open(aOutPath->str);
    }
    HASSERT(out);
    return out;
}

QString
FoilPicsModel::Task::writeThumb(
    const QImage& aImage,
    const FoilMsgHeaders* aHeaders,
    const char* aContentType,
    const QImage& aThumb,
    const QString& aDestDir) const
{
    QString thumbName;

    if (!aThumb.isNull()) {
        static const char* keys[] = {
            HEADER_ORIGINAL_PATH,
            HEADER_ORIGINAL_SIZE,
            HEADER_MODIFICATION_TIME,
            HEADER_ACCESS_TIME,
            HEADER_ORIENTATION,
            HEADER_CAMERA_MANUFACTURER,
            HEADER_CAMERA_MODEL,
            HEADER_LATITUDE,
            HEADER_LONGITUDE,
            HEADER_ALTITUDE,
            HEADER_IMAGE_DATE,
            HEADER_TITLE,
            HEADER_GROUP
        };

        FoilMsgHeaders headers;
        FoilMsgHeader header[G_N_ELEMENTS(keys) + 2];

        FoilMsgEncryptOptions opt;
        memset(&opt, 0, sizeof(opt));
        opt.key_type = ENCRYPT_KEY_TYPE;

        headers.header = header;
        headers.count = 0;

        // Copy the headers
        for (uint i = 0; i < G_N_ELEMENTS(keys); i++) {
            if (getHeader(header + headers.count, aHeaders, keys[i])) {
                headers.count++;
            }
        }

        char width[16], height[16];

        snprintf(width, sizeof(width), "%d", aImage.width());
        header[headers.count].name = HEADER_THUMB_FULL_WIDTH;
        header[headers.count].value = width;
        headers.count++;

        snprintf(height, sizeof(height), "%d", aImage.height());
        header[headers.count].name = HEADER_THUMB_FULL_HEIGHT;
        header[headers.count].value = height;
        headers.count++;

        QByteArray thumbData;
        QBuffer buffer(&thumbData);
        aThumb.save(&buffer, ModelData::format(aContentType));

        GString* dest = g_string_sized_new(aDestDir.size() + 9);
        FoilOutput* out = createFoilFile(aDestDir, dest);
        if (out) {
            FoilBytes bytes;
            bytes.val = (guint8*)thumbData.constData();
            bytes.len = thumbData.size();
            HDEBUG("Writing thumbnail to" << dest->str);
            if (foilmsg_encrypt(out, &bytes, aContentType, &headers,
                iPrivateKey, iPublicKey, &opt, Q_NULLPTR)) {
                thumbName = QFileInfo(dest->str).fileName();
            }
            foil_output_unref(out);
        }
        g_string_free(dest, TRUE);
    }
    return thumbName;
}

// ==========================================================================
// FoilPicsModel::Task::GenerateKey
// ==========================================================================

class FoilPicsModel::Task::GenerateKey :
    public Task
{
    Q_OBJECT

public:
    GenerateKey(QThreadPool*, const QString&, int, const QString&);

    void performTask() Q_DECL_OVERRIDE;

public:
    const QString iKeyFile;
    const int iBits;
    const QString iPassword;
};

FoilPicsModel::Task::GenerateKey::GenerateKey(
    QThreadPool* aPool,
    const QString& aKeyFile,
    int aBits,
    const QString& aPassword) :
    Task(aPool, Q_NULLPTR, Q_NULLPTR),
    iKeyFile(aKeyFile),
    iBits(aBits),
    iPassword(aPassword)
{}

void
FoilPicsModel::Task::GenerateKey::performTask()
{
    HDEBUG("Generating key..." << iBits << "bits");
    FoilKey* key = foil_key_generate_new(FOIL_KEY_RSA_PRIVATE, iBits);

    if (key) {
        GError* error = Q_NULLPTR;
        const QByteArray path(iKeyFile.toUtf8());
        const QByteArray passphrase(iPassword.toUtf8());
        FoilOutput* out = foil_output_file_new_open(path.constData());
        FoilPrivateKey* pk = FOIL_PRIVATE_KEY(key);

        if (foil_private_key_encrypt(pk, out, FOIL_KEY_EXPORT_FORMAT_DEFAULT,
            passphrase.constData(), Q_NULLPTR, &error)) {
            iPrivateKey = pk;
            iPublicKey = foil_public_key_new_from_private(pk);
        } else {
            HWARN(error->message);
            g_error_free(error);
            foil_key_unref(key);
        }
        foil_output_unref(out);
    }
    HDEBUG("Done!");
}

// ==========================================================================
// FoilPicsModel::Task::Encrypt
// ==========================================================================

class FoilPicsModel::Task::Encrypt :
    public Task
{
    Q_OBJECT

public:
    Encrypt(QThreadPool*, const QString&, const QString&,
        FoilPrivateKey*, FoilKey*, const QSize&, const QVariantMap&);
    ~Encrypt() Q_DECL_OVERRIDE;

    void performTask() Q_DECL_OVERRIDE;

    static bool addDoubleHeader(const QVariant&, FoilMsgHeader*, const char*, char*);

public:
    const QString iSourceFile;
    const QString iDestDir;
    const QSize iThumbSize;
    const QVariantMap iMetaData;
    ModelData* iData;
};

FoilPicsModel::Task::Encrypt::Encrypt(
    QThreadPool* aPool,
    const QString& aSourceFile,
    const QString& aDestDir,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    const QSize& aThumbSize,
    const QVariantMap& aMetaData) :
    Task(aPool, aPrivateKey, aPublicKey),
    iSourceFile(aSourceFile),
    iDestDir(aDestDir),
    iThumbSize(aThumbSize),
    iMetaData(aMetaData),
    iData(Q_NULLPTR)
{
    HDEBUG("Encrypting" << qPrintable(aSourceFile) << aMetaData);
}

FoilPicsModel::Task::Encrypt::~Encrypt()
{
    // Normally, iData is null here
    delete iData;
}

/* static */
bool
FoilPicsModel::Task::Encrypt::addDoubleHeader(
    const QVariant& aValue,
    FoilMsgHeader* aHeader,
    const char* aName,
    char* aBuffer) // at least G_ASCII_DTOSTR_BUF_SIZE bytes long
{
    if (aValue.isValid()) {
        bool ok = false;
        double d = aValue.toDouble(&ok);

        if (ok) {
            g_ascii_dtostr(aBuffer, G_ASCII_DTOSTR_BUF_SIZE, d);
            aHeader->name = aName;
            aHeader->value = aBuffer;
            return true;
        }
    }
    aBuffer[0] = 0;
    return false;
}

void
FoilPicsModel::Task::Encrypt::performTask()
{
    const QByteArray path(iSourceFile.toUtf8());
    const char* fname = path.constData();

    HDEBUG(fname);

    GError* error = Q_NULLPTR;
    GMappedFile* map = g_mapped_file_new(fname, FALSE, &error);

    if (map) {
        GString* dest = g_string_sized_new(iDestDir.size() + 9);
        FoilOutput* out = createFoilFile(iDestDir, dest);

        if (out) {
            QMimeDatabase db;
            QMimeType type = db.mimeTypeForFile(iSourceFile);
            QByteArray mimeTypeBytes;
            const char* content_type = Q_NULLPTR;

            if (type.isValid()) {
                mimeTypeBytes = type.name().toUtf8();
                content_type = mimeTypeBytes.constData();
                HDEBUG(content_type);
            }

            FoilBytes bytes;

            bytes.val = (guint8*)g_mapped_file_get_contents(map);
            bytes.len = g_mapped_file_get_length(map);

            QImage image = QImage::fromData(bytes.val, bytes.len,
                ModelData::format(content_type));

            if (!image.isNull()) {
                char* mtime = Q_NULLPTR;
                char* atime = Q_NULLPTR;
                char* ttime = Q_NULLPTR;
                const QString title(ModelData::defaultTitle(iSourceFile));
                const QByteArray titleBytes(title.toUtf8());
                QDateTime sortTime, dateTaken;
                QString cameraMaker, cameraModel;
                QByteArray cameraMakerBytes, cameraModelBytes;

                FoilMsgEncryptOptions opt;
                memset(&opt, 0, sizeof(opt));
                opt.key_type = ENCRYPT_KEY_TYPE;

                FoilMsgHeaders headers;
                FoilMsgHeader header[12];

                headers.header = header;
                headers.count = 0;

                header[headers.count].name = HEADER_ORIGINAL_PATH;
                header[headers.count].value = fname;
                headers.count++;

                char fsize[16];
                snprintf(fsize, sizeof(fsize), "%lu", (gulong)bytes.len);
                header[headers.count].name = HEADER_ORIGINAL_SIZE;
                header[headers.count].value = fsize;
                headers.count++;

                header[headers.count].name = HEADER_TITLE;
                header[headers.count].value = titleBytes.constData();
                headers.count++;

                struct stat st;
                if (stat(fname, &st) == 0) {
                    /*
                     * Ignore deprecation warnings for GTimeVal and
                     * g_time_val_to_iso8601()
                     */
                    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
                    GTimeVal tv;

                    tv.tv_sec = st.st_mtim.tv_sec;
                    tv.tv_usec = st.st_mtim.tv_nsec / 1000;
                    mtime = g_time_val_to_iso8601(&tv);
                    header[headers.count].name = HEADER_MODIFICATION_TIME;
                    header[headers.count].value = mtime;
                    headers.count++;

                    sortTime.setMSecsSinceEpoch(((qint64)tv.tv_sec) * 1000 +
                        st.st_mtim.tv_nsec/1000000);

                    tv.tv_sec = st.st_atim.tv_sec;
                    tv.tv_usec = st.st_atim.tv_nsec / 1000;
                    atime = g_time_val_to_iso8601(&tv);
                    header[headers.count].name = HEADER_ACCESS_TIME;
                    header[headers.count].value = atime;
                    headers.count++;
                    G_GNUC_END_IGNORE_DEPRECATIONS;
                }

                // Metadata
                char degrees[16];
                int o = 0;
                QVariant var(iMetaData.value(MetaOrientation));
                if (var.isValid()) {
                    o = var.toInt();
                    snprintf(degrees, sizeof(degrees), "%d", o);
                    header[headers.count].name = HEADER_ORIENTATION;
                    header[headers.count].value = degrees;
                    headers.count++;
                }

                var = iMetaData.value(MetaImageDate);
                if (var.isValid()) {
                    QDateTime dateTime(var.toDateTime());
                    if (dateTime.isValid()) {
                        /*
                         * Ignore deprecation warnings for GTimeVal and
                         * g_time_val_to_iso8601()
                         */
                        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
                        GTimeVal tv;
                        qint64 msec = dateTime.toMSecsSinceEpoch();

                        tv.tv_sec = (glong)(msec/1000);
                        tv.tv_usec = (glong)(msec%1000) * 1000;
                        ttime = g_time_val_to_iso8601(&tv);
                        header[headers.count].name = HEADER_IMAGE_DATE;
                        header[headers.count].value = ttime;
                        headers.count++;
                        sortTime = dateTime;
                        dateTaken = dateTime;
                        G_GNUC_END_IGNORE_DEPRECATIONS;
                    }
                }

                cameraMaker = iMetaData.value(MetaCameraManufacturer).toString();
                if (!cameraMaker.isEmpty()) {
                    cameraMakerBytes = cameraMaker.toUtf8();
                    header[headers.count].name = HEADER_CAMERA_MANUFACTURER;
                    header[headers.count].value = cameraMakerBytes.constData();
                    headers.count++;
                }

                cameraModel = iMetaData.value(MetaCameraModel).toString();
                if (!cameraModel.isEmpty()) {
                    cameraModelBytes = cameraModel.toUtf8();
                    header[headers.count].name = HEADER_CAMERA_MODEL;
                    header[headers.count].value = cameraModelBytes.constData();
                    headers.count++;
                }

                char latitude[G_ASCII_DTOSTR_BUF_SIZE];
                if (addDoubleHeader(iMetaData.value(MetaLatitude),
                    header + headers.count, HEADER_LATITUDE, latitude)) {
                    headers.count++;
                }

                char longitude[G_ASCII_DTOSTR_BUF_SIZE];
                if (addDoubleHeader(iMetaData.value(MetaLongitude),
                    header + headers.count, HEADER_LONGITUDE, longitude)) {
                    headers.count++;
                }

                char altitude[G_ASCII_DTOSTR_BUF_SIZE];
                if (addDoubleHeader(iMetaData.value(MetaAltitude),
                    header + headers.count, HEADER_ALTITUDE, altitude)) {
                    headers.count++;
                }

                HASSERT(headers.count <= G_N_ELEMENTS(header));
                HDEBUG("Writing" << dest->str);
                if (foilmsg_encrypt(out, &bytes, content_type, &headers,
                    iPrivateKey, iPublicKey, &opt, Q_NULLPTR)) {
                    if (atime && mtime) {
                        foil_output_close(out);
                        foil_output_unref(out);
                        out = Q_NULLPTR;

                        struct timeval times[2];
                        times[0].tv_sec = st.st_atim.tv_sec;
                        times[0].tv_usec = st.st_atim.tv_nsec / 1000;
                        times[1].tv_sec = st.st_mtim.tv_sec;
                        times[1].tv_usec = st.st_mtim.tv_nsec / 1000;
                        if (utimes(dest->str, times) < 0) {
                            HWARN("Failed to set times on" <<
                                dest->str << ":" << strerror(errno));
                        }
                    }

                    GBytes* digest = foil_digest_data(DIGEST_TYPE,
                        bytes.val, bytes.len);
                    QImage thumb(ModelData::thumbnail(image, iThumbSize, o));
                    const QString thumbName(writeThumb(image, &headers,
                        content_type, thumb, iDestDir));

                    iData = new ModelData(iSourceFile, bytes.len, image.size(),
                        digest, dest->str, thumbName, thumb, title,
                        content_type, sortTime, o, cameraMaker,
                        cameraModel, latitude, longitude, altitude,
                        dateTaken, Q_NULLPTR);
                    g_bytes_unref(digest);
                }
                g_free(mtime);
                g_free(atime);
                g_free(ttime);
            }

            foil_output_unref(out);
        }
        g_mapped_file_unref(map);
        if (iData) {
            removeFile(iSourceFile);
        } else {
            unlink(dest->str);
        }
        g_string_free(dest, TRUE);
    } else {
        HWARN("Failed to read" << fname << error->message);
        g_error_free(error);
    }
}

// ==========================================================================
// FoilPicsModel::Task::SaveInfo
// ==========================================================================

class FoilPicsModel::Task::SaveInfo :
    public Task
{
    Q_OBJECT

public:
    SaveInfo(QThreadPool*, const ModelInfo&, const QString&,
        FoilPrivateKey*, FoilKey*);

    void performTask() Q_DECL_OVERRIDE;

public:
    const ModelInfo iInfo;
    const QString iFoilDir;
};

FoilPicsModel::Task::SaveInfo::SaveInfo(
    QThreadPool* aPool,
    const ModelInfo& aInfo,
    const QString& aFoilDir,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey) :
    Task(aPool, aPrivateKey, aPublicKey),
    iInfo(aInfo),
    iFoilDir(aFoilDir)
{}

void FoilPicsModel::Task::SaveInfo::performTask()
{
    if (!isCanceled()) {
        iInfo.save(iFoilDir, iPrivateKey, iPublicKey);
    }
}

// ==========================================================================
// FoilPicsModel::CheckPicsTask
// ==========================================================================

class FoilPicsModel::CheckPicsTask :
    public HarbourTask
{
    Q_OBJECT
public:
    CheckPicsTask(QThreadPool*, const QString&);

    void performTask() Q_DECL_OVERRIDE;

public:
    const QString iDir;
    bool iMayHaveEncryptedPictures;
};

FoilPicsModel::CheckPicsTask::CheckPicsTask(
    QThreadPool* aPool,
    const QString& aDir) :
    HarbourTask(aPool),
    iDir(aDir),
    iMayHaveEncryptedPictures(false)
{}

void
FoilPicsModel::CheckPicsTask::performTask()
{
    const QString path(iDir);
    HDEBUG("Checking" << iDir);

    QDir dir(path);
    QFileInfoList list = dir.entryInfoList(QDir::Files |
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);

    const QString infoFile(INFO_FILE);
    for (int i = 0; i < list.count() && !iMayHaveEncryptedPictures; i++) {
        const QFileInfo& info = list.at(i);

        if (info.isFile() && info.fileName() != infoFile) {
            const QByteArray fileNameBytes(info.filePath().toUtf8());
            const char* fname = fileNameBytes.constData();
            GMappedFile* map = g_mapped_file_new(fname, FALSE, Q_NULLPTR);

            if (map) {
                FoilBytes bytes;

                bytes.val = (guint8*)g_mapped_file_get_contents(map);
                bytes.len = g_mapped_file_get_length(map);

                FoilMsgInfo* info = foilmsg_parse(&bytes);

                if (info) {
                    HDEBUG(fname << "may be a foiled picture");
                    iMayHaveEncryptedPictures = true;
                    foilmsg_info_free(info);
                }
                g_mapped_file_unref(map);
            }
        }
    }
}

// ==========================================================================
// FoilPicsModel::DecryptPicsTask
// ==========================================================================

class FoilPicsModel::DecryptPicsTask :
    public Task
{
    Q_OBJECT

public:
    // The purpose of this class is to make sure that ModelData doesn't
    // get lost in transit when we asynchronously post the results from
    // DecryptPicsTask to FoilPicsModel.
    //
    // If the signal successfully reaches the slot, the receiver zeros
    // iModelData which stops ModelData from being deallocated by the
    // Progress destructor. If the signal never reaches the slot, then
    // ModelData is deallocated together with when the last reference
    // to Progress
    class Progress
    {
    public:
        typedef QSharedPointer<Progress> Ptr;

        Progress(ModelData* aModelData, DecryptPicsTask* aTask) :
            iModelData(aModelData), iTask(aTask) {}
        ~Progress() { delete iModelData; }

    public:
        ModelData* iModelData;
        DecryptPicsTask* iTask;
    };

    DecryptPicsTask(QThreadPool*, const QString&, FoilPrivateKey*, FoilKey*,
        const QSize&);

    void performTask() Q_DECL_OVERRIDE;

    ModelData* decryptThumb(const QString&, const QString&);
    ModelData* decryptImage(const QString&);
    bool decryptFile(const QString&, const QString&);

Q_SIGNALS:
    void groupsDecrypted(FoilPicsGroupModel::GroupList aGroups);
    void progress(DecryptPicsTask::Progress::Ptr aProgress);

public:
    const QString iDir;
    const QSize iThumbSize;
    bool iSaveInfo;
};

Q_DECLARE_METATYPE(FoilPicsModel::DecryptPicsTask::Progress::Ptr)

FoilPicsModel::DecryptPicsTask::DecryptPicsTask(
    QThreadPool* aPool,
    const QString& aDir,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    const QSize& aThumbSize) :
    Task(aPool, aPrivateKey, aPublicKey),
    iDir(aDir),
    iThumbSize(aThumbSize),
    iSaveInfo(false)
{}

FoilPicsModel::ModelData*
FoilPicsModel::DecryptPicsTask::decryptImage(
    const QString& aImagePath)
{
    FoilPicsModel::ModelData* data = Q_NULLPTR;
    FoilMsg* msg = decryptAndVerify(aImagePath);

    if (msg) {
        QString origPath(ModelData::headerString(msg, HEADER_ORIGINAL_PATH));

        if (!origPath.isEmpty()) {
            const QImage image(toImage(msg));

            if (!image.isNull()) {
                HDEBUG("Loaded image from" << qPrintable(aImagePath));
                int deg = ModelData::headerInt(msg, HEADER_ORIENTATION);
                QImage thumb(ModelData::thumbnail(image, iThumbSize, deg));
                QString thumbName(writeThumb(image, &msg->headers,
                    msg->content_type, thumb, iDir));

                data = ModelData::fromFoilMsg(msg, origPath, image.size(),
                    aImagePath, thumbName, thumb, msg->content_type, deg);
            }
        }
        foilmsg_free(msg);
    }
    return data;
}

FoilPicsModel::ModelData*
FoilPicsModel::DecryptPicsTask::decryptThumb(
    const QString& aImagePath,
    const QString& aThumbPath)
{
    FoilPicsModel::ModelData* data = Q_NULLPTR;
    FoilMsg* msg = decryptAndVerify(aThumbPath);

    if (msg) {
        // Thumbnail absolutely must have these:
        const int w = ModelData::headerInt(msg, HEADER_THUMB_FULL_WIDTH);
        const int h = ModelData::headerInt(msg, HEADER_THUMB_FULL_HEIGHT);
        QString origPath = ModelData::headerString(msg, HEADER_ORIGINAL_PATH);

        if (w > 0 && h > 0 && !origPath.isEmpty()) {
            // Make sure that the size is right
            QImage thumbImage = toImage(msg);
            QString thumbName = QFileInfo(aThumbPath).fileName();

            HDEBUG(thumbName << thumbImage.size());
            if (!thumbImage.isNull() && thumbImage.size() == iThumbSize) {
                // This thumb is good to go
                HDEBUG("Loaded thumbnail from" << qPrintable(aThumbPath));
                data = ModelData::fromFoilMsg(msg, origPath, QSize(w, h),
                    aImagePath, thumbName, thumbImage, msg->content_type,
                    ModelData::headerInt(msg, HEADER_ORIENTATION));
            }
        }
        foilmsg_free(msg);
    }
    return data;
}

bool
FoilPicsModel::DecryptPicsTask::decryptFile(
    const QString& aImagePath,
    const QString& aThumbPath)
{
    ModelData* data = decryptThumb(aImagePath, aThumbPath);

    if (!data) {
        data = decryptImage(aImagePath);
    }
    if (data) {
        // The Progress takes ownership of ModelData
        Q_EMIT progress(Progress::Ptr(new Progress(data, this)));
        return true;
    }
    return false;
}

void
FoilPicsModel::DecryptPicsTask::performTask()
{
    if (!isCanceled()) {
        const QString path(iDir);
        HDEBUG("Checking" << iDir);

        QDir dir(path);
        QFileInfoList list = dir.entryInfoList(QDir::Files |
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);

        // Restore the order
        ModelInfo info = ModelInfo::load(iDir, iPrivateKey, iPublicKey);
        Q_EMIT groupsDecrypted(info.iGroups);

        const QString infoFile(INFO_FILE);
        QHash<QString,QString> fileMap;
        int i;
        for (i = 0; i < list.count(); i++) {
            const QFileInfo& info = list.at(i);
            if (info.isFile()) {
                const QString name(info.fileName());
                if (name != infoFile) {
                    fileMap.insert(name, info.filePath());
                }
            }
        }

        // First decrypt files in known order
        for (i = 0; i < info.iOrder.count() && !isCanceled(); i++) {
            const QString image(info.iOrder.at(i));
            const QString thumb(info.iThumbMap.value(image));
            QString imagePath, thumbPath;
            if (fileMap.contains(image)) {
                imagePath = fileMap.take(image);
            } else {
                // Broken order
                HDEBUG(qPrintable(image) << "oops!");
                iSaveInfo = true;
            }
            if (!thumb.isEmpty()) {
                if (fileMap.contains(thumb)) {
                    thumbPath = fileMap.take(thumb);
                } else {
                    // Broken order
                    HDEBUG(qPrintable(thumb) << "oops!");
                    iSaveInfo = true;
                }
            }
            if (!decryptFile(imagePath, thumbPath)) {
                HDEBUG(qPrintable(image) << ":" <<
                    qPrintable(thumb) << "oops!");
                iSaveInfo = true;
            }
        }

        // Followed by the remaining files in no particular order
        if (!fileMap.isEmpty()) {
            QStringList remainingFiles = fileMap.values();
            HDEBUG("Remaining file(s)" << remainingFiles);
            for (i = 0; i < remainingFiles.count() && !isCanceled(); i++) {
                if (decryptFile(remainingFiles.at(i), QString())) {
                    HDEBUG(remainingFiles.at(i) << "was not expected");
                    iSaveInfo = true;
                }
            }
        }
    }
}

// ==========================================================================
// FoilPicsModel::Task::Decrypt
// ==========================================================================

class FoilPicsModel::Task::Decrypt :
    public Task
{
    Q_OBJECT

public:
    Decrypt(QThreadPool*, ModelData*, FoilPrivateKey*, FoilKey*);

    void performTask() Q_DECL_OVERRIDE;

    QString decryptionPath(FoilMsg*);
    bool saveDecrypted(FoilMsg*);

    static QString defaultDecryptDir();
    static void setTimeVal(struct timeval*, const char*, const struct timespec*);
    static void setFileTimes(const char*, const char*, const char*);

public:
    ModelData* iData;
    const QString iPath;
    const QString iThumbFile;
    bool iOk;
};

FoilPicsModel::Task::Decrypt::Decrypt(
    QThreadPool* aPool,
    ModelData* aData,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey) :
    Task(aPool, aPrivateKey, aPublicKey),
    iData(aData),
    iPath(aData->iPath),
    iThumbFile(aData->iThumbFile),
    iOk(false)
{}

/* static */
void
FoilPicsModel::Task::Decrypt::setTimeVal(
    struct timeval* aTimeVal,
    const char* aIso8601,
    const struct timespec* aDefaultTime)
{
    /*
     * Ignore deprecation warnings for GTimeVal and
     * g_time_val_from_iso8601()
     */
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    GTimeVal tv;
    if (aIso8601 && g_time_val_from_iso8601(aIso8601, &tv)) {
        aTimeVal->tv_sec = tv.tv_sec;
        aTimeVal->tv_usec = tv.tv_usec;
    } else {
        aTimeVal->tv_sec = aDefaultTime->tv_sec;
        aTimeVal->tv_usec = aDefaultTime->tv_nsec / 1000;
    }
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

/* static */
void
FoilPicsModel::Task::Decrypt::setFileTimes(
    const char* aPath,
    const char* aAccessTime,
    const char* aModificationTime)
{
    struct stat st;

    if ((aAccessTime || aModificationTime) && stat(aPath, &st) == 0) {
        struct timeval times[2];

        setTimeVal(times + 0, aAccessTime, &st.st_atim);
        setTimeVal(times + 1, aModificationTime, &st.st_mtim);
        if (utimes(aPath, times) < 0) {
            HWARN("Failed to set times on" << aPath << ":" << strerror(errno));
        }
    }
}

/* static */
QString
FoilPicsModel::Task::Decrypt::defaultDecryptDir()
{
    // ~/Pictures/FoilPics
    static const QString decryptDir(QDir(QStandardPaths::
        writableLocation(QStandardPaths::PicturesLocation)).
        absoluteFilePath("FoilPics"));

    return decryptDir;
}

QString
FoilPicsModel::Task::Decrypt::decryptionPath(
    FoilMsg* aMsg)
{
    const char* origPath = foilmsg_get_value(aMsg, HEADER_ORIGINAL_PATH);
    QFileInfo fileInfo;
    QString filePath;

    if (origPath && origPath[0]) {
        filePath = QString::fromUtf8(origPath);
    }
    if (filePath.isEmpty()) {
        // File name is unknown, pick the default
        QDir defaultDir(defaultDecryptDir());
        const char* format = ModelData::format(aMsg->content_type);

        if (format) {
            fileInfo = QFileInfo(defaultDir, QString("image.").append(QString::fromLatin1(format)));
        } else {
            fileInfo = QFileInfo(defaultDir, QString("image"));
        }
        filePath = fileInfo.absoluteFilePath();
    } else {
        fileInfo = QFileInfo(filePath);
    }

    QDir dir(fileInfo.dir());

    if (!dir.exists()) {
        // Destination directory doesn't exist. Try to create but only
        // if it's under home.
        QString dirPath(dir.absolutePath());

        if (!dirPath.startsWith(QDir::homePath() + QDir::separator())) {
            HDEBUG("Directory" << qPrintable(dirPath) << "is missing (not trying to create)");
        } else if (dir.mkpath(dirPath)) {
            HDEBUG("Created directory" << qPrintable(dirPath));
        } else {
            HWARN("Failed to create" << qPrintable(dirPath));
        }
        if (!dir.exists()) {
            // Try the default location ~/Pictures/FoilPics
            const QString defaultDir(defaultDecryptDir());

            if (dirPath.compare(defaultDir)) {
                dir.setPath(defaultDir);
                dirPath = dir.absolutePath();
                if (!dir.mkpath(dirPath)) {
                    HWARN("Failed to create" << qPrintable(dirPath));
                }
            }
        }
        if (dir.exists()) {
            fileInfo = QFileInfo(dir, fileInfo.fileName());
            filePath = fileInfo.absoluteFilePath();
        } else {
            return QString();
        }
    }
    if (fileInfo.exists()) {
        // Destination file already exists. We don't want to overwrite
        // existing files.
        HWARN(qPrintable(filePath) << "already exists");
        const QString baseName(fileInfo.baseName());
        const QString suffix(fileInfo.suffix());
        int i = 1;

        do {
            if (isCanceled()) {
                return QString();
            }
            // Build a similar file name
            fileInfo = QFileInfo(dir, baseName + QString::fromLatin1(" (") +
                QString::number(i++) + QString::fromLatin1(").") + suffix);
            filePath = fileInfo.absoluteFilePath();
        } while (fileInfo.exists());
    }
    return filePath;
}

bool
FoilPicsModel::Task::Decrypt::saveDecrypted(
    FoilMsg* aMsg)
{
    bool ok = false;
    const QString destPath(decryptionPath(aMsg));

    if (destPath.isEmpty()) {
        HWARN("Can't figure out file name for decryption");
    } else {
        const QByteArray destBytes(destPath.toUtf8());
        const char* dest = destBytes.constData();
        FoilOutput* out = foil_output_file_new_open(dest);

        if (out) {
            if (foil_output_write_bytes_all(out, aMsg->data) &&
                foil_output_flush(out)) {
                foil_output_close(out);
                HDEBUG("Wrote" << dest);
                setFileTimes(dest,
                    foilmsg_get_value(aMsg, HEADER_ACCESS_TIME),
                    foilmsg_get_value(aMsg, HEADER_MODIFICATION_TIME));
                ok = true;
            } else {
                HWARN("Failed to write" << dest);
            }
            foil_output_unref(out);
        } else {
            HWARN("Failed to open" << dest);
        }
    }
    return ok;
}

void
FoilPicsModel::Task::Decrypt::performTask()
{
    FoilMsg* msg = decryptAndVerify(iPath);

    if (msg) {
        iOk = (!isCanceled() && saveDecrypted(msg));
        foilmsg_free(msg);
        if (iOk) {
            removeFile(iPath);
            if (!iThumbFile.isEmpty()) {
                QString thumbPath(QFileInfo(iPath).dir().filePath(iThumbFile));
                removeFile(thumbPath);
            }
        }
    }
}

// ==========================================================================
// FoilPicsModel::Task::ImageRequest
// ==========================================================================

class FoilPicsModel::Task::ImageRequest :
    public Task
{
    Q_OBJECT

public:
    ImageRequest(QThreadPool*, const QString&, const QByteArray&,
        const QString&, FoilPrivateKey*, FoilKey*, FoilPicsImageRequest);
    ~ImageRequest() Q_DECL_OVERRIDE;

    void performTask() Q_DECL_OVERRIDE;

public:
    const QString iPath;
    QByteArray iBytes;
    QString iContentType;
    FoilPicsImageRequest iRequest;
};

FoilPicsModel::Task::ImageRequest::ImageRequest(
    QThreadPool* aPool,
    const QString& aPath,
    const QByteArray& aBytes,
    const QString& aContentType,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    FoilPicsImageRequest aRequest) :
    Task(aPool, aPrivateKey, aPublicKey),
    iPath(aPath),
    iBytes(aBytes),
    iContentType(aContentType),
    iRequest(aRequest)
{}

FoilPicsModel::Task::ImageRequest::~ImageRequest()
{
    // Make sure we have replied to the request
    iRequest.reply();
}

void
FoilPicsModel::Task::ImageRequest::performTask()
{
    FoilMsg* msg = Q_NULLPTR;
    const QByteArray contentTypeBytes(iContentType.toLatin1());
    const char* type = contentTypeBytes.constData();

    if (iBytes.isEmpty() && !isCanceled()) {
        const QByteArray path(iPath.toUtf8());
        const char* fname = path.constData();

        msg = decryptAndVerify(fname);
        if (msg && !isCanceled()) {
            gsize size;
            const char* data = (char*)g_bytes_get_data(msg->data, &size);
            if (data && size) {
                iBytes = QByteArray(data, size);
            }
        }
    }

    if (!iBytes.isEmpty() && !isCanceled()) {
        QImage image(QImage::fromData(iBytes, ModelData::format(type)));

        HDEBUG(qPrintable(iPath) << image.size());
        iRequest.reply(image);
    } else {
        // This sends empty reply
        iRequest.reply();
    }
    foilmsg_free(msg);
}

// ==========================================================================
// FoilPicsModel::Task::SetHeader
// ==========================================================================

class FoilPicsModel::Task::SetHeader :
    public Task
{
    Q_OBJECT

public:
    SetHeader(QThreadPool*, FoilPrivateKey*, FoilKey*, ModelData*,
        const char*, const QByteArray&);

    static SetHeader* createTitleTask(QThreadPool*, FoilPrivateKey*, FoilKey*,
        ModelData*);
    static SetHeader* createGroupTask(QThreadPool*, FoilPrivateKey*, FoilKey*,
        ModelData*);

    void performTask() Q_DECL_OVERRIDE;

    QString setHeaderAndEncrypt(FoilMsg* aMsg);

public:
    ModelData* iData;
    const QString iPath;
    const QString iThumbFile;
    const char* iHeaderName;
    const QByteArray iHeaderValue;
    QString iNewPath;
    QString iNewThumbFile;
    bool iOk;
};

FoilPicsModel::Task::SetHeader::SetHeader(
    QThreadPool* aPool,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    ModelData* aData,
    const char* aHeaderName,
    const QByteArray& aHeaderValue) :
    Task(aPool, aPrivateKey, aPublicKey),
    iData(aData),
    iPath(aData->iPath),
    iThumbFile(aData->iThumbFile),
    iHeaderName(aHeaderName),
    iHeaderValue(aHeaderValue),
    iOk(false)
{}

/* static */
FoilPicsModel::Task::SetHeader*
FoilPicsModel::Task::SetHeader::createTitleTask(
    QThreadPool* aPool,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    ModelData* aData)
{
    return new SetHeader(aPool, aPrivateKey, aPublicKey, aData,
        HEADER_TITLE, aData->iTitle.toUtf8());
}

/* static */
FoilPicsModel::Task::SetHeader*
FoilPicsModel::Task::SetHeader::createGroupTask(
    QThreadPool* aPool,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    ModelData* aData)
{
    return new SetHeader(aPool, aPrivateKey, aPublicKey, aData,
        HEADER_GROUP, aData->iGroupId);
}

QString
FoilPicsModel::Task::SetHeader::setHeaderAndEncrypt(
    FoilMsg* aMsg)
{
    QString destPath;
    const QString destDir(QFileInfo(iPath).dir().path());
    GString* dest = g_string_sized_new(destDir.size() + 9);
    FoilOutput* out = createFoilFile(destDir, dest);

    if (out) {
        // Allocate one more in case if the header in question is missing
        FoilMsgHeader* header = new FoilMsgHeader[aMsg->headers.count + 1];
        FoilMsgHeaders headers;
        uint i;

        HDEBUG("Writing" << dest->str);

        // Copy the headers (except the one we are going to change)
        headers.header = header;
        headers.count = 0;
        for (i = 0; i < aMsg->headers.count; i++) {
            const FoilMsgHeader* src = aMsg->headers.header + i;
            if (strcmp(src->name, iHeaderName)) {
                HDEBUG(" " << src->name << ":" << src->value);
                header[headers.count++] = *src;
            }
        }

        // Now add the header in question (unless it's empty)
        if (!iHeaderValue.isEmpty()) {
            HDEBUG(" " << iHeaderName << ":" << iHeaderValue.constData());
            header[headers.count].name = iHeaderName;
            header[headers.count].value = iHeaderValue.constData();
            headers.count++;
        }

        FoilMsgEncryptOptions opt;

        memset(&opt, 0, sizeof(opt));
        opt.key_type = ENCRYPT_KEY_TYPE;

        FoilBytes bytes;
        if (foilmsg_encrypt(out, foil_bytes_from_data(&bytes, aMsg->data),
            aMsg->content_type, &headers, iPrivateKey, iPublicKey, &opt,
            Q_NULLPTR)) {
            destPath = QString(dest->str);
            HDEBUG("Wrote" << foil_output_bytes_written(out) << "bytes");
        }
        foil_output_unref(out);
        g_string_free(dest, TRUE);
        delete [] header;
    }
    return destPath;
}

void
FoilPicsModel::Task::SetHeader::performTask()
{
    FoilMsg* fileMsg = decryptAndVerify(iPath);

    if (fileMsg) {
        if (!isCanceled()) {
            QString thumbPath(QFileInfo(iPath).dir().filePath(iThumbFile));
            FoilMsg* thumbMsg = decryptAndVerify(thumbPath);

            if (thumbMsg) {
                if (!isCanceled()) {
                    QString newFile = setHeaderAndEncrypt(fileMsg);

                    if (!newFile.isEmpty()) {
                        if (!isCanceled()) {
                            QString newThumb = setHeaderAndEncrypt(thumbMsg);

                            if (!newThumb.isEmpty()) {
                                // All good
                                iOk = true;
                                iNewPath = newFile;
                                iNewThumbFile = QFileInfo(newThumb).fileName();
                                HDEBUG(iNewPath << iNewThumbFile);
                                // Remove the old files
                                removeFile(iPath);
                                removeFile(thumbPath);
                            } else {
                                // Failed to save thumb
                                removeFile(newFile);
                            }
                        } else {
                            // Cancelled
                            removeFile(newFile);
                        }
                    }
                }
                foilmsg_free(thumbMsg);
            }
        }
        foilmsg_free(fileMsg);
    }
}

// ==========================================================================
// FoilPicsModel::Private
// ==========================================================================

// s(SignalName,signalName,Suffix)
#define QUEUED_SIGNALS(s) \
    s(Count,count,Changed) \
    s(Busy,busy,Changed) \
    s(KeyAvailable,keyAvailable,Changed) \
    s(FoilState,foilState,Changed) \
    s(MayHaveEncryptedPictures,mayHaveEncryptedPictures,Changed) \
    s(ThumbnailSize,thumbnailSize,Changed) \
    s(DecryptionStarted,decryptionStarted,)

enum FoilPicsModelSignal {
    #define SIGNAL_ENUM_(Name,name,Suffix) Signal##Name##Suffix,
    QUEUED_SIGNALS(SIGNAL_ENUM_)
    #undef SIGNAL_ENUM_
    FoilPicsModelSignalCount
};

typedef HarbourParentSignalQueueObject<FoilPicsModel,
    FoilPicsModelSignal, FoilPicsModelSignalCount>
    FoilPicsModelPrivateBase;

class FoilPicsModel::Private :
    public FoilPicsModelPrivateBase
{
    Q_OBJECT

    static const SignalEmitter gSignalEmitters [];

public:
    Private(FoilPicsModel*);
    ~Private();

    ModelData* dataAt(int);
    int find(const QString&);
    int compare(const ModelData*, const ModelData*) const;
    static int sortProc(const void*, const void*, void*);

    struct LessThan {
        Private* obj;
        bool operator()(ModelData* aData1, ModelData* aData2) const {
            return obj->compare(aData1, aData2) < 0;
        }
        LessThan(Private* aThis) : obj(aThis) {}
    };

Q_SIGNALS:
    void stopIgnoringGroupModelChanges();

public Q_SLOTS:
    void onCheckPicsTaskDone();
    void onGroupsDecrypted(FoilPicsGroupModel::GroupList);
    void onDecryptPicsProgress(DecryptPicsTask::Progress::Ptr);
    void onDecryptPicsTaskDone();
    void onGenerateKeyTaskDone();
    void onEncryptTaskDone();
    void onDecryptTaskDone();
    void onDecryptAllProgress();
    void onSetTitleTaskDone();
    void onSetGroupTaskDone();
    void onSaveInfoDone();
    void onImageRequestDone();
    void onGroupModelChanged();
    void onGroupRenamed(int, const FoilPicsGroupModel::Group&);
    void onStopIgnoringGroupModelChanges();

public:
    static size_t maxBytesToDecrypt();
    void connectGroupModel();
    bool disconnectGroupModel();
    bool checkPassword(const QString&);
    bool changePassword(const QString&, const QString&);
    void setKeys(FoilPrivateKey*, FoilKey* aPublic = Q_NULLPTR);
    void setFoilState(FoilState);
    void insertModelData(ModelData*);
    void destroyItemAt(int);
    bool destroyItemAndRemoveFilesAt(int);
    void removeAt(int);
    void removeFiles(QList<int>);
    void clearGroupModel();
    void clearModel();
    void saveInfo();
    void generate(int, const QString&);
    void lock(bool);
    bool unlock(const QString&);
    void encryptFile(const QString&, QVariantMap);
    bool encrypt(const QUrl&, const QVariantMap&);
    void encryptFiles(QAbstractItemModel*, const QList<int>&);
    void decryptAt(int aIndex);
    void decryptFiles(QList<int> aRows);
    void decryptAll();
    void decryptTaskDone(Task::Decrypt*, bool);
    void ignoreGroupModelChange();
    void setTitleAt(int, const QString&);
    bool setGroupId(ModelData*, const QByteArray&);
    void clearGroup(const QByteArray&);
    bool sortModel();
    void setGroupIdAt(int, const QByteArray&);
    void setGroupIdForRows(QList<int>, const QByteArray&);
    void dataChanged(int, ModelData::Role);
    void dataChanged(const QList<int>&, ModelData::Role);
    void imageRequest(const QString&, FoilPicsImageRequest);
    void headerUpdateDone(Task::SetHeader*);
    int findPath(const QString&);
    bool dropDecryptedData(int);
    bool tooMuchDataDecrypted();
    bool busy() const;

public:
    const size_t iMaxBytesToDecrypt;
    bool iMayHaveEncryptedPictures;
    FoilPicsImageProvider* iImageProvider;
    FoilPicsThumbnailProvider* iThumbnailProvider;
    QSize iThumbSize;
    ModelData::List iData;
    FoilState iFoilState;
    QString iFoilPicsDir;
    QString iFoilKeyDir;
    QString iFoilKeyFile;
    FoilPrivateKey* iPrivateKey;
    FoilKey* iPublicKey;
    QThreadPool* iThreadPool;
    CheckPicsTask* iCheckPicsTask;
    Task::SaveInfo* iSaveInfoTask;
    Task::GenerateKey* iGenerateKeyTask;
    DecryptPicsTask* iDecryptPicsTask;
    QList<Task::Encrypt*> iEncryptTasks;
    QList<Task::ImageRequest*> iImageRequestTasks;
    FoilPicsGroupModel* iGroupModel;
    bool iGroupModelConnected;
    int iIgnoreGroupModelChanges;
};

const FoilPicsModel::Private::SignalEmitter
FoilPicsModel::Private::gSignalEmitters [] = {
    #define SIGNAL_EMITTER_(Name,name,Suffix) &FoilPicsModel::name##Suffix,
    QUEUED_SIGNALS(SIGNAL_EMITTER_)
    #undef  SIGNAL_EMITTER_
};

FoilPicsModel::Private::Private(
    FoilPicsModel* aParent) :
    FoilPicsModelPrivateBase(aParent, gSignalEmitters),
    iMaxBytesToDecrypt(maxBytesToDecrypt()),
    iMayHaveEncryptedPictures(false),
    iImageProvider(Q_NULLPTR),
    iThumbnailProvider(Q_NULLPTR),
    iThumbSize(32,32),
    iFoilState(FoilKeyMissing),
    iFoilPicsDir(QDir::homePath() + "/" FOIL_PICS_DIR),
    iFoilKeyDir(QDir::homePath() + "/" FOIL_KEY_DIR),
    iFoilKeyFile(iFoilKeyDir + "/" + FOIL_KEY_FILE),
    iPrivateKey(Q_NULLPTR),
    iPublicKey(Q_NULLPTR),
    iThreadPool(new QThreadPool(this)),
    iCheckPicsTask(Q_NULLPTR),
    iSaveInfoTask(Q_NULLPTR),
    iGenerateKeyTask(Q_NULLPTR),
    iDecryptPicsTask(Q_NULLPTR),
    iGroupModel(Q_NULLPTR), // FoilPicsModel::FoilPicsModel will create it
    iGroupModelConnected(false),
    iIgnoreGroupModelChanges(0)
{
    // Serialize the tasks:
    iThreadPool->setMaxThreadCount(1);
    qRegisterMetaType<DecryptPicsTask::Progress::Ptr>("DecryptPicsTask::Progress::Ptr");

    HDEBUG("Key file" << qPrintable(iFoilKeyFile));
    HDEBUG("Pics dir" << qPrintable(iFoilPicsDir));

    // Create the directories if necessary
    if (QDir().mkpath(iFoilKeyDir)) {
        const QByteArray dir(iFoilKeyDir.toUtf8());
        chmod(dir.constData(), 0700);
    }

    if (QDir().mkpath(iFoilPicsDir)) {
        const QByteArray dir(iFoilPicsDir.toUtf8());
        chmod(dir.constData(), 0700);
    }

    // Initialize the key state
    GError* error = Q_NULLPTR;
    const QByteArray path(iFoilKeyFile.toUtf8());
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), Q_NULLPTR, &error);

    if (key) {
        HDEBUG("Key not encrypted");
        iFoilState = FoilKeyNotEncrypted;
        foil_private_key_unref(key);
    } else {
        if (error->domain == FOIL_ERROR) {
            if (error->code == FOIL_ERROR_KEY_ENCRYPTED) {
                HDEBUG("Key encrypted");
                iFoilState = FoilLocked;
            } else {
                HDEBUG("Key invalid" << error->code);
                iFoilState = FoilKeyInvalid;
            }
        } else {
            HDEBUG(error->message);
            iFoilState = FoilKeyMissing;
        }
        g_error_free(error);
    }

    iCheckPicsTask = new CheckPicsTask(iThreadPool, iFoilPicsDir);
    iCheckPicsTask->submit(this, SLOT(onCheckPicsTaskDone()));

    // A trick to stop one queued group model change:
    connect(this,
        SIGNAL(stopIgnoringGroupModelChanges()),
        SLOT(onStopIgnoringGroupModelChanges()),
        Qt::QueuedConnection);
}

FoilPicsModel::Private::~Private()
{
    foil_private_key_unref(iPrivateKey);
    foil_key_unref(iPublicKey);
    if (iCheckPicsTask) iCheckPicsTask->release(this);
    if (iSaveInfoTask) iSaveInfoTask->release(this);
    if (iGenerateKeyTask) iGenerateKeyTask->release(this);
    if (iDecryptPicsTask) iDecryptPicsTask->release(this);
    int i;
    for (i = 0; i < iEncryptTasks.count(); i++) {
        iEncryptTasks.at(i)->release(this);
    }
    iEncryptTasks.clear();
    for (i = 0; i < iImageRequestTasks.count(); i++) {
        iImageRequestTasks.at(i)->release(this);
    }
    iImageRequestTasks.clear();
    iThreadPool->waitForDone();
    qDeleteAll(iData);
    if (iImageProvider) {
        iImageProvider->release();
    }
    if (iThumbnailProvider) {
        iThumbnailProvider->release();
    }
}

size_t
FoilPicsModel::Private::maxBytesToDecrypt()
{
    // Basically, we are willing to use up to 5MB per gigabyte of RAM
    size_t kbTotal = sysconf(_SC_PHYS_PAGES)*sysconf(_SC_PAGESIZE)/0x400;

    HDEBUG("We seem to have" << kbTotal << "kB of RAM");
    return 5*kbTotal;
}

FoilPicsModel::ModelData*
FoilPicsModel::Private::dataAt(
    int aIndex)
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        return iData.at(aIndex);
    } else {
        return Q_NULLPTR;
    }
}

int
FoilPicsModel::Private::find(
    const QString& aImageId)
{
    const int n = iData.count();

    for (int i = 0; i < n; i++) {
        ModelData* data = iData.at(i);

        if (data->iImageId == aImageId) {
            return i;
        }
    }
    return -1;
}

bool
FoilPicsModel::Private::disconnectGroupModel()
{
    if (iGroupModelConnected) {
        iGroupModelConnected = false;
        iGroupModel->disconnect(this);
        return true;
    } else {
        return false;
    }
}

void
FoilPicsModel::Private::connectGroupModel()
{
    if (!iGroupModelConnected) {
        static const char* modelSignals[] = {
            SIGNAL(modelReset()),
            SIGNAL(rowsInserted(QModelIndex,int,int)),
            SIGNAL(rowsRemoved(QModelIndex,int,int)),
            SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>))
        };

        // Save the info whenever group model changes
        iGroupModelConnected = true;
        for (unsigned int i = 0; i < G_N_ELEMENTS(modelSignals); i++) {
            connect(iGroupModel, modelSignals[i], SLOT(onGroupModelChanged()));
        }
        // rowsMoved is ignored because it's being emitted when groups
        // are being dragged. When the drag is finished and rows have
        // actually been moved, rowsActuallyMoved signal is emitted.
        connect(iGroupModel,
            SIGNAL(rowsActuallyMoved()),
            SLOT(onGroupModelChanged()));
        // And this is to emit dataChanged when groupName role changes:
        connect(iGroupModel,
            SIGNAL(groupRenamed(int,FoilPicsGroupModel::Group)),
            SLOT(onGroupRenamed(int,FoilPicsGroupModel::Group)));
    }
}

void
FoilPicsModel::Private::setKeys(
    FoilPrivateKey* aPrivate,
    FoilKey* aPublic)
{
    if (aPrivate) {
        if (iPrivateKey) {
            foil_private_key_unref(iPrivateKey);
        } else {
            queueSignal(SignalKeyAvailableChanged);
        }
        foil_key_unref(iPublicKey);
        iPrivateKey = foil_private_key_ref(aPrivate);
        iPublicKey = aPublic ? foil_key_ref(aPublic) :
            foil_public_key_new_from_private(aPrivate);
    } else if (iPrivateKey) {
        queueSignal(SignalKeyAvailableChanged);
        foil_private_key_unref(iPrivateKey);
        foil_key_unref(iPublicKey);
        iPrivateKey = Q_NULLPTR;
        iPublicKey = Q_NULLPTR;
    }
}

bool
FoilPicsModel::Private::checkPassword(
    const QString& aPassword)
{
    GError* error = Q_NULLPTR;
    const QByteArray path(iFoilKeyFile.toUtf8());
    HDEBUG(iFoilKeyFile);

    // First make sure that it's encrypted
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), Q_NULLPTR, &error);
    if (key) {
        HWARN("Key not encrypted");
        foil_private_key_unref(key);
    } else if (error->domain == FOIL_ERROR) {
        if (error->code == FOIL_ERROR_KEY_ENCRYPTED) {
            // Validate the old password
            QByteArray password(aPassword.toUtf8());
            g_clear_error(&error);
            key = foil_private_key_decrypt_from_file
                (FOIL_KEY_RSA_PRIVATE, path.constData(),
                    password.constData(), &error);
            if (key) {
                HDEBUG("Password OK");
                foil_private_key_unref(key);
                return true;
            } else {
                HDEBUG("Wrong password");
                g_error_free(error);
            }
        } else {
            HWARN("Key invalid:" << error->message);
            g_error_free(error);
        }
    } else {
        HWARN(error->message);
        g_error_free(error);
    }
    return false;
}

bool
FoilPicsModel::Private::changePassword(
    const QString& aOldPassword,
    const QString& aNewPassword)
{
    HDEBUG(iFoilKeyFile);
    if (checkPassword(aOldPassword)) {
        GError* error = Q_NULLPTR;
        QByteArray password(aNewPassword.toUtf8());

        // First write the temporary file
        QString tmpKeyFile = iFoilKeyFile + ".new";
        const QByteArray tmp(tmpKeyFile.toUtf8());
        FoilOutput* out = foil_output_file_new_open(tmp.constData());

        if (foil_private_key_encrypt(iPrivateKey, out,
            FOIL_KEY_EXPORT_FORMAT_DEFAULT, password.constData(),
            Q_NULLPTR, &error) && foil_output_flush(out)) {
            foil_output_unref(out);

            // Then rename it
            QString saveKeyFile = iFoilKeyFile + ".save";
            QFile::remove(saveKeyFile);
            if (QFile::rename(iFoilKeyFile, saveKeyFile) &&
                QFile::rename(tmpKeyFile, iFoilKeyFile)) {
                Task::removeFile(saveKeyFile);
                HDEBUG("Password changed");
                Q_EMIT parentObject()->passwordChanged();
                return true;
            }
        } else {
            if (error) {
                HWARN(error->message);
                g_error_free(error);
            }
            foil_output_unref(out);
        }
    }
    return false;
}

void
FoilPicsModel::Private::setFoilState(
    FoilState aState)
{
    if (iFoilState != aState) {
        iFoilState = aState;
        if (iFoilState == FoilPicsReady) {
            // Save the info whenever group model changes
            connectGroupModel();
            // onDecryptPicsTaskDone may be queued before the last
            // picture has been inserted into to the model which may
            // generate a few stray group model change events resulting
            // in an unnecessary overwrite of the info file. So we ignore
            // group model changes until queued stopIgnoringGroupModelChanges
            // event arrives.
            ignoreGroupModelChange();
        } else {
            // Group model is changing whenever the pictures are being
            // decrypted, we don't want to rewrite the info file every
            // time we are decrypting the pictures. Flash memory has
            // a limited number of write cycles.
            disconnectGroupModel();
        }
        queueSignal(SignalFoilStateChanged);
    }
}

void
FoilPicsModel::Private::ignoreGroupModelChange()
{
    iIgnoreGroupModelChanges++;
    Q_EMIT stopIgnoringGroupModelChanges();
    HDEBUG(iIgnoreGroupModelChanges);
}

void
FoilPicsModel::Private::onStopIgnoringGroupModelChanges()
{
    iIgnoreGroupModelChanges--;
    HDEBUG(iIgnoreGroupModelChanges);
    HASSERT(iIgnoreGroupModelChanges >= 0);
}

int
FoilPicsModel::Private::compare(
    const ModelData* aData1,
    const ModelData* aData2) const
{
    if (aData1->iGroupId != aData2->iGroupId) {
        // Different groups
        return iGroupModel->indexOfGroup(aData1->iGroupId) -
            iGroupModel->indexOfGroup(aData2->iGroupId);
    }

    // Most recent first (within the group)
    const qint64 msec1 = aData1->iSortTime.toMSecsSinceEpoch();
    const qint64 msec2 = aData2->iSortTime.toMSecsSinceEpoch();

    if (msec1 > msec2) {
        return -1;
    } else if (msec1 < msec2) {
        return 1;
    }

    // Finally compare the original file name. If those match too,
    // then image ids must be different.
    int res = aData1->iFileName.compare(aData2->iFileName);

    return res ? res : aData1->iImageId.compare(aData2->iImageId);
}

int
FoilPicsModel::Private::sortProc(
    const void* aPtr1,
    const void* aPtr2,
    void* aThis)
{
    ModelData** aDataPtr1 = (ModelData**)aPtr1;
    ModelData** aDataPtr2 = (ModelData**)aPtr2;

    return ((Private*)aThis)->compare(*aDataPtr1, *aDataPtr2);
}

void
FoilPicsModel::Private::insertModelData(
    ModelData* aData)
{
    FoilPicsModel* model = parentObject();

    // Create image providers on demand because QQmlEngine::contextForObject
    // doesn't work at initialization time
    if (!iThumbnailProvider) {
        iThumbnailProvider = FoilPicsThumbnailProvider::createForObject(model);
    }
    if (iThumbnailProvider) {
        aData->iThumbSource = iThumbnailProvider->addThumbnail(aData->iImageId,
            aData->iThumbnail);
        aData->updateVariantMap(ModelData::ThumbnailRole);
    }
    if (!iImageProvider) {
        iImageProvider = FoilPicsImageProvider::createForObject(model);
    }
    if (iImageProvider) {
        aData->iImageSource = iImageProvider->addImage(aData->iImageId,
            aData->iPath);
        aData->updateVariantMap(ModelData::UrlRole);
    }

    // Make sure that group id is valid
    if (!iGroupModel->isKnownGroup(aData->iGroupId)) {
        aData->iGroupId = QByteArray(); // Default group
    }

    // Insert the data into the model
    ModelData::ConstIterator it = qLowerBound(iData.begin(), iData.end(),
        aData, LessThan(this));
    const int pos = it - iData.begin();

    model->beginInsertRows(QModelIndex(), pos, pos);
    iData.insert(pos, aData);
    HDEBUG(iData.count() << aData->iSortTime.
        toString(Qt::SystemLocaleShortDate) << "at" << pos);    

    // And this tells the app that we better not generate a new key:
    if (!iMayHaveEncryptedPictures) {
        iMayHaveEncryptedPictures = true;
        queueSignal(SignalMayHaveEncryptedPicturesChanged);
    }
    model->endInsertRows();
    queueSignal(SignalCountChanged);
}

void
FoilPicsModel::Private::destroyItemAt(
    int aIndex)
{
    if (aIndex >= 0 && aIndex <= iData.count()) {
        FoilPicsModel* model = parentObject();
        ModelData* data = iData.at(aIndex);

        HDEBUG("Removing" << qPrintable(data->iPath));
        // Providers must have been created by insertModelData
        iThumbnailProvider->releaseThumbnail(data->iImageId);
        iImageProvider->releaseImage(data->iImageId);
        model->beginRemoveRows(QModelIndex(), aIndex, aIndex);
        delete iData.takeAt(aIndex);
        if (iData.isEmpty() && iMayHaveEncryptedPictures) {
            // We no longer have any decryptable pictures
            iMayHaveEncryptedPictures = false;
            queueSignal(SignalMayHaveEncryptedPicturesChanged);
        }
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
    }
}

bool
FoilPicsModel::Private::destroyItemAndRemoveFilesAt(
    int aIndex)
{
    ModelData* data = dataAt(aIndex);

    if (data) {
        const QString path(data->iPath);
        QString thumbPath;

        if (!data->iThumbFile.isEmpty()) {
            thumbPath = (QFileInfo(path).dir().filePath(data->iThumbFile));
            HDEBUG("Removing" << qPrintable(thumbPath));
        }
        destroyItemAt(aIndex);
        Task::removeFile(path);
        Task::removeFile(thumbPath);
        return true;
    }
    return false;
}

void
FoilPicsModel::Private::removeAt(
    int aIndex)
{
    const bool wasBusy = busy();
    if (destroyItemAndRemoveFilesAt(aIndex)) {
        // saveInfo() doesn't queue BusyChanged signal, we have to do it here
        saveInfo();
        if (wasBusy != busy()) {
            queueSignal(SignalBusyChanged);
        }
    }
}

void
FoilPicsModel::Private::removeFiles(
    QList<int> aRows)
{
    const int n = aRows.count();

    if (n > 0) {
        int removed = 0;
        const bool wasBusy = busy();

        qSort(aRows);
        for (int i = n - 1; i >= 0; i--) {
            if (destroyItemAndRemoveFilesAt(aRows.at(i))) {
                removed++;
            }
        }
        if (removed > 0) {
            saveInfo();
            if (wasBusy != busy()) {
                queueSignal(SignalBusyChanged);
            }
        }
    }
}

void
FoilPicsModel::Private::clearGroupModel()
{
    const bool wasConnected = disconnectGroupModel();

    iGroupModel->clear();
    if (wasConnected) connectGroupModel();
}

void
FoilPicsModel::Private::clearModel()
{
    const int n = iData.count();

    if (n > 0) {
        FoilPicsModel* model = parentObject();

        model->beginRemoveRows(QModelIndex(), 0, n-1);
        qDeleteAll(iData);
        iData.clear();
        // We no longer have any decryptable pictures:
        if (iMayHaveEncryptedPictures) {
            iMayHaveEncryptedPictures = false;
            queueSignal(SignalMayHaveEncryptedPicturesChanged);
        }
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
    }
}

void
FoilPicsModel::Private::onCheckPicsTaskDone()
{
    HDEBUG("Done");
    if (sender() == iCheckPicsTask) {
        const bool mayHave = iCheckPicsTask->iMayHaveEncryptedPictures;

        if (iMayHaveEncryptedPictures != mayHave) {
            iMayHaveEncryptedPictures = mayHave;
            queueSignal(SignalMayHaveEncryptedPicturesChanged);
        }
        iCheckPicsTask->release(this);
        iCheckPicsTask = Q_NULLPTR;
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void
FoilPicsModel::Private::onGroupModelChanged()
{
    if (iIgnoreGroupModelChanges > 0) {
        HDEBUG("Ignoring group model change");
    } else {
        // Save the info whenever group model changes
        const bool wasBusy = busy();
        sortModel();
        saveInfo();
        if (busy() != wasBusy) {
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void
FoilPicsModel::Private::onGroupRenamed(
    int,
    const FoilPicsGroupModel::Group& aGroup)
{
    const QString groupId(aGroup.iId);
    HDEBUG("Group" << groupId << "renamed into" << aGroup.iName);
    const int n = iData.count();
    QList<int> changedRows;

    for (int i = 0; i < n; i++) {
        ModelData* data = iData.at(i);
        if (data->iGroupId == groupId) {
            changedRows.append(i);
        }
    }
    if (!changedRows.isEmpty()) {
        dataChanged(changedRows, ModelData::GroupNameRole);
    }
}

void
FoilPicsModel::Private::saveInfo()
{
    // N.B. This method may change the busy state but doesn't queue
    // BusyChanged signal, it's done by the caller.
    if (iSaveInfoTask) iSaveInfoTask->release(this);
    iSaveInfoTask = new Task::SaveInfo(iThreadPool,
        ModelInfo(iData, iGroupModel->groups()),
        iFoilPicsDir, iPrivateKey, iPublicKey);
    iSaveInfoTask->submit(this, SLOT(onSaveInfoDone()));
}

void
FoilPicsModel::Private::onSaveInfoDone()
{
    HDEBUG("Done");
    if (sender() == iSaveInfoTask) {
        iSaveInfoTask->release(this);
        iSaveInfoTask = Q_NULLPTR;
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void
FoilPicsModel::Private::generate(
    int aBits,
    const QString& aPassword)
{
    const bool wasBusy = busy();

    if (iGenerateKeyTask) iGenerateKeyTask->release(this);
    iGenerateKeyTask = new Task::GenerateKey(iThreadPool, iFoilKeyFile,
        aBits, aPassword);
    iGenerateKeyTask->submit(this, SLOT(onGenerateKeyTaskDone()));
    setFoilState(FoilGeneratingKey);
    if (!wasBusy) {
        // We know we are busy now
        queueSignal(SignalBusyChanged);
    }
    emitQueuedSignals();
}

void
FoilPicsModel::Private::onGenerateKeyTaskDone()
{
    HDEBUG("Got a new key");
    HASSERT(sender() == iGenerateKeyTask);
    if (iGenerateKeyTask->iPrivateKey) {
        setKeys(iGenerateKeyTask->iPrivateKey, iGenerateKeyTask->iPublicKey);
        setFoilState(FoilPicsReady);
    } else {
        setKeys(Q_NULLPTR);
        setFoilState(FoilKeyError);
    }
    iGenerateKeyTask->release(this);
    iGenerateKeyTask = Q_NULLPTR;
    if (!busy()) {
        // We know we were busy when we received this signal
        queueSignal(SignalBusyChanged);
    }
    parentObject()->keyGenerated();
    emitQueuedSignals();
}

void
FoilPicsModel::Private::lock(
    bool aTimeout)
{
    // Cancel whatever we are doing
    const bool wasBusy = busy();

    if (iSaveInfoTask) {
        iSaveInfoTask->release(this);
        iSaveInfoTask = Q_NULLPTR;
    }
    if (iDecryptPicsTask) {
        iDecryptPicsTask->release(this);
        iDecryptPicsTask = Q_NULLPTR;
    }

    int i;
    for (i = 0; i < iEncryptTasks.count(); i++) {
        iEncryptTasks.at(i)->release(this);
    }
    for (i = 0; i < iImageRequestTasks.count(); i++) {
        iImageRequestTasks.at(i)->release(this);
    }

    iEncryptTasks.clear();
    iImageRequestTasks.clear();
    // Disconnect the signals before clearing the model
    disconnectGroupModel();
    // Destroy decrypted pictures
    if (!iData.isEmpty()) {
        FoilPicsModel* model = parentObject();

        model->beginRemoveRows(QModelIndex(), 0, iData.count()-1);
        qDeleteAll(iData);
        iData.clear();
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
    }
    clearGroupModel();
    if (busy() != wasBusy) {
        queueSignal(SignalBusyChanged);
    }
    if (iPrivateKey) {
        // Throw the keys away
        setKeys(Q_NULLPTR);
        setFoilState(aTimeout ? FoilLockedTimedOut : FoilLocked);
        HDEBUG("Locked");
    } else {
        HDEBUG("Nothing to lock, there's no key yet!");
    }
}

bool
FoilPicsModel::Private::unlock(
    const QString& aPassword)
{
    GError* error = Q_NULLPTR;
    const QByteArray path(iFoilKeyFile.toUtf8());
    bool ok = false;

    HDEBUG(path.constData());

    // First make sure that it's encrypted
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), NULL, &error);

    if (key) {
        HWARN("Key not encrypted");
        setFoilState(FoilKeyNotEncrypted);
        foil_private_key_unref(key);
    } else if (error->domain == FOIL_ERROR) {
        if (error->code == FOIL_ERROR_KEY_ENCRYPTED) {
            // Then try to decrypt it
            const QByteArray password(aPassword.toUtf8());

            g_clear_error(&error);
            key = foil_private_key_decrypt_from_file
                (FOIL_KEY_RSA_PRIVATE, path.constData(),
                    password.constData(), &error);
            if (key) {
                HDEBUG("Password accepted, thank you!");
                setKeys(key);
                // Now that we know the key, decrypt the pictures
                if (iDecryptPicsTask) iDecryptPicsTask->release(this);
                iDecryptPicsTask = new DecryptPicsTask(iThreadPool,
                    iFoilPicsDir, iPrivateKey, iPublicKey, iThumbSize);
                clearModel();
                clearGroupModel();
                connect(iDecryptPicsTask,
                    SIGNAL(groupsDecrypted(FoilPicsGroupModel::GroupList)),
                    SLOT(onGroupsDecrypted(FoilPicsGroupModel::GroupList)),
                    Qt::QueuedConnection);
                connect(iDecryptPicsTask,
                    SIGNAL(progress(DecryptPicsTask::Progress::Ptr)),
                    SLOT(onDecryptPicsProgress(DecryptPicsTask::Progress::Ptr)),
                    Qt::QueuedConnection);
                iDecryptPicsTask->submit(this, SLOT(onDecryptPicsTaskDone()));
                setFoilState(FoilDecrypting);
                foil_private_key_unref(key);
                ok = true;
            } else {
                HDEBUG("Wrong password");
                g_error_free(error);
                setFoilState(FoilLocked);
            }
        } else {
            HWARN("Key invalid:" << error->message);
            g_error_free(error);
            setFoilState(FoilKeyInvalid);
        }
    } else {
        HWARN(error->message);
        g_error_free(error);
        setFoilState(FoilKeyMissing);
    }
    return ok;
}

void
FoilPicsModel::Private::encryptFile(
    const QString& aFile,
    QVariantMap aMetaData)
{
    // Missing coordinates are sometimes represented as all zeros
    const double latitude = aMetaData.value(MetaLatitude).toDouble();
    const double longitude = aMetaData.value(MetaLongitude).toDouble();
    const double altitude = aMetaData.value(MetaAltitude).toDouble();

    if (latitude == 0.0 && longitude == 0.0 && altitude == 0.0) {
        // Bogus coordinates, don't store them
        aMetaData.remove(MetaLatitude);
        aMetaData.remove(MetaLongitude);
        aMetaData.remove(MetaAltitude);
    }

    Task::Encrypt* task = new Task::Encrypt(iThreadPool, aFile, iFoilPicsDir,
        iPrivateKey, iPublicKey, iThumbSize, aMetaData);

    iEncryptTasks.append(task);
    task->submit(this, SLOT(onEncryptTaskDone()));
}

bool
FoilPicsModel::Private::encrypt(
    const QUrl& aUrl,
    const QVariantMap& aMetaData)
{
    if (iPrivateKey && aUrl.isLocalFile()) {
        const bool wasBusy = busy();

        encryptFile(aUrl.toLocalFile(), aMetaData);
        if (!wasBusy) {
            // We know we are busy now
            queueSignal(SignalBusyChanged);
        }
        return true;
    }
    return false;
}

void
FoilPicsModel::Private::encryptFiles(
    QAbstractItemModel* aModel,
    const QList<int>& aRows)
{
    HASSERT(aModel);
    if (iPrivateKey && !aRows.isEmpty()) {
        // URL is absolutely required
        const FoilPicsRole urlRole(aModel, MetaUrl);

        if (urlRole.isValid()) {
            int submitted = 0;
            const bool wasBusy = busy();
            FoilPicsRole orientationRole(aModel, MetaOrientation);
            FoilPicsRole imageDateRole(aModel, MetaImageDate);
            FoilPicsRole cameraManufacturerRole(aModel, MetaCameraManufacturer);
            FoilPicsRole cameraModelRole(aModel, MetaCameraModel);
            FoilPicsRole latitudeRole(aModel, MetaLatitude);
            FoilPicsRole longitudeRole(aModel, MetaLongitude);
            FoilPicsRole altitudeRole(aModel, MetaAltitude);
            const FoilPicsRole* roles[] = {&orientationRole, &imageDateRole,
                &cameraManufacturerRole, &cameraModelRole, &latitudeRole,
                &longitudeRole, &altitudeRole
            };

            // Start from the end of the list, in an attempt to disturb
            // the model less
            for (int i = aRows.count()-1; i >= 0; i--) {
                const int row = aRows.at(i);
                QUrl url(urlRole.valueAt(row).toUrl());

                if (url.isLocalFile()) {
                    // Pull metadata out of the model
                    QVariantMap metadata;

                    for (uint k = 0; k < G_N_ELEMENTS(roles); k++) {
                        const FoilPicsRole* role = roles[k];
                        QVariant value(role->valueAt(row));

                        if (value.isValid()) {
                            metadata.insert(role->name(), value);
                        }
                    }
                    // Encrypt this file
                    encryptFile(url.toLocalFile(), metadata);
                    submitted++;
                } else {
                    HWARN("Can't encrypt" << url);
                }
            }
            if (!wasBusy && submitted) {
                // We know we are busy now
                queueSignal(SignalBusyChanged);
            }
        }
    }
}

void
FoilPicsModel::Private::onEncryptTaskDone()
{
    Task::Encrypt* task = qobject_cast<Task::Encrypt*>(sender());

    HVERIFY(iEncryptTasks.removeAll(task));
    HDEBUG("Encrypted" << qPrintable(task->iSourceFile));
    if (task->iData) {
        // Steal model data from the task
        insertModelData(task->iData);
        task->iData = Q_NULLPTR;
        saveInfo();
    }

    const QUrl mediaUrl(QUrl::fromLocalFile(task->iSourceFile));

    task->release(this);
    if (!busy()) {
        // We know we were busy when we received this signal
        queueSignal(SignalBusyChanged);
    }
    Q_EMIT parentObject()->mediaDeleted(mediaUrl);
    emitQueuedSignals();
}

void
FoilPicsModel::Private::decryptAt(
    int aIndex)
{
    ModelData* data = dataAt(aIndex);

    if (data && !data->iDecryptTask) {
        const bool wasBusy = busy();

        HDEBUG("About to decrypt" << qPrintable(data->iPath));
        data->iDecryptTask = new Task::Decrypt(iThreadPool, data,
            iPrivateKey, iPublicKey);
        data->iDecryptTask->submit(this, SLOT(onDecryptTaskDone()));
        if (!wasBusy) {
            // We know we are busy now
            queueSignal(SignalBusyChanged);
        }
        queueSignal(SignalDecryptionStarted);
    }
}

void
FoilPicsModel::Private::decryptTaskDone(
    Task::Decrypt* aTask,
    bool aLast)
{
    if (aTask) {
        Task::Decrypt* task = qobject_cast<Task::Decrypt*>(sender());
        ModelData* data = task->iData;

        data->iDecryptTask = Q_NULLPTR;
        task->iData = Q_NULLPTR;
        task->release(this);
        const bool wasConnected = disconnectGroupModel();
        destroyItemAt(iData.indexOf(data));
        if (wasConnected) connectGroupModel();
        if (aLast) {
            saveInfo();
        }
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void
FoilPicsModel::Private::onDecryptTaskDone()
{
    decryptTaskDone(qobject_cast<Task::Decrypt*>(sender()), true);
}

void
FoilPicsModel::Private::decryptFiles(
    QList<int> aRows)
{
    if (!iData.isEmpty() && !aRows.isEmpty()) {
        const bool wasBusy = busy();
        ModelData* data;

        HDEBUG("Decrypting" << aRows.count() << "picture(s)");

        // Start from the last picture
        qSort(aRows);
        for (int i = aRows.count() - 1; i > 0; i--) {
            data = dataAt(aRows.at(i));
            if (data && !data->iDecryptTask) {
                data->iDecryptTask = new Task::Decrypt(iThreadPool, data,
                    iPrivateKey, iPublicKey);
                data->iDecryptTask->submit(this, SLOT(onDecryptAllProgress()));
            }
        }
        // The last onDecryptTaskDone will reset the image info
        data = dataAt(aRows.first());
        if (data && !data->iDecryptTask) {
            data->iDecryptTask = new Task::Decrypt(iThreadPool, data,
                iPrivateKey, iPublicKey);
            data->iDecryptTask->submit(this, SLOT(onDecryptTaskDone()));
        }
        if (busy() != wasBusy) {
            queueSignal(SignalBusyChanged);
        }
        queueSignal(SignalDecryptionStarted);
    }
}

void
FoilPicsModel::Private::decryptAll()
{
    if (!iData.isEmpty()) {
        ModelData* data;
        const bool wasBusy = busy();

        HDEBUG("Decrypting all" << iData.count() << "picture(s)");
        // Start from the last picture
        for (int i = iData.count() - 1; i > 0; i--) {
            data = iData.at(i);
            if (!data->iDecryptTask) {
                data->iDecryptTask = new Task::Decrypt(iThreadPool, data,
                    iPrivateKey, iPublicKey);
                data->iDecryptTask->submit(this, SLOT(onDecryptAllProgress()));
            }
        }
        // The last onDecryptTaskDone will reset the image info
        data = iData.first();
        if (!data->iDecryptTask) {
            data->iDecryptTask = new Task::Decrypt(iThreadPool, data,
                iPrivateKey, iPublicKey);
            data->iDecryptTask->submit(this, SLOT(onDecryptTaskDone()));
        }
        if (busy() != wasBusy) {
            queueSignal(SignalBusyChanged);
        }
        queueSignal(SignalDecryptionStarted);
    }
}

void
FoilPicsModel::Private::onDecryptAllProgress()
{
    decryptTaskDone(qobject_cast<Task::Decrypt*>(sender()), false);
}

void
FoilPicsModel::Private::onGroupsDecrypted(
    FoilPicsGroupModel::GroupList aGroups)
{
    const bool wasConnected = disconnectGroupModel();

    HDEBUG(aGroups.count() << "group(s)");
    iGroupModel->setGroups(aGroups);
    if (wasConnected) connectGroupModel();
}

void
FoilPicsModel::Private::onDecryptPicsProgress(
    DecryptPicsTask::Progress::Ptr aProgress)
{
    if (aProgress && aProgress->iTask == iDecryptPicsTask) {
        // Transfer ownership of this ModelData to the model
        insertModelData(aProgress->iModelData);
        aProgress->iModelData = Q_NULLPTR;
    }
    emitQueuedSignals();
}

void
FoilPicsModel::Private::onDecryptPicsTaskDone()
{
    HDEBUG(iData.count() << "picture(s) decrypted");
    if (sender() == iDecryptPicsTask) {
        const bool infoUpdated = iDecryptPicsTask->iSaveInfo;

        iDecryptPicsTask->release(this);
        iDecryptPicsTask = Q_NULLPTR;
        if (iFoilState == FoilDecrypting) {
            setFoilState(FoilPicsReady);
        }
        if (infoUpdated) saveInfo();
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
    }
    emitQueuedSignals();
}

void
FoilPicsModel::Private::setTitleAt(
    int aIndex,
    const QString& aTitle)
{
    ModelData* data = dataAt(aIndex);

    if (data) {
        QString title(aTitle.isEmpty() ? data->iDefaultTitle : aTitle);

        if (data->iTitle != title) {
            data->iTitle = title;
            data->updateVariantMap(ModelData::TitleRole);
            dataChanged(aIndex, ModelData::TitleRole);

            HDEBUG("Settings title at" << aIndex << "to" << title);
            const bool wasBusy = busy();

            if (data->iSetTitleTask) {
                HDEBUG("Dropping previous task");
                data->iSetTitleTask->release(this);
                data->iSetTitleTask = Q_NULLPTR;
            }
            data->iSetTitleTask = Task::SetHeader::createTitleTask(iThreadPool,
                iPrivateKey, iPublicKey, data);
            data->iSetTitleTask->submit(this, SLOT(onSetTitleTaskDone()));
            if (!wasBusy) {
                // We know we are busy now
                queueSignal(SignalBusyChanged);
            }
        }
    }
}

bool
FoilPicsModel::Private::sortModel()
{
    FoilPicsModel* model = parentObject();
    ModelData::List data = iData;

    qSort(data.begin(), data.end(), LessThan(this));
    if (data != iData) {
        HDEBUG("list has changed");
        model->beginResetModel();
        iData = data;
        model->endResetModel();
        return true;
    } else {
        // The order didn't change
        return false;
    }
}

bool
FoilPicsModel::Private::setGroupId(
    ModelData* aData,
    const QByteArray& aId)
{
    if (aData->iGroupId != aId) {
        aData->iGroupId = aId;
        aData->updateVariantMap(ModelData::GroupIdRole);
        // Update the encrypted file
        if (aData->iSetGroupTask) {
            aData->iSetGroupTask->release(this);
            aData->iSetGroupTask = Q_NULLPTR;
        }
        aData->iSetGroupTask = Task::SetHeader::createGroupTask(iThreadPool,
            iPrivateKey, iPublicKey, aData);
        aData->iSetGroupTask->submit(this, SLOT(onSetGroupTaskDone()));
        return true;
    }
    return false;
}

void
FoilPicsModel::Private::clearGroup(
    const QByteArray& aId)
{
    if (!aId.isEmpty()) {
        const bool wasBusy = busy();
        const int n = iData.count();
        QList<int> changedRows;

        for (int i = 0; i < n; i++) {
            ModelData* data = iData.at(i);

            if (data->iGroupId == aId) {
                HDEBUG(i << QString::fromLatin1(aId));
                setGroupId(data, QByteArray());
                changedRows.append(i);
            }
        }

        if (!changedRows.isEmpty()) {
            if (!sortModel()) {
                // The order hasn't changed => the model wasn't reset
                dataChanged(changedRows, ModelData::ImageIdRole);
            }
            if (!wasBusy) {
                // We know we are busy now
                queueSignal(SignalBusyChanged);
            }
        }
    }
}

void
FoilPicsModel::Private::setGroupIdAt(
    int aIndex,
    const QByteArray& aId)
{
    // This could be pretty destructive in the sense that it can
    // move the item quite far away from its current position.
    ModelData* data = dataAt(aIndex);

    if (data) {
        const bool wasBusy = busy();

        if (setGroupId(data, aId)) {
            HDEBUG(aIndex << QString::fromLatin1(aId));
            if (!sortModel()) {
                // The order hasn't changed => the model wasn't reset
                dataChanged(aIndex, ModelData::ImageIdRole);
            }
            if (!wasBusy) {
                // We know we are busy now
                queueSignal(SignalBusyChanged);
            }
        }
    }
}

void
FoilPicsModel::Private::setGroupIdForRows(
    QList<int> aRows,
    const QByteArray& aId)
{
    if (!aRows.isEmpty()) {
        const bool wasBusy = busy();

        qSort(aRows);

        QList<int> updatedRows;
        const int n = aRows.count();

        for (int i = 0; i < n; i++) {
            const int row = aRows.at(i);
            ModelData* data = dataAt(row);
            if (setGroupId(data, aId)) {
                updatedRows.append(row);
            }
        }

        if (!updatedRows.isEmpty()) {
            if (!sortModel()) {
                // The order hasn't changed => the model wasn't reset
                dataChanged(updatedRows, ModelData::ImageIdRole);
            }
            if (!wasBusy) {
                // We know we are busy now
                queueSignal(SignalBusyChanged);
            }
        }
    }
}

void
FoilPicsModel::Private::headerUpdateDone(
    Task::SetHeader* task)
{
    // task->iData must be valid because if ModelData were deleted
    // it would cancel the task in the destructor and we wouldn't
    // receive this signal
    ModelData* data = task->iData;
    task->iData = Q_NULLPTR;

    if (task->iOk) {
        data->iThumbFile = task->iNewThumbFile;
        data->iPath = task->iNewPath;

        // Image path changed but source URL didn't because it's derived
        // from the hash of the original file. Just update the path.
        iImageProvider->addImage(data->iImageId, data->iPath);

        // The file size may have changed
        const int size = QFileInfo(data->iPath).size();
        if (data->iEncryptedSize != size) {
            HDEBUG("Encrypted size" << data->iEncryptedSize << "->" << size);
            data->iEncryptedSize = size;
            data->updateVariantMap(ModelData::EncryptedFileSizeRole);
            dataChanged(iData.indexOf(data), ModelData::EncryptedFileSizeRole);
        }
    }

    // There's no need to queue BusyChanged because we were busy when we
    // received this signal and we are still going to be busy after we
    // call saveInfo()
    task->release(this);
    emitQueuedSignals();
    saveInfo();
}

void
FoilPicsModel::Private::onSetTitleTaskDone()
{
    // task->iData must be valid, see comment in headerUpdateDone
    Task::SetHeader* task = qobject_cast<Task::SetHeader*>(sender());

    HDEBUG(QString::fromUtf8(task->iHeaderValue));
    task->iData->iSetTitleTask = Q_NULLPTR;
    headerUpdateDone(task);
}

void
FoilPicsModel::Private::onSetGroupTaskDone()
{
    // task->iData must be valid, see comment in headerUpdateDone
    Task::SetHeader* task = qobject_cast<Task::SetHeader*>(sender());

    HDEBUG(QString::fromUtf8(task->iHeaderValue));
    task->iData->iSetGroupTask = Q_NULLPTR;
    headerUpdateDone(task);
}

void
FoilPicsModel::Private::dataChanged(
    int aIndex,
    ModelData::Role aRole)
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        QVector<int> roles;

        roles.append(aRole);
        FoilPicsModel* model = parentObject();
        const QModelIndex modelIndex(model->index(aIndex));

        Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
    }
}

void
FoilPicsModel::Private::dataChanged(
    const QList<int>& aRows,
    ModelData::Role aRole)
{
    if (!aRows.isEmpty()) {
        QVector<int> roles;
        roles.append(aRole);

        FoilPicsModel* model = parentObject();
        const int n = aRows.count();

        for (int i = 0; i < n; i++) {
            // TODO: coalesce signals for sequential rows
            const int row = aRows.at(i);

            if (row >= 0 && row < iData.count()) {
                QModelIndex modelIndex(model->index(row));
                Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
            }
        }
    }
}

//
// Three threads are involved in fetching the decrypted image:
//
// 1. QQuickPixmapReader calls FoilPicsImageProvider::requestImage on its
//    own thread, which queues "imageRequest" signal to FoilPicsModel and
//    blocks until FoilPicsImageRequest is replied to.
// 2. FoilPicsModel receives the signal on the UI thread and queues
//    ImageRequest task. It's done even if the decrypted data is cached
//    because creating the image from data may take too long for the UI
//    thread.
// 3. ImageRequest task gets executed on yet another worker thread and
//    when it's done, it replies to FoilPicsImageRequest which unblocks
//    QQuickPixmapReader thread and queues the "done" signal to FoilPicsModel.
//
// The "done" signal is finally handled by onImageRequestDone() on the UI
// thread. If caches the freshly decrypted data.
//
void
FoilPicsModel::Private::imageRequest(
    const QString& aPath,
    FoilPicsImageRequest aRequest)
{
    const bool wasBusy = busy();
    QByteArray bytes;
    QString contentType;

    // Check if the decrypted data is cached
    const int index = findPath(aPath);

    if (index >= 0) {
        const ModelData* data = iData.at(index);

        if (!data->iBytes.isEmpty()) {
            bytes = data->iBytes;
            contentType = data->iContentType;
        }
    }

    HDEBUG("Requesting" << qPrintable(aPath));
    Task::ImageRequest* task = new Task::ImageRequest(iThreadPool, aPath,
        bytes, contentType, iPrivateKey, iPublicKey, aRequest);

    iImageRequestTasks.append(task);
    task->submit(this, SLOT(onImageRequestDone()));
    if (!wasBusy) {
        // We know we are busy now
        queueSignal(SignalBusyChanged);
    }
}

void
FoilPicsModel::Private::onImageRequestDone()
{
    Task::ImageRequest* task = qobject_cast<Task::ImageRequest*>(sender());

    HVERIFY(iImageRequestTasks.removeAll(task));
    if (!task->iBytes.isEmpty()) {
        // Cache the decrypted data
        int index = findPath(task->iPath);

        if (index >= 0) {
            ModelData* data = iData.at(index);

            data->iBytes = task->iBytes;
            HDEBUG(qPrintable(data->iPath) << data->iBytes.count() << "bytes");
            while (tooMuchDataDecrypted() && dropDecryptedData(index));
        }
    }
    task->release(this);
    if (!busy()) {
        // We know we were busy when we received this signal
        queueSignal(SignalBusyChanged);
    }
    emitQueuedSignals();
}

int
FoilPicsModel::Private::findPath(
    const QString& aPath)
{
    const int n = iData.count();
    for (int i = 0; i < n; i++) {
        if (iData.at(i)->iPath == aPath) {
            return i;
        }
    }
    return -1;
}

bool
FoilPicsModel::Private::dropDecryptedData(
    int aDontTouch)
{
    int indexToDrop = -1;
    int maxDistance = -1;
    const int n = iData.count();

    // Find the index furthest away from the one we don't touch
    for (int i = 0; i < n; i++) {
        if (i != aDontTouch && !iData.at(i)->iBytes.isEmpty()) {
            // The distance is calculated assuming that the list is circular
            const int distance = (aDontTouch > indexToDrop) ?
                qMin(aDontTouch - i, i + n - aDontTouch) :
                qMin(i - aDontTouch, aDontTouch + n - i);
            if (indexToDrop < 0 || distance > maxDistance) {
                indexToDrop = i;
                maxDistance = distance;
            }
        }
    }

    if (indexToDrop >= 0) {
        ModelData* data = iData.at(indexToDrop);
        HDEBUG("Dropping"<< qPrintable(data->iPath) << "at" << indexToDrop );
        data->iBytes = QByteArray();
        return true;
    } else {
        return false;
    }
}

bool
FoilPicsModel::Private::tooMuchDataDecrypted()
{
    int count = 0;
    size_t totalSize = 0;
    const int n = iData.count();

    for (int i = 0; i < n; i++) {
        ModelData* data = iData.at(i);
        if (!data->iBytes.isEmpty()) {
            count++;
            totalSize += data->iBytes.size();
            if (count > 1 && totalSize > iMaxBytesToDecrypt)  {
                return true;
            }
        }
    }
    return false;
}

bool
FoilPicsModel::Private::busy() const
{
    if (iCheckPicsTask ||
        iSaveInfoTask ||
        iGenerateKeyTask ||
        iDecryptPicsTask ||
        !iEncryptTasks.isEmpty() ||
        !iImageRequestTasks.isEmpty()) {
        return true;
    } else {
        // Hmm... This is not very scalable
        const int n = iData.count();

        for (int i = 0; i < n; i++) {
            const ModelData* data = iData.at(i);

            if (data->iDecryptTask ||
                data->iSetTitleTask ||
                data->iSetGroupTask) {
                return true;
            }
        }
        return false;
    }
}

// ==========================================================================
// FoilPicsModel
// ==========================================================================

FoilPicsModel::FoilPicsModel(
    QObject* aParent) :
    QAbstractListModel(aParent),
    iPrivate(new Private(this))
{
    // FoilPicsGroupModel has to be created after iPrivate pointer
    // has been set up because it will call FoilPicsModel::rowCount
    // from the constructor.
    iPrivate->iGroupModel = new FoilPicsGroupModel(this);
}

bool
FoilPicsModel::busy() const
{
    return iPrivate->busy();
}

bool
FoilPicsModel::keyAvailable() const
{
    return iPrivate->iPrivateKey != Q_NULLPTR;
}

FoilPicsModel::FoilState
FoilPicsModel::foilState() const
{
    return iPrivate->iFoilState;
}

bool
FoilPicsModel::mayHaveEncryptedPictures() const
{
    return iPrivate->iMayHaveEncryptedPictures;
}

QSize
FoilPicsModel::thumbnailSize() const
{
    return iPrivate->iThumbSize;
}

int
FoilPicsModel::groupIdRole()
{
    return ModelData::GroupIdRole;
}

QAbstractItemModel*
FoilPicsModel::groupModel()
{
    return iPrivate->iGroupModel;
}

void
FoilPicsModel::clearGroup(
    QByteArray aId)
{
    HDEBUG(QString::fromLatin1(aId));
    iPrivate->clearGroup(aId);
    iPrivate->emitQueuedSignals();
}

QHash<int,QByteArray>
FoilPicsModel::roleNames() const
{
    QHash<int,QByteArray> roles;

    #define ROLE(X,x) roles.insert(ModelData::X##Role, #x);
    FOILPICS_ROLES(ROLE)
    #undef ROLE
    return roles;
}

int
FoilPicsModel::rowCount(
    const QModelIndex& aParent) const
{
    return iPrivate->iData.count();
}

QVariant
FoilPicsModel::data(
    const QModelIndex& aIndex,
    int aRole) const
{
    const ModelData* data = iPrivate->dataAt(aIndex.row());

    if (data) {
        if (aRole == ModelData::GroupNameRole) {
            return iPrivate->iGroupModel->groupName(data->iGroupId);
        } else {
            return data->get((ModelData::Role)aRole);
        }
    }
    return QVariant();
}

void
FoilPicsModel::setThumbnailSize(
    QSize aSize)
{
    if (iPrivate->iThumbSize != aSize) {
        iPrivate->iThumbSize = aSize;
        HDEBUG(aSize);
        // Re-generate the thumbnails?
        Q_EMIT thumbnailSizeChanged();
    }
}

void
FoilPicsModel::removeAt(
    int aIndex)
{
    HDEBUG(aIndex);
    iPrivate->removeAt(aIndex);
    iPrivate->emitQueuedSignals();
}

void
FoilPicsModel::removeFiles(
    QList<int> aRows)
{
    HDEBUG(aRows);
    iPrivate->removeFiles(aRows);
    iPrivate->emitQueuedSignals();
}

QVariantMap
FoilPicsModel::get(int aIndex) const
{
    const ModelData* data = iPrivate->dataAt(aIndex);

    HDEBUG(aIndex);
    if (data) {
        QVariantMap map(data->toVariantMap());

        map.insert(ModelData::RoleNameGroupName,
            iPrivate->iGroupModel->groupName(data->iGroupId));
        return map;
    }
    return QVariantMap();
}

void
FoilPicsModel::decryptFiles(
    QList<int> aRows)
{
    HDEBUG(aRows);
    iPrivate->decryptFiles(aRows);
    iPrivate->emitQueuedSignals();
}

void
FoilPicsModel::decryptAt(
    int aIndex)
{
    HDEBUG(aIndex);
    iPrivate->decryptAt(aIndex);
    iPrivate->emitQueuedSignals();
}

void
FoilPicsModel::decryptAll()
{
    HDEBUG("Decrypting all");
    iPrivate->decryptAll();
    iPrivate->emitQueuedSignals();
}

bool
FoilPicsModel::encryptFile(
    QUrl aUrl,
    QVariantMap aMetaData)
{
    const bool ok = iPrivate->encrypt(aUrl, aMetaData);
    iPrivate->emitQueuedSignals();
    return ok;
}

void
FoilPicsModel::encryptFiles(
    QObject* aModel,
    QList<int> aList)
{
    iPrivate->encryptFiles(qobject_cast<QAbstractItemModel*>(aModel), aList);
    iPrivate->emitQueuedSignals();
}

void
FoilPicsModel::lock(
    bool aTimeout)
{
    iPrivate->lock(aTimeout);
    iPrivate->emitQueuedSignals();
}

bool
FoilPicsModel::unlock(
    QString aPassword)
{
    const bool ok = iPrivate->unlock(aPassword);

    iPrivate->emitQueuedSignals();
    return ok;
}

bool
FoilPicsModel::checkPassword(
    QString aPassword)
{
    return iPrivate->checkPassword(aPassword);
}

bool
FoilPicsModel::changePassword(
    QString aOld,
    QString aNew)
{
    const bool ok = iPrivate->changePassword(aOld, aNew);

    iPrivate->emitQueuedSignals();
    return ok;
}

void
FoilPicsModel::generateKey(
    int aBits,
    QString aPassword)
{
    iPrivate->generate(aBits, aPassword);
    iPrivate->emitQueuedSignals();
}

void
FoilPicsModel::imageRequest(
    QString aPath,
    FoilPicsImageRequest aRequest)
{
    iPrivate->imageRequest(aPath, aRequest);
    iPrivate->emitQueuedSignals();
}

int
FoilPicsModel::groupIndexAt(
    int aIndex) const
{
    const ModelData* data = iPrivate->dataAt(aIndex);

    if (data) {
        return iPrivate->iGroupModel->indexOfGroup(data->iGroupId);
    } else {
        return -1;
    }
}

void
FoilPicsModel::setGroupId(
    QString aImageId,
    QString aId)
{
    const int i = iPrivate->find(aImageId);

    if (i >= 0) {
        iPrivate->setGroupIdAt(i, aId.toLatin1());
        iPrivate->emitQueuedSignals();
    } else {
        HWARN("Invalid image id" << aImageId);
    }
}

void
FoilPicsModel::setGroupIdAt(
    int aIndex,
    QString aId)
{
    iPrivate->setGroupIdAt(aIndex, aId.toLatin1());
    iPrivate->emitQueuedSignals();
}

void
FoilPicsModel::setGroupIdForRows(
    QList<int> aRows,
    QString aId)
{
    HDEBUG(aRows << aId);
    iPrivate->setGroupIdForRows(aRows, aId.toLatin1());
    iPrivate->emitQueuedSignals();
}

void
FoilPicsModel::setTitleAt(
    int aIndex,
    QString aTitle)
{
    iPrivate->setTitleAt(aIndex, aTitle);
    iPrivate->emitQueuedSignals();
}

#include "FoilPicsModel.moc"
