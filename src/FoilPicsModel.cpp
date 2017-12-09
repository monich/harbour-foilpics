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

#include "FoilPicsModel.h"
#include "FoilPicsTask.h"
#include "FoilPicsThumbnailProvider.h"
#include "FoilPicsImageProvider.h"
#include "FoilPicsFileUtil.h"

#include "foil_private_key.h"
#include "foil_digest.h"
#include "foil_output.h"
#include "foil_random.h"
#include "foil_util.h"
#include "foilmsg.h"

#include "HarbourDebug.h"

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

/* Thumbnail specific headers */
#define HEADER_THUMB_FULL_WIDTH     "Full-Width"
#define HEADER_THUMB_FULL_HEIGHT    "Full-Height"

#define INFO_FILE ".info"
#define INFO_CONTENTS "FoilPics"
#define INFO_ORDER_HEADER "Order"
#define INFO_ORDER_DELIMITER   ','
#define INFO_ORDER_DELIMITER_S ","
#define INFO_ORDER_THUMB_DELIMITER ':'

// Keys for metadata passed to encryptFile:
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
    role(Title, title) \
    role(DefaultTitle, defaultTitle) \
    role(ImageWidth, imageWidth) \
    role(ImageHeight, imageHeight)

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

    struct FormatMap {
        const char* contentType;
        const char* imageFormat;
    };

    ModelData(QString aOriginalPath, int aOriginalSize, QSize aFullDimensions,
        GBytes* aDigest, QString aPath, QString aThumbFile, QImage aThumbImage,
        QString aTitle, const char* aContentType, QDateTime aSortTime,
        int aOrientation, QString aCameraManufacturer, QString aCameraModel,
        const char* aLatitude, const char* aLongitude, const char* aAltitude,
        QDateTime aImageDate);
    ~ModelData();

    QVariant get(Role aRole) const;

    static QString defaultTitle(QString aPath);
    static QString defaultTitle(QFileInfo aFileInfo);
    static QImage thumbnail(const QImage aImage, QSize aSize, int aRotate);
    static const char* format(const char* aContentType);
    static bool lessThan(ModelData* aData1, ModelData* aData2);
    static int compareFormatMap(const void* aElem1, const void* aElem2);

    static double* toDouble(const char* aString);
    static QString headerString(const FoilMsg* aMsg, const char* aKey);
    static QDateTime headerTime(const FoilMsg* aMsg, const char* aKey);
    static QDateTime headerSortTime(const FoilMsg* aMsg);
    static int headerInt(const FoilMsg* aMsg, const char* aKey, int aDef = 0);

    static ModelData* fromFoilMsg(FoilMsg* aFoilMsg, QString aOriginalPath,
        QSize aFullDimensions, QString aPath, QString aThumbFile,
        QImage aThumbImage, const char* aContentType, int aOrientation);

public:
    QString iPath;
    QString iFileName;
    QString iThumbFile; // Without path
    QString iDefaultTitle;
    QString iTitle;
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
    FoilPicsTask* iDecryptTask;
    FoilPicsTask* iSetTitleTask;
};

FoilPicsModel::ModelData::ModelData(QString aOriginalPath, int aOriginalSize,
    QSize aFullDimensions, GBytes* aDigest, QString aPath, QString aThumbFile,
    QImage aThumbImage, QString aTitle, const char* aContentType,
    QDateTime aSortTime, int aOrientation, QString aCameraManufacturer,
    QString aCameraModel, const char* aLatitude, const char* aLongitude,
    const char* aAltitude, QDateTime aImageDate) :
    iPath(aPath), iThumbFile(aThumbFile), iTitle(aTitle),
    iFullDimensions(aFullDimensions), iThumbnail(aThumbImage),
    iSortTime(aSortTime), iOriginalSize(aOriginalSize),
    iOrientation(aOrientation), iCameraManufacturer(aCameraManufacturer),
    iCameraModel(aCameraModel), iImageDate(aImageDate),
    iLatitude(toDouble(aLatitude)), iLongitude(toDouble(aLongitude)),
    iAltitude(toDouble(aAltitude)), iDecryptTask(NULL), iSetTitleTask(NULL)
{
    QFileInfo fileInfo(aOriginalPath);
    iFileName = fileInfo.fileName();
    iEncryptedSize = QFileInfo(aPath).size();
    iDefaultTitle = defaultTitle(fileInfo);
    if (iTitle.isEmpty()) iTitle = iDefaultTitle;
    if (aContentType) iContentType = QLatin1String(aContentType);

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
    delete iLatitude;
    delete iLongitude;
    delete iAltitude;
}

FoilPicsModel::ModelData* FoilPicsModel::ModelData::fromFoilMsg(FoilMsg* aMsg,
    QString aOriginalPath, QSize aFullDimensions, QString aPath,
    QString aThumbFile, QImage aThumbImage, const char* aContentType,
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
        headerTime(aMsg, HEADER_IMAGE_DATE));
    g_bytes_unref(digest);
    return data;
}

QVariant FoilPicsModel::ModelData::get(Role aRole) const
{
    switch (aRole) {
    case ModelData::UrlRole: return iImageSource;
    case ModelData::OriginalFileSizeRole: return iOriginalSize ?
            QVariant::fromValue(iOriginalSize) : QVariant();
    case ModelData::EncryptedFileSizeRole: return iEncryptedSize ?
            QVariant::fromValue(iEncryptedSize) : QVariant();
    case ModelData::ThumbnailRole: return iThumbSource;
    case ModelData::OrientationRole: return iOrientation;
    case ModelData::CameraManufacturerRole: return iCameraManufacturer;
    case ModelData::CameraModelRole: return iCameraModel;
    case ModelData::LatitudeRole: return iLatitude ?
            QVariant::fromValue(*iLatitude) : QVariant();
    case ModelData::LongitudeRole: return iLongitude ?
            QVariant::fromValue(*iLongitude) : QVariant();
    case ModelData::AltitudeRole: return iAltitude ?
            QVariant::fromValue(*iAltitude) : QVariant();
    case ModelData::ImageDateRole: return iImageDate.isValid() ?
            QVariant::fromValue(iImageDate) : QVariant();
    case ModelData::MimeTypeRole: return iContentType;
    case ModelData::TitleRole: return iTitle;
    case ModelData::DefaultTitleRole: return iDefaultTitle;
    case ModelData::FileNameRole: return iFileName;
    case ModelData::ImageWidthRole: return iFullDimensions.width();
    case ModelData::ImageHeightRole: return iFullDimensions.height();
    // No default to make sure that we get "warning: enumeration value
    // not handled in switch" if we forget to handle a real role.
    case ModelData::FirstRole:
    case ModelData::LastRole:
        break;
    }
    return QVariant();
}

double* FoilPicsModel::ModelData::toDouble(const char* aString)
{
    if (aString && aString[0]) {
        double q = g_ascii_strtod(aString, NULL);
        if (!errno) {
            return new double(q);
        }
    }
    return NULL;
}

QString FoilPicsModel::ModelData::defaultTitle(QString aPath)
{
    return defaultTitle(QFileInfo(aPath));
}

QString FoilPicsModel::ModelData::defaultTitle(QFileInfo aFileInfo)
{
    return aFileInfo.baseName();
}

QImage FoilPicsModel::ModelData::thumbnail(const QImage aImage, QSize aSize,
    int aRotate)
{
    QImage cropped;
    const QSize imageSize(aImage.size());
    const Qt::TransformationMode txMode(Qt::SmoothTransformation);
    if (imageSize.width()*aSize.height() > aSize.width()*imageSize.height()) {
        QImage scaled(aImage.scaledToHeight(aSize.height(), txMode));
        const int x = (scaled.width() - aSize.width())/2;
        cropped = scaled.copy(x, 0, aSize.width(), aSize.height());
    } else {
        QImage scaled(aImage.scaledToWidth(aSize.width(), txMode));
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

bool FoilPicsModel::ModelData::lessThan(ModelData* aData1, ModelData* aData2)
{
    // Most recent first
    return aData1->iSortTime > aData2->iSortTime;
}

const char* FoilPicsModel::ModelData::format(const char* aContentType)
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
        key.imageFormat = NULL;
        const FormatMap* res = (const FormatMap*)bsearch(&key, formatMap,
            G_N_ELEMENTS(formatMap), sizeof(formatMap[0]), compareFormatMap);
        if (res) {
            return res->imageFormat;
        }
        HDEBUG("Unknown content type" << aContentType);
    }
    return NULL;
}

int FoilPicsModel::ModelData::compareFormatMap(const void* aElem1,
    const void* aElem2)
{
    return strcmp(((const FormatMap*)aElem1)->contentType,
        ((const FormatMap*)aElem2)->contentType);
}

int FoilPicsModel::ModelData::headerInt(const FoilMsg* aMsg,
    const char* aKey, int aDefaultValue)
{
    int result = aDefaultValue;
    const char* str = foilmsg_get_value(aMsg, aKey);
    if (str && str[0]) {
        gboolean ok;
        char *str2 = g_strstrip(g_strdup(str));
        char *end = str2;
        long l;
        errno = 0;
        l = strtol(str2, &end, 0);
        ok = !*end && errno != ERANGE && l >= INT_MIN && l <= INT_MAX;
        if (ok) {
            result = (int)l;
        }
        g_free(str2);
    }
    return result;
}

QString FoilPicsModel::ModelData::headerString(const FoilMsg* aMsg,
    const char* aKey)
{
    const char* value = foilmsg_get_value(aMsg, aKey);
    return (value && value[0]) ? QString(value) : QString();
}

QDateTime FoilPicsModel::ModelData::headerTime(const FoilMsg* aMsg,
    const char* aKey)
{
    const char* value = foilmsg_get_value(aMsg, aKey);
    return value ? QDateTime::fromString(value, Qt::ISODate) : QDateTime();
}

QDateTime FoilPicsModel::ModelData::headerSortTime(const FoilMsg* aMsg)
{
    QDateTime time = headerTime(aMsg, HEADER_IMAGE_DATE);
    return time.isValid() ? time : headerTime(aMsg, HEADER_MODIFICATION_TIME);
}

// ==========================================================================
// FoilPicsModel::ModelInfo
// ==========================================================================

class FoilPicsModel::ModelInfo {
public:
    ModelInfo() {}
    ModelInfo(const FoilMsg* msg);
    ModelInfo(const QList<ModelData*> aData);
    ModelInfo(const ModelInfo& aInfo);

    static ModelInfo load(QString aDir, FoilPrivateKey* aPrivate,
        FoilKey* aPublic);

    void save(QString aDir, FoilPrivateKey* aPrivate, FoilKey* aPublic);
    ModelInfo& operator = (const ModelInfo& aInfo);

public:
    QStringList iOrder;
    QHash<QString,QString> iThumbMap;
};

FoilPicsModel::ModelInfo::ModelInfo(const ModelInfo& aInfo) :
    iOrder(aInfo.iOrder), iThumbMap(aInfo.iThumbMap)
{
}

FoilPicsModel::ModelInfo& FoilPicsModel::ModelInfo::operator=(const ModelInfo& aInfo)
{
    iOrder = aInfo.iOrder;
    iThumbMap = aInfo.iThumbMap;
    return *this;
}

FoilPicsModel::ModelInfo::ModelInfo(const QList<ModelData*> aData)
{
    const int n = aData.count();
    for (int i=0; i<n; i++) {
        const ModelData* data = aData.at(i);
        QString name(QFileInfo(data->iPath).fileName());
        iOrder.append(name);
        if (!data->iThumbFile.isEmpty()) {
            iThumbMap.insert(name, data->iThumbFile);
        }
    }
}

FoilPicsModel::ModelInfo::ModelInfo(const FoilMsg* msg)
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
}

FoilPicsModel::ModelInfo FoilPicsModel::ModelInfo::load(QString aDir,
    FoilPrivateKey* aPrivate, FoilKey* aPublic)
{
    ModelInfo info;
    QString fullPath(aDir + "/" INFO_FILE);
    const QByteArray path(fullPath.toUtf8());
    const char* fname = path.constData();
    HDEBUG("Loading" << fname);
    FoilMsg* msg = foilmsg_decrypt_file(aPrivate, fname, NULL);
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

void FoilPicsModel::ModelInfo::save(QString aDir, FoilPrivateKey* aPrivate,
    FoilKey* aPublic)
{
    QString fullPath(aDir + "/" INFO_FILE);
    const QByteArray path(fullPath.toUtf8());
    const char* fname = path.constData();
    FoilOutput* out = foil_output_file_new_open(fname);
    if (out) {
        QString buf;
        const int n = iOrder.count();
        for (int i=0; i<n; i++) {
            if (!buf.isEmpty()) buf += QChar(INFO_ORDER_DELIMITER);
            QString img(iOrder.at(i));
            QString thumb(iThumbMap.value(img));
            buf += img;
            if (!thumb.isEmpty()) {
                buf += QChar(INFO_ORDER_THUMB_DELIMITER);
                buf += thumb;
            }
        }

        const QByteArray order(buf.toUtf8());
        HDEBUG("Saving" << fname);
        HDEBUG(order.constData());

        FoilMsgHeaders headers;
        FoilMsgHeader header;
        header.name = INFO_ORDER_HEADER;
        header.value = order.constData();
        headers.count = 1;
        headers.header = &header;

        FoilMsgEncryptOptions opt;
        memset(&opt, 0, sizeof(opt));
        opt.key_type = ENCRYPT_KEY_TYPE;

        FoilBytes data;
        foil_bytes_from_string(&data, INFO_CONTENTS);
        foilmsg_encrypt(out, &data, NULL, &headers, aPrivate, aPublic,
            &opt, NULL);
        foil_output_unref(out);
    } else {
        HWARN("Failed to open" << fname);
    }
}

// ==========================================================================
// FoilPicsModel::BaseTask
// ==========================================================================

class FoilPicsModel::BaseTask : public FoilPicsTask {
    Q_OBJECT

public:
    BaseTask(QThreadPool* aPool, FoilPrivateKey* aPrivateKey,
        FoilKey* aPublicKey);
    virtual ~BaseTask();

    FoilMsg* decryptAndVerify(QString aFileName);
    FoilMsg* decryptAndVerify(const char* aFileName);
    QString writeThumb(QImage aImage, const FoilMsgHeaders* aHeaders,
        const char* aContentType, QImage aThumb, QString aDestDir);

    static bool removeFile(QString aPath);
    static QImage toImage(const FoilMsg* aMsg);
    static FoilOutput* createFoilFile(QString aDestDir, GString* aOutPath);
    static bool addHeader(FoilMsgHeader* aHeader,
        const FoilMsgHeaders* aHeaders, const char* aKey);

public:
    FoilPrivateKey* iPrivateKey;
    FoilKey* iPublicKey;
};

FoilPicsModel::BaseTask::BaseTask(QThreadPool* aPool,
    FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey) :
    FoilPicsTask(aPool),
    iPrivateKey(foil_private_key_ref(aPrivateKey)),
    iPublicKey(foil_key_ref(aPublicKey))
{
}

FoilPicsModel::BaseTask::~BaseTask()
{
    foil_private_key_unref(iPrivateKey);
    foil_key_unref(iPublicKey);
}

bool FoilPicsModel::BaseTask::removeFile(QString aPath)
{
    if (!aPath.isEmpty()) {
        if (QFile::remove(aPath)) {
            return true;
        }
        HWARN("Failed to delete" << qPrintable(aPath));
    }
    return false;
}

FoilMsg* FoilPicsModel::BaseTask::decryptAndVerify(QString aFileName)
{
    if (!aFileName.isEmpty()) {
        const QByteArray fileNameBytes(aFileName.toUtf8());
        return decryptAndVerify(fileNameBytes.constData());
    } else{
        return NULL;
    }
}

FoilMsg* FoilPicsModel::BaseTask::decryptAndVerify(const char* aFileName)
{
    if (aFileName) {
        HDEBUG("Decrypting" << aFileName);
        FoilMsg* msg = foilmsg_decrypt_file(iPrivateKey, aFileName, NULL);
        if (msg) {
            if (foilmsg_verify(msg, iPublicKey)) {
                return msg;
            } else {
                HWARN("Could not verify" << aFileName);
            }
            foilmsg_free(msg);
        }
    }
    return NULL;
}

QImage FoilPicsModel::BaseTask::toImage(const FoilMsg* aMsg)
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

bool FoilPicsModel::BaseTask::addHeader(FoilMsgHeader* aHeader,
    const FoilMsgHeaders* aHeaders, const char* aKey)
{
    if (aHeaders) {
        for (uint i=0; i<aHeaders->count; i++) {
            if (!strcmp(aHeaders->header[i].name, aKey)) {
                aHeader->value = aHeaders->header[i].value;
                aHeader->name = aKey;
                return true;
            }
        }
    }
    return false;
}

FoilOutput* FoilPicsModel::BaseTask::createFoilFile(QString aDestDir,
    GString* aOutPath)
{
    // Generate random name for the encrypted file
    FoilOutput* out = NULL;
    const QByteArray dir(aDestDir.toUtf8());
    g_string_truncate(aOutPath, 0);
    g_string_append_len(aOutPath, dir.constData(), dir.size());
    g_string_append_c(aOutPath, '/');
    const gsize prefix_len = aOutPath->len;
    for (int i=0; i<100 && !out; i++) {
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

QString FoilPicsModel::BaseTask::writeThumb(QImage aImage,
    const FoilMsgHeaders* aHeaders, const char* aContentType,
    QImage aThumb, QString aDestDir)
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
            HEADER_TITLE
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
            if (addHeader(header + headers.count, aHeaders, keys[i])) {
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
                iPrivateKey, iPublicKey, &opt, NULL)) {
                thumbName = QFileInfo(dest->str).fileName();
            }
            foil_output_unref(out);
        }
        g_string_free(dest, TRUE);
    }
    return thumbName;
}

// ==========================================================================
// FoilPicsModel::GenerateKeyTask
// ==========================================================================

class FoilPicsModel::GenerateKeyTask : public BaseTask {
    Q_OBJECT

public:
    GenerateKeyTask(QThreadPool* aPool, QString aKeyFile, int aBits,
        QString aPassword);

    virtual void performTask();

public:
    QString iKeyFile;
    int iBits;
    QString iPassword;
};

FoilPicsModel::GenerateKeyTask::GenerateKeyTask(QThreadPool* aPool,
    QString aKeyFile, int aBits, QString aPassword) :
    BaseTask(aPool, NULL, NULL),
    iKeyFile(aKeyFile),
    iBits(aBits),
    iPassword(aPassword)
{
}

void FoilPicsModel::GenerateKeyTask::performTask()
{
    HDEBUG("Generating key..." << iBits << "bits");
    FoilKey* key = foil_key_generate_new(FOIL_KEY_RSA_PRIVATE, iBits);
    if (key) {
        GError* error = NULL;
        const QByteArray path(iKeyFile.toUtf8());
        const QByteArray passphrase(iPassword.toUtf8());
        FoilOutput* out = foil_output_file_new_open(path.constData());
        FoilPrivateKey* pk = FOIL_PRIVATE_KEY(key);
        if (foil_private_key_encrypt(pk, out, FOIL_KEY_EXPORT_FORMAT_DEFAULT,
            passphrase.constData(),
            NULL, &error)) {
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
// FoilPicsModel::EncryptTask
// ==========================================================================

class FoilPicsModel::EncryptTask : public BaseTask {
    Q_OBJECT

public:
    EncryptTask(QThreadPool* aPool, QString aSourceFile, QString aDestDir,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey, QSize aThumbSize,
        QVariantMap aMetaData);

    virtual void performTask();
    static bool addDoubleHeader(QVariant aValue, FoilMsgHeader* aHeader,
        const char* aName, char* aBuffer);

public:
    QString iSourceFile;
    QString iDestDir;
    QSize iThumbSize;
    QVariantMap iMetaData;
    ModelData* iData;
};

FoilPicsModel::EncryptTask::EncryptTask(QThreadPool* aPool, QString aSourceFile,
    QString aDestDir, FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey,
    QSize aThumbSize, QVariantMap aMetaData) :
    BaseTask(aPool, aPrivateKey, aPublicKey), iSourceFile(aSourceFile),
    iDestDir(aDestDir),
    iThumbSize(aThumbSize),
    iMetaData(aMetaData),
    iData(NULL)
{
    HDEBUG("Encrypting" << qPrintable(aSourceFile) << aMetaData);
}

bool FoilPicsModel::EncryptTask::addDoubleHeader(QVariant aValue,
    FoilMsgHeader* aHeader, const char* aName, char* aBuffer)
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

void FoilPicsModel::EncryptTask::performTask()
{
    const QByteArray path(iSourceFile.toUtf8());
    const char* fname = path.constData();
    HDEBUG(fname);
    GError* error = NULL;
    GMappedFile* map = g_mapped_file_new(fname, FALSE, &error);
    if (map) {
        GString* dest = g_string_sized_new(iDestDir.size() + 9);
        FoilOutput* out = createFoilFile(iDestDir, dest);
        if (out) {
            QMimeDatabase db;
            QMimeType type = db.mimeTypeForFile(iSourceFile);
            QByteArray mimeTypeBytes;
            const char* content_type = NULL;
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
                char* mtime = NULL;
                char* atime = NULL;
                char* ttime = NULL;
                QDateTime sortTime, dateTaken;
                QString title(ModelData::defaultTitle(iSourceFile));
                const QByteArray titleBytes(title.toUtf8());
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
                }

                // Metadata
                char degrees[16];
                int orientation = 0;
                QVariant var(iMetaData.value(MetaOrientation));
                if (var.isValid()) {
                    orientation = var.toInt();
                    snprintf(degrees, sizeof(degrees), "%d", orientation);
                    header[headers.count].name = HEADER_ORIENTATION;
                    header[headers.count].value = degrees;
                    headers.count++;
                }

                var = iMetaData.value(MetaImageDate);
                if (var.isValid()) {
                    QDateTime dateTime(var.toDateTime());
                    if (dateTime.isValid()) {
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

                HDEBUG("Writing" << dest->str);
                if (foilmsg_encrypt(out, &bytes, content_type, &headers,
                    iPrivateKey, iPublicKey, &opt, NULL)) {
                    if (atime && mtime) {
                        foil_output_close(out);
                        foil_output_unref(out);
                        out = NULL;

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
                    QImage thumb = ModelData::thumbnail(image, iThumbSize,
                        orientation);
                    QString thumbName = writeThumb(image, &headers,
                        content_type, thumb, iDestDir);
                    iData = new ModelData(iSourceFile, bytes.len, image.size(),
                        digest, dest->str, thumbName, thumb, title,
                        content_type, sortTime, orientation, cameraMaker,
                        cameraModel, latitude, longitude, altitude,
                        dateTaken);
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
// FoilPicsModel::SaveInfoTask
// ==========================================================================

class FoilPicsModel::SaveInfoTask : public BaseTask {
    Q_OBJECT

public:
    SaveInfoTask(QThreadPool* aPool, ModelInfo aInfo, QString aDir,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey);

    virtual void performTask();

public:
    ModelInfo iInfo;
    QString iFoilDir;
};

FoilPicsModel::SaveInfoTask::SaveInfoTask(QThreadPool* aPool, ModelInfo aInfo,
    QString aFoilDir, FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iInfo(aInfo),
    iFoilDir(aFoilDir)
{
}

void FoilPicsModel::SaveInfoTask::performTask()
{
    if (!isCanceled()) {
        iInfo.save(iFoilDir, iPrivateKey, iPublicKey);
    }
}

// ==========================================================================
// FoilPicsModel::CheckPicsTask
// ==========================================================================
class FoilPicsModel::CheckPicsTask : public FoilPicsTask {
    Q_OBJECT
public:
    CheckPicsTask(QThreadPool* aPool, QString aDir);

    virtual void performTask();

public:
    QString iDir;
    bool iMayHaveEncryptedPictures;
};

FoilPicsModel::CheckPicsTask::CheckPicsTask(QThreadPool* aPool, QString aDir) :
    FoilPicsTask(aPool), iDir(aDir), iMayHaveEncryptedPictures(false)
{
}

void FoilPicsModel::CheckPicsTask::performTask()
{
    const QString path(iDir);
    HDEBUG("Checking" << iDir);

    QDir dir(path);
    QFileInfoList list = dir.entryInfoList(QDir::Files |
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);

    const QString infoFile(INFO_FILE);
    for (int i=0; i<list.count() && !iMayHaveEncryptedPictures; i++) {
        const QFileInfo& info = list.at(i);
        if (info.isFile() && info.fileName() != infoFile) {
            const QByteArray fileNameBytes(info.filePath().toUtf8());
            const char* fname = fileNameBytes.constData();
            GMappedFile* map = g_mapped_file_new(fname, FALSE, NULL);
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

class FoilPicsModel::DecryptPicsTask : public BaseTask {
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
    class Progress {
    public:
        typedef QSharedPointer<Progress> Ptr;

        Progress(ModelData* aModelData, DecryptPicsTask* aTask) :
            iModelData(aModelData), iTask(aTask) {}
        ~Progress() { delete iModelData; }

    public:
        ModelData* iModelData;
        DecryptPicsTask* iTask;
    };

    DecryptPicsTask(QThreadPool* aPool, QString aDir,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey,
        QSize aThumbSize);

    virtual void performTask();

    ModelData* decryptThumb(QString aImagePath, QString aThumbPath);
    ModelData* decryptImage(QString aImagePath);
    bool decryptFile(QString aPath, QString aThumbPath);

Q_SIGNALS:
    void progress(DecryptPicsTask::Progress::Ptr aProgress);

public:
    QString iDir;
    QSize iThumbSize;
    bool iSaveInfo;
};

Q_DECLARE_METATYPE(FoilPicsModel::DecryptPicsTask::Progress::Ptr)

FoilPicsModel::DecryptPicsTask::DecryptPicsTask(QThreadPool* aPool,
    QString aDir, FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey,
    QSize aThumbSize) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iDir(aDir),
    iThumbSize(aThumbSize),
    iSaveInfo(false)
{
}

FoilPicsModel::ModelData*
FoilPicsModel::DecryptPicsTask::decryptImage(QString aImagePath)
{
    FoilPicsModel::ModelData* data = NULL;
    FoilMsg* msg = decryptAndVerify(aImagePath);
    if (msg) {
        QString origPath = ModelData::headerString(msg, HEADER_ORIGINAL_PATH);
        if (!origPath.isEmpty()) {
            QImage image = toImage(msg);
            if (!image.isNull()) {
                HDEBUG("Loaded image from" << qPrintable(aImagePath));
                int deg = ModelData::headerInt(msg, HEADER_ORIENTATION);
                QImage thumb = ModelData::thumbnail(image, iThumbSize, deg);
                QString thumbName = writeThumb(image, &msg->headers,
                    msg->content_type, thumb, iDir);
                data = ModelData::fromFoilMsg(msg, origPath, image.size(),
                    aImagePath, thumbName, thumb, msg->content_type, deg);
            }
        }
        foilmsg_free(msg);
    }
    return data;
}

FoilPicsModel::ModelData*
FoilPicsModel::DecryptPicsTask::decryptThumb(QString aImagePath,
    QString aThumbPath)
{
    FoilPicsModel::ModelData* data = NULL;
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

bool FoilPicsModel::DecryptPicsTask::decryptFile(QString aImagePath,
    QString aThumbPath)
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

void FoilPicsModel::DecryptPicsTask::performTask()
{
    if (!isCanceled()) {
        const QString path(iDir);
        HDEBUG("Checking" << iDir);

        QDir dir(path);
        QFileInfoList list = dir.entryInfoList(QDir::Files |
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);

        // Restore the order
        ModelInfo info = ModelInfo::load(iDir, iPrivateKey, iPublicKey);

        const QString infoFile(INFO_FILE);
        QHash<QString,QString> fileMap;
        int i;
        for (i=0; i<list.count(); i++) {
            const QFileInfo& info = list.at(i);
            if (info.isFile()) {
                const QString name(info.fileName());
                if (name != infoFile) {
                    fileMap.insert(name, info.filePath());
                }
            }
        }

        // First decrypt files in known order
        for (i=0; i<info.iOrder.count() && !isCanceled(); i++) {
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
                iSaveInfo = true;
            }
        }

        // Followed by the remaining files in no particular order
        if (!fileMap.isEmpty()) {
            QStringList remainingFiles = fileMap.values();
            for (i=0; i<remainingFiles.count() && !isCanceled(); i++) {
                if (decryptFile(remainingFiles.at(i), QString())) {
                    HDEBUG(remainingFiles.at(i) << "was not expected");
                    iSaveInfo = true;
                }
            }
        }
    }
}

// ==========================================================================
// FoilPicsModel::DecryptTask
// ==========================================================================

class FoilPicsModel::DecryptTask : public BaseTask {
    Q_OBJECT

public:
    DecryptTask(QThreadPool* aPool, ModelData* aData,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey);

    virtual void performTask();
    static bool saveDecrypted(FoilMsg* msg);
    static void setTimeVal(struct timeval* aTimeVal, const char* aIso8601,
        const struct timespec* aDefaultTime);
    static void setFileTimes(const char* aPath, const char* aAccessTime,
        const char* aModificationTime);

public:
    ModelData* iData;
    QString iPath;
    QString iThumbFile;
    bool iOk;
};

FoilPicsModel::DecryptTask::DecryptTask(QThreadPool* aPool, ModelData* aData,
    FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iData(aData),
    iPath(aData->iPath),
    iThumbFile(aData->iThumbFile),
    iOk(false)
{
}

void FoilPicsModel::DecryptTask::setTimeVal(struct timeval* aTimeVal,
    const char* aIso8601, const struct timespec* aDefaultTime)
{
    GTimeVal tv;
    if (aIso8601 && g_time_val_from_iso8601(aIso8601, &tv)) {
        aTimeVal->tv_sec = tv.tv_sec;
        aTimeVal->tv_usec = tv.tv_usec;
    } else {
        aTimeVal->tv_sec = aDefaultTime->tv_sec;
        aTimeVal->tv_usec = aDefaultTime->tv_nsec / 1000;
    }
}

void FoilPicsModel::DecryptTask::setFileTimes(const char* aPath,
    const char* aAccessTime, const char* aModificationTime)
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

bool FoilPicsModel::DecryptTask::saveDecrypted(FoilMsg* msg)
{
    bool ok = false;
    const char* dest = foilmsg_get_value(msg, HEADER_ORIGINAL_PATH);
    if (dest) {
        FoilOutput* out = foil_output_file_new_open(dest);
        if (out) {
            if (foil_output_write_bytes_all(out, msg->data) &&
                foil_output_flush(out)) {
                foil_output_close(out);
                HDEBUG("Wrote" << dest);
                 setFileTimes(dest,
                    foilmsg_get_value(msg, HEADER_ACCESS_TIME),
                    foilmsg_get_value(msg, HEADER_MODIFICATION_TIME));
                ok = true;
            } else {
                HWARN("Failed to write" << dest);
            }
            foil_output_unref(out);
        } else {
            HWARN("Failed to open" << dest);
        }
    } else {
        HWARN("Original file name is unknown");
    }
    return ok;
}

void FoilPicsModel::DecryptTask::performTask()
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
// FoilPicsModel::ImageRequestTask
// ==========================================================================

class FoilPicsModel::ImageRequestTask : public BaseTask {
    Q_OBJECT

public:
    ImageRequestTask(QThreadPool* aPool, QString aPath,
        QByteArray aBytes, QString aContentType,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey,
        FoilPicsImageRequest aRequest);
    virtual ~ImageRequestTask();

    virtual void performTask();

public:
    QString iPath;
    QByteArray iBytes;
    QString iContentType;
    FoilPicsImageRequest iRequest;
};

FoilPicsModel::ImageRequestTask::ImageRequestTask(QThreadPool* aPool,
    QString aPath, QByteArray aBytes, QString aContentType,
    FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey,
    FoilPicsImageRequest aRequest) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iPath(aPath),
    iBytes(aBytes),
    iContentType(aContentType),
    iRequest(aRequest)
{
}

FoilPicsModel::ImageRequestTask::~ImageRequestTask()
{
    // Make sure we have replied to the request
    iRequest.reply();
}

void FoilPicsModel::ImageRequestTask::performTask()
{
    FoilMsg* msg = NULL;
    QByteArray contentTypeBytes = iContentType.toLatin1();
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
        QImage image = QImage::fromData(iBytes, ModelData::format(type));
        HDEBUG(qPrintable(iPath) << image.size());
        iRequest.reply(image);
    } else {
        // This sends empty reply
        iRequest.reply();
    }
    foilmsg_free(msg);
}

// ==========================================================================
// FoilPicsModel::SetTitleTask
// ==========================================================================

class FoilPicsModel::SetTitleTask : public BaseTask {
    Q_OBJECT

public:
    SetTitleTask(QThreadPool* aPool, ModelData* aData,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey);

    virtual void performTask();
    QString setTitleAndEncrypt(FoilMsg* aMsg);

public:
    ModelData* iData;
    const QString iPath;
    const QString iThumbFile;
    const QString iTitle;
    QString iNewPath;
    QString iNewThumbFile;
    bool iOk;
};

FoilPicsModel::SetTitleTask::SetTitleTask(QThreadPool* aPool, ModelData* aData,
    FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iData(aData),
    iPath(aData->iPath),
    iThumbFile(aData->iThumbFile),
    iTitle(aData->iTitle),
    iOk(false)
{
}

QString FoilPicsModel::SetTitleTask::setTitleAndEncrypt(FoilMsg* aMsg)
{
    QString destPath;
    // Allocate one more in case if title was missing
    FoilMsgHeader* header = new FoilMsgHeader[aMsg->headers.count + 1];
    FoilMsgHeaders headers;
    uint i;

    // Copy the headers (except Title)
    headers.header = header;
    headers.count = 0;
    for (i = 0; i < aMsg->headers.count; i++) {
        if (strcmp(aMsg->headers.header[i].name, HEADER_TITLE)) {
            header[headers.count++] = aMsg->headers.header[i];
        }
    }

    // Now add the title
    QByteArray titleBytes(iTitle.toUtf8());
    const char* title = titleBytes.constData();
    header[headers.count].name = HEADER_TITLE;
    header[headers.count].value = title;
    headers.count++;

    FoilMsgEncryptOptions opt;
    memset(&opt, 0, sizeof(opt));
    opt.key_type = ENCRYPT_KEY_TYPE;

    QString destDir(QFileInfo(iPath).dir().path());
    GString* dest = g_string_sized_new(destDir.size() + 9);
    FoilOutput* out = createFoilFile(destDir, dest);
    if (out) {
        FoilBytes bytes;
        HDEBUG("Writing" << dest->str);
        if (foilmsg_encrypt(out, foil_bytes_from_data(&bytes, aMsg->data),
            aMsg->content_type, &headers, iPrivateKey, iPublicKey, &opt,
            NULL)) {
            destPath = QString(dest->str);
        }
        foil_output_unref(out);
    }
    g_string_free(dest, TRUE);
    delete [] header;
    return destPath;
}

void FoilPicsModel::SetTitleTask::performTask()
{
    FoilMsg* fileMsg = decryptAndVerify(iPath);
    if (fileMsg) {
        if (!isCanceled()) {
            QString thumbPath(QFileInfo(iPath).dir().filePath(iThumbFile));
            FoilMsg* thumbMsg = decryptAndVerify(thumbPath);
            if (thumbMsg) {
                if (!isCanceled()) {
                    QString newFile = setTitleAndEncrypt(fileMsg);
                    if (!newFile.isEmpty()) {
                        if (!isCanceled()) {
                            QString newThumb = setTitleAndEncrypt(thumbMsg);
                            if (!newThumb.isEmpty()) {
                                // All good
                                iOk = true;
                                iNewPath = newFile;
                                iNewThumbFile = QFileInfo(newThumb).fileName();
                                HDEBUG(iTitle << iNewPath << iNewThumbFile);
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

class FoilPicsModel::Private : public QObject {
    Q_OBJECT

public:
    // The order of constants must match the array in emitQueuedSignals()
    typedef void (FoilPicsModel::*SignalEmitter)();
    typedef uint SignalMask;
    enum Signal {
        NoSignal = -1,
        SignalCountChanged,
        SignalBusyChanged,
        SignalKeyAvailableChanged,
        SignalFoilStateChanged,
        SignalThumbnailSizeChanged,
        SignalMayHaveEncryptedPicturesChanged,
        SignalCount
    };

#define ROLE(X,x) static const QString Role##X;
    FOILPICS_ROLES(ROLE)
#undef ROLE

    Private(FoilPicsModel* aParent);
    ~Private();

    FoilPicsModel* parentModel();
    ModelData* dataAt(int aIndex);

public Q_SLOTS:
    void onCheckPicsTaskDone();
    void onDecryptPicsProgress(DecryptPicsTask::Progress::Ptr aProgress);
    void onDecryptPicsTaskDone();
    void onGenerateKeyTaskDone();
    void onEncryptTaskDone();
    void onDecryptTaskDone();
    void onDecryptAllProgress();
    void onSetTitleTaskDone();
    void onSaveInfoDone();
    void onImageRequestDone();

public:
    static size_t maxBytesToDecrypt();
    void queueSignal(Signal sig);
    void emitQueuedSignals();
    bool checkPassword(QString aPassword);
    bool changePassword(QString aOldPassword, QString aNewPassword);
    void setKeys(FoilPrivateKey* aPrivate, FoilKey* aPublic = NULL);
    void setFoilState(FoilState aState);
    void insertModelData(ModelData* aModelData);
    void destroyItemAt(int aIndex);
    void removeAt(int aIndex);
    void clearModel();
    void saveInfo();
    void generate(int aBits, QString aPassword);
    void lock(bool aTimeout);
    bool unlock(QString aPassword);
    bool encrypt(QUrl aUrl, QVariantMap aMetaData);
    void decryptAt(int aIndex);
    void decryptAll();
    void decryptTaskDone(DecryptTask* aTask, bool aLast);
    void setTitleAt(int aIndex, QString aTitle);
    void imageRequest(QString aPath, FoilPicsImageRequest aRequest);
    int findPath(QString aPath);
    bool dropDecryptedData(int aDontTouch);
    bool tooMuchDataDecrypted();
    bool busy() const;

public:
    const size_t iMaxBytesToDecrypt;
    bool iMayHaveEncryptedPictures;
    SignalMask iQueuedSignals;
    int iFirstQueuedSignal;
    FoilPicsImageProvider* iImageProvider;
    FoilPicsThumbnailProvider* iThumbnailProvider;
    QSize iThumbSize;
    QList<ModelData*> iData;
    FoilState iFoilState;
    QString iFoilPicsDir;
    QString iFoilKeyDir;
    QString iFoilKeyFile;
    FoilPrivateKey* iPrivateKey;
    FoilKey* iPublicKey;
    QThreadPool* iThreadPool;
    CheckPicsTask* iCheckPicsTask;
    SaveInfoTask* iSaveInfoTask;
    GenerateKeyTask* iGenerateKeyTask;
    DecryptPicsTask* iDecryptPicsTask;
    QList<EncryptTask*> iEncryptTasks;
    QList<ImageRequestTask*> iImageRequestTasks;
};

#define ROLE(X,x) const QString FoilPicsModel::Private::Role##X(#x);
FOILPICS_ROLES(ROLE)
#undef ROLE

FoilPicsModel::Private::Private(FoilPicsModel* aParent) :
    QObject(aParent),
    iMaxBytesToDecrypt(maxBytesToDecrypt()),
    iMayHaveEncryptedPictures(false),
    iQueuedSignals(0),
    iFirstQueuedSignal(NoSignal),
    iImageProvider(NULL),
    iThumbnailProvider(NULL),
    iThumbSize(32,32),
    iFoilState(FoilKeyMissing),
    iFoilPicsDir(QDir::homePath() + "/Documents/FoilPics"),
    iFoilKeyDir(QDir::homePath() + "/.local/share/foil"),
    iFoilKeyFile(iFoilKeyDir + "/foil.key"),
    iPrivateKey(NULL),
    iPublicKey(NULL),
    iThreadPool(new QThreadPool(this)),
    iCheckPicsTask(NULL),
    iSaveInfoTask(NULL),
    iGenerateKeyTask(NULL),
    iDecryptPicsTask(NULL)
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
    GError* error = NULL;
    const QByteArray path(iFoilKeyFile.toUtf8());
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), NULL, &error);
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
    for (i=0; i<iEncryptTasks.count(); i++) {
        iEncryptTasks.at(i)->release(this);
    }
    iEncryptTasks.clear();
    for (i=0; i<iImageRequestTasks.count(); i++) {
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

size_t FoilPicsModel::Private::maxBytesToDecrypt()
{
    // Basically, we are willing to use up to 5MB per gigabyte of RAM
    size_t kbTotal = sysconf(_SC_PHYS_PAGES)*sysconf(_SC_PAGESIZE)/0x400;
    HDEBUG("We seem to have" << kbTotal << "kB of RAM");
    return 5*kbTotal;
}

inline FoilPicsModel* FoilPicsModel::Private::parentModel()
{
    return qobject_cast<FoilPicsModel*>(parent());
}

FoilPicsModel::ModelData* FoilPicsModel::Private::dataAt(int aIndex)
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        return iData.at(aIndex);
    } else {
        return NULL;
    }
}

void FoilPicsModel::Private::queueSignal(Signal aSignal)
{
    if (aSignal > NoSignal && aSignal < SignalCount) {
        const SignalMask signalBit = (SignalMask(1) << aSignal);
        if (iQueuedSignals) {
            iQueuedSignals |= signalBit;
            if (iFirstQueuedSignal > aSignal) {
                iFirstQueuedSignal = aSignal;
            }
        } else {
            iQueuedSignals = signalBit;
            iFirstQueuedSignal = aSignal;
        }
    }
}

void FoilPicsModel::Private::emitQueuedSignals()
{
    // The order must match the Signal enum:
    static const SignalEmitter emitSignal [] = {
        &FoilPicsModel::countChanged,           // SignalCountChanged
        &FoilPicsModel::busyChanged,            // SignalBusyChanged
        &FoilPicsModel::keyAvailableChanged,    // SignalKeyAvailableChanged
        &FoilPicsModel::foilStateChanged,       // SignalFoilStateChanged
        &FoilPicsModel::thumbnailSizeChanged,   // SignalThumbnailSizeChanged
        &FoilPicsModel::mayHaveEncryptedPicturesChanged  // SignalMayHaveEncryptedPicturesChanged
    };

    Q_STATIC_ASSERT(G_N_ELEMENTS(emitSignal) == SignalCount);
    if (iQueuedSignals) {
        FoilPicsModel* model = parentModel();
        for (int i = iFirstQueuedSignal; i < SignalCount && iQueuedSignals; i++) {
            const SignalMask signalBit = (SignalMask(1) << i);
            if (iQueuedSignals & signalBit) {
                iQueuedSignals &= ~signalBit;
                Q_EMIT (model->*(emitSignal[i]))();
            }
        }
    }
}

void FoilPicsModel::Private::setKeys(FoilPrivateKey* aPrivate, FoilKey* aPublic)
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
        iPrivateKey = NULL;
        iPublicKey = NULL;
    }
}

bool FoilPicsModel::Private::checkPassword(QString aPassword)
{
    GError* error = NULL;
    HDEBUG(iFoilKeyFile);
    const QByteArray path(iFoilKeyFile.toUtf8());

    // First make sure that it's encrypted
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), NULL, &error);
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

bool FoilPicsModel::Private::changePassword(QString aOldPassword,
    QString aNewPassword)
{
    HDEBUG(iFoilKeyFile);
    if (checkPassword(aOldPassword)) {
        GError* error = NULL;
        QByteArray password(aNewPassword.toUtf8());

        // First write the temporary file
        QString tmpKeyFile = iFoilKeyFile + ".new";
        const QByteArray tmp(tmpKeyFile.toUtf8());
        FoilOutput* out = foil_output_file_new_open(tmp.constData());
        if (foil_private_key_encrypt(iPrivateKey, out,
            FOIL_KEY_EXPORT_FORMAT_DEFAULT, password.constData(),
            NULL, &error) && foil_output_flush(out)) {
            foil_output_unref(out);

            // Then rename it
            QString saveKeyFile = iFoilKeyFile + ".save";
            QFile::remove(saveKeyFile);
            if (QFile::rename(iFoilKeyFile, saveKeyFile) &&
                QFile::rename(tmpKeyFile, iFoilKeyFile)) {
                BaseTask::removeFile(saveKeyFile);
                HDEBUG("Password changed");
                Q_EMIT parentModel()->passwordChanged();
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

void FoilPicsModel::Private::setFoilState(FoilState aState)
{
    if (iFoilState != aState) {
        iFoilState = aState;
        queueSignal(SignalFoilStateChanged);
    }
}

void FoilPicsModel::Private::insertModelData(ModelData* aData)
{
    FoilPicsModel* model = parentModel();

    // Create image providers on demand because QQmlEngine::contextForObject
    // doesn't work at initialization time
    if (!iThumbnailProvider) {
        iThumbnailProvider = FoilPicsThumbnailProvider::createForObject(model);
    }
    if (iThumbnailProvider) {
        aData->iThumbSource = iThumbnailProvider->addThumbnail(aData->iImageId,
            aData->iThumbnail);
    }
    if (!iImageProvider) {
        iImageProvider = FoilPicsImageProvider::createForObject(model);
    }
    if (iImageProvider) {
        aData->iImageSource = iImageProvider->addImage(aData->iImageId,
            aData->iPath);
    }

    // Insert the data into the model
    QList<ModelData*>::const_iterator it = qLowerBound(iData.begin(),
        iData.end(), aData, ModelData::lessThan);
    int pos = it - iData.begin();
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

void FoilPicsModel::Private::destroyItemAt(int aIndex)
{
    if (aIndex >= 0 && aIndex <= iData.count()) {
        FoilPicsModel* model = parentModel();
        ModelData* data = iData.at(aIndex);
        HDEBUG("Removing" << qPrintable(data->iPath));
        // Providers must have been created by insertModelData
        iThumbnailProvider->releaseThumbnail(data->iImageId);
        iImageProvider->releaseImage(data->iImageId);
        model->beginRemoveRows(QModelIndex(), aIndex, aIndex);
        iData.removeAt(aIndex);
        delete data;
        // We no longer have any decryptable pictures:
        if (iMayHaveEncryptedPictures) {
            iMayHaveEncryptedPictures = false;
            queueSignal(SignalMayHaveEncryptedPicturesChanged);
        }
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
    }
}

void FoilPicsModel::Private::removeAt(int aIndex)
{
    ModelData* data = dataAt(aIndex);
    if (data) {
        QString path(data->iPath);
        QString thumbPath;
        if (!data->iThumbFile.isEmpty()) {
            thumbPath = (QFileInfo(path).dir().filePath(data->iThumbFile));
        }
        destroyItemAt(aIndex);
        BaseTask::removeFile(path);
        BaseTask::removeFile(thumbPath);

        // saveInfo() doesn't queue BusyChanged signal, we have to do it here
        const bool wasBusy = busy();
        saveInfo();
        if (busy() != wasBusy) {
            queueSignal(SignalBusyChanged);
        }
    }
}

void FoilPicsModel::Private::clearModel()
{
    const int n = iData.count();
    if (n > 0) {
        FoilPicsModel* model = parentModel();
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

void FoilPicsModel::Private::onCheckPicsTaskDone()
{
    HDEBUG("Done");
    if (sender() == iCheckPicsTask) {
        const bool mayHave = iCheckPicsTask->iMayHaveEncryptedPictures;
        if (iMayHaveEncryptedPictures != mayHave) {
            iMayHaveEncryptedPictures = mayHave;
            queueSignal(SignalMayHaveEncryptedPicturesChanged);
        }
        iCheckPicsTask->release(this);
        iCheckPicsTask = NULL;
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void FoilPicsModel::Private::saveInfo()
{
    // N.B. This method may change the busy state but doesn't queue
    // BusyChanged signal, it's done by the caller.
    if (iSaveInfoTask) iSaveInfoTask->release(this);
    iSaveInfoTask = new SaveInfoTask(iThreadPool, ModelInfo(iData),
        iFoilPicsDir, iPrivateKey, iPublicKey);
    iSaveInfoTask->submit(this, SLOT(onSaveInfoDone()));
}

void FoilPicsModel::Private::onSaveInfoDone()
{
    HDEBUG("Done");
    if (sender() == iSaveInfoTask) {
        iSaveInfoTask->release(this);
        iSaveInfoTask = NULL;
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void FoilPicsModel::Private::generate(int aBits, QString aPassword)
{
    const bool wasBusy = busy();
    if (iGenerateKeyTask) iGenerateKeyTask->release(this);
    iGenerateKeyTask = new GenerateKeyTask(iThreadPool, iFoilKeyFile,
        aBits, aPassword);
    iGenerateKeyTask->submit(this, SLOT(onGenerateKeyTaskDone()));
    setFoilState(FoilGeneratingKey);
    if (!wasBusy) {
        // We know we are busy now
        queueSignal(SignalBusyChanged);
    }
    emitQueuedSignals();
}

void FoilPicsModel::Private::onGenerateKeyTaskDone()
{
    HDEBUG("Got a new key");
    HASSERT(sender() == iGenerateKeyTask);
    if (iGenerateKeyTask->iPrivateKey) {
        setKeys(iGenerateKeyTask->iPrivateKey, iGenerateKeyTask->iPublicKey);
        setFoilState(FoilPicsReady);
    } else {
        setKeys(NULL);
        setFoilState(FoilKeyError);
    }
    iGenerateKeyTask->release(this);
    iGenerateKeyTask = NULL;
    if (!busy()) {
        // We know we were busy when we received this signal
        queueSignal(SignalBusyChanged);
    }
    parentModel()->keyGenerated();
    emitQueuedSignals();
}

void FoilPicsModel::Private::lock(bool aTimeout)
{
    // Cancel whatever we are doing
    const bool wasBusy = busy();
    if (iSaveInfoTask) {
        iSaveInfoTask->release(this);
        iSaveInfoTask = NULL;
    }
    if (iDecryptPicsTask) {
        iDecryptPicsTask->release(this);
        iDecryptPicsTask = NULL;
    }
    int i;
    for (i=0; i<iEncryptTasks.count(); i++) {
        iEncryptTasks.at(i)->release(this);
    }
    for (i=0; i<iImageRequestTasks.count(); i++) {
        iImageRequestTasks.at(i)->release(this);
    }
    iEncryptTasks.clear();
    iImageRequestTasks.clear();
    // Destroy decrypted pictures
    if (!iData.isEmpty()) {
        FoilPicsModel* model = parentModel();
        model->beginRemoveRows(QModelIndex(), 0, iData.count()-1);
        qDeleteAll(iData);
        iData.clear();
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
    }
    if (busy() != wasBusy) {
        queueSignal(SignalBusyChanged);
    }
    if (iPrivateKey) {
        // Throw the keys away
        setKeys(NULL);
        setFoilState(aTimeout ? FoilLockedTimedOut : FoilLocked);
        HDEBUG("Locked");
    } else {
        HDEBUG("Nothing to lock, there's no key yet!");
    }
}

bool FoilPicsModel::Private::unlock(QString aPassword)
{
    GError* error = NULL;
    HDEBUG(iFoilKeyFile);
    const QByteArray path(iFoilKeyFile.toUtf8());
    bool ok = false;

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
                connect(iDecryptPicsTask,
                    SIGNAL(progress(DecryptPicsTask::Progress::Ptr)),
                    SLOT(onDecryptPicsProgress(DecryptPicsTask::Progress::Ptr)));
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

bool FoilPicsModel::Private::encrypt(QUrl aUrl, QVariantMap aMetaData)
{
    if (iPrivateKey && aUrl.isLocalFile()) {
        const bool wasBusy = busy();
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
        EncryptTask* task = new EncryptTask(iThreadPool, aUrl.toLocalFile(),
            iFoilPicsDir, iPrivateKey, iPublicKey, iThumbSize, aMetaData);
        iEncryptTasks.append(task);
        task->submit(this, SLOT(onEncryptTaskDone()));
        if (!wasBusy) {
            // We know we are busy now
            queueSignal(SignalBusyChanged);
        }
        return true;
    }
    return false;
}

void FoilPicsModel::Private::onEncryptTaskDone()
{
    EncryptTask* task = qobject_cast<EncryptTask*>(sender());
    HVERIFY(iEncryptTasks.removeAll(task));
    HDEBUG("Encrypted" << qPrintable(task->iSourceFile));
    if (task->iData) {
        insertModelData(task->iData);
        task->iData = NULL;
        saveInfo();
    }
    FoilPicsFileUtil::instance()->mediaDeleted(task->iSourceFile);
    task->release(this);
    if (!busy()) {
        // We know we were busy when we received this signal
        queueSignal(SignalBusyChanged);
    }
    emitQueuedSignals();
}

void FoilPicsModel::Private::decryptAt(int aIndex)
{
    ModelData* data = dataAt(aIndex);
    if (data && !data->iDecryptTask) {
        const bool wasBusy = busy();
        HDEBUG("About to decrypt" << qPrintable(data->iPath));
        data->iDecryptTask = new DecryptTask(iThreadPool, data,
            iPrivateKey, iPublicKey);
        data->iDecryptTask->submit(this, SLOT(onDecryptTaskDone()));
        if (!wasBusy) {
            // We know we are busy now
            queueSignal(SignalBusyChanged);
        }
    }
}

void FoilPicsModel::Private::decryptTaskDone(DecryptTask* aTask, bool aLast)
{
    if (aTask) {
        DecryptTask* task = qobject_cast<DecryptTask*>(sender());
        ModelData* data = task->iData;
        data->iDecryptTask = NULL;
        task->iData = NULL;
        task->release(this);
        destroyItemAt(iData.indexOf(data));
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

void FoilPicsModel::Private::onDecryptTaskDone()
{
    decryptTaskDone(qobject_cast<DecryptTask*>(sender()), true);
}

void FoilPicsModel::Private::decryptAll()
{
    if (!iData.isEmpty()) {
        const bool wasBusy = busy();
        const int n = iData.count();
        HDEBUG("Decrypting all" << n << "picture(s)");
        // Start from the last picture
        ModelData* data;
        for (int i = n-1; i > 0; i--) {
            data = iData.at(i);
            if (!data->iDecryptTask) {
                data->iDecryptTask = new DecryptTask(iThreadPool, data,
                    iPrivateKey, iPublicKey);
                data->iDecryptTask->submit(this, SLOT(onDecryptAllProgress()));
            }
        }
        // The last onDecryptTaskDone will reset the image info
        data = iData.first();
        if (!data->iDecryptTask) {
            data->iDecryptTask = new DecryptTask(iThreadPool, data,
                iPrivateKey, iPublicKey);
            data->iDecryptTask->submit(this, SLOT(onDecryptTaskDone()));
        }
        if (busy() != wasBusy) {
            queueSignal(SignalBusyChanged);
        }
    }
}

void FoilPicsModel::Private::onDecryptAllProgress()
{
    decryptTaskDone(qobject_cast<DecryptTask*>(sender()), false);
}

void FoilPicsModel::Private::onDecryptPicsProgress(DecryptPicsTask::Progress::Ptr aProgress)
{
    if (aProgress && aProgress->iTask == iDecryptPicsTask) {
        // Transfer ownership of this ModelData to the model
        insertModelData(aProgress->iModelData);
        aProgress->iModelData = NULL;
    }
    emitQueuedSignals();
}

void FoilPicsModel::Private::onDecryptPicsTaskDone()
{
    HDEBUG(iData.count() << "picture(s) decrypted");
    if (sender() == iDecryptPicsTask) {
        if (iDecryptPicsTask->iSaveInfo) saveInfo();
        iDecryptPicsTask->release(this);
        iDecryptPicsTask = NULL;
        if (iFoilState == FoilDecrypting) {
            setFoilState(FoilPicsReady);
        }
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
    }
    emitQueuedSignals();
}

void FoilPicsModel::Private::setTitleAt(int aIndex, QString aTitle)
{
    ModelData* data = dataAt(aIndex);
    if (data) {
        QString title(aTitle.isEmpty() ? data->iDefaultTitle : aTitle);
        if (data->iTitle != title) {
            data->iTitle = title;
            HDEBUG("Settings title at" << aIndex << "to" << title);
            const bool wasBusy = busy();
            if (data->iSetTitleTask) {
                HDEBUG("Dropping previous task");
                data->iSetTitleTask->release(this);
                data->iSetTitleTask = NULL;
            }
            data->iSetTitleTask = new SetTitleTask(iThreadPool, data,
                iPrivateKey, iPublicKey);
            data->iSetTitleTask->submit(this, SLOT(onSetTitleTaskDone()));
            if (!wasBusy) {
                // We know we are busy now
                queueSignal(SignalBusyChanged);
            }

            // Notify the view
            QVector<int> roles;
            roles.append(ModelData::TitleRole);
            FoilPicsModel* model = parentModel();
            QModelIndex index(model->createIndex(aIndex, 0));
            Q_EMIT model->dataChanged(index, index, roles);
        }
    }
}

void FoilPicsModel::Private::onSetTitleTaskDone()
{
    // task->iData must be valid because if ModelData were deleted
    // it would cancel the task in the destructor and we wouldn't
    // receive this signal
    SetTitleTask* task = qobject_cast<SetTitleTask*>(sender());
    HDEBUG(task->iTitle);
    ModelData* data = task->iData;
    data->iSetTitleTask = NULL;
    task->iData = NULL;

    if (task->iOk) {
        data->iThumbFile = task->iNewThumbFile;
        data->iPath = task->iNewPath;

        // Image path changed but source URL didn't because it's derived
        // from the hash of the original file. Just update the path.
        iImageProvider->addImage(data->iImageId, data->iPath);

        // Title change already (optimistically) signalled by setTitleAt
        QVector<int> roles;
        roles.append(ModelData::EncryptedFileSizeRole);
        roles.append(ModelData::FileNameRole);
        FoilPicsModel* model = parentModel();
        QModelIndex index(model->createIndex(iData.indexOf(data), 0));
        Q_EMIT model->dataChanged(index, index, roles);
    }

    // There's no need to queue BusyChanged because we were busy when we
    // received this signal and we are still going to be busy after we
    // call saveInfo()
    task->release(this);
    emitQueuedSignals();
    saveInfo();
}

//
// Three threads are involved in fetching the decrypted image:
//
// 1. QQuickPixmapReader calls FoilPicsImageProvider::requestImage on its
//    own thread, which queues "imageRequest" signal to FoilPicsModel and
//    blocks until FoilPicsImageRequest is replied to.
// 2. FoilPicsModel receives the signal on the UI thread and queues
//    ImageRequestTask. It's done even if the decrypted data is cached
//    because creating the image from data may take too long for the UI
//    thread.
// 3. ImageRequestTask gets executed on yet another worker thread and
//    when it's done, it replies to FoilPicsImageRequest which unblocks
//    QQuickPixmapReader thread and queues the "done" signal to FoilPicsModel.
//
// The "done" signal is finally handled by onImageRequestDone() on the UI
// thread. If caches the freshly decrypted data.
//
void FoilPicsModel::Private::imageRequest(QString aPath,
    FoilPicsImageRequest aRequest)
{
    const bool wasBusy = busy();
    QByteArray bytes;
    QString contentType;

    // Check if the decrypted data is cached
    const int index = findPath(aPath);
    if (index >= 0) {
        ModelData* data = iData.at(index);
        if (!data->iBytes.isEmpty()) {
            bytes = data->iBytes;
            contentType = data->iContentType;
        }
    }
    HDEBUG("Requesting" << qPrintable(aPath));
    ImageRequestTask* task = new ImageRequestTask(iThreadPool, aPath,
        bytes, contentType, iPrivateKey, iPublicKey, aRequest);
    iImageRequestTasks.append(task);
    task->submit(this, SLOT(onImageRequestDone()));
    if (!wasBusy) {
        // We know we are busy now
        queueSignal(SignalBusyChanged);
    }
}

void FoilPicsModel::Private::onImageRequestDone()
{
    ImageRequestTask* task = qobject_cast<ImageRequestTask*>(sender());
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

int FoilPicsModel::Private::findPath(QString aPath)
{
    const int n = iData.count();
    for (int i=0; i<n; i++) {
        if (iData.at(i)->iPath == aPath) {
            return i;
        }
    }
    return -1;
}

bool FoilPicsModel::Private::dropDecryptedData(int aDontTouch)
{
    int indexToDrop = -1;
    int maxDistance = -1;
    const int n = iData.count();
    // Find the index furthest away from the one we don't touch
    for (int i=0; i<n; i++) {
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

bool FoilPicsModel::Private::tooMuchDataDecrypted()
{
    int count = 0;
    size_t totalSize = 0;
    const int n = iData.count();
    for (int i=0; i<n; i++) {
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

bool FoilPicsModel::Private::busy() const
{
    if (iCheckPicsTask || iSaveInfoTask || iGenerateKeyTask ||
        iDecryptPicsTask || !iEncryptTasks.isEmpty() ||
        !iImageRequestTasks.isEmpty()) {
        return true;
    } else {
        // Hmm... This is not very scalable
        const int n = iData.count();
        for (int i=0; i<n; i++) {
            ModelData* data = iData.at(i);
            if (data->iDecryptTask || data->iSetTitleTask) {
                return true;
            }
        }
        return false;
    }
}

// ==========================================================================
// FoilPicsModel
// ==========================================================================

FoilPicsModel::FoilPicsModel(QObject* aParent) :
    QAbstractListModel(aParent),
    iPrivate(new Private(this))
{
}

int FoilPicsModel::count() const
{
    return iPrivate->iData.count();
}

bool FoilPicsModel::busy() const
{
    return iPrivate->busy();
}

bool FoilPicsModel::keyAvailable() const
{
    return iPrivate->iPrivateKey != NULL;
}

FoilPicsModel::FoilState FoilPicsModel::foilState() const
{
    return iPrivate->iFoilState;
}

bool FoilPicsModel::mayHaveEncryptedPictures() const
{
    return iPrivate->iMayHaveEncryptedPictures;
}

QSize FoilPicsModel::thumbnailSize() const
{
    return iPrivate->iThumbSize;
}

QHash<int,QByteArray> FoilPicsModel::roleNames() const
{
    QHash<int,QByteArray> roles;
#define ROLE(X,x) roles.insert(ModelData::X##Role, #x);
FOILPICS_ROLES(ROLE)
#undef ROLE
    return roles;
}

int FoilPicsModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->iData.count();
}

QVariant FoilPicsModel::data(const QModelIndex& aIndex, int aRole) const
{
    ModelData* data = iPrivate->dataAt(aIndex.row());
    return data ? data->get((ModelData::Role)aRole) : QVariant();
}

void FoilPicsModel::setThumbnailSize(QSize aSize)
{
    if (iPrivate->iThumbSize != aSize) {
        iPrivate->iThumbSize = aSize;
        HDEBUG(aSize);
        // Re-generate the thumbnails?
        Q_EMIT thumbnailSizeChanged();
    }
}

void FoilPicsModel::removeAt(int aIndex)
{
    HDEBUG(aIndex);
    iPrivate->removeAt(aIndex);
    iPrivate->emitQueuedSignals();
}

QVariantMap FoilPicsModel::get(int aIndex) const
{
    HDEBUG(aIndex);
    QVariantMap map;
    ModelData* data = iPrivate->dataAt(aIndex);
    if (data) {
#define ROLE(X,x) map.insert(Private::Role##X, data->get(ModelData::X##Role));
FOILPICS_ROLES(ROLE)
#undef ROLE
    }
    return map;
}

void FoilPicsModel::decryptAt(int aIndex)
{
    HDEBUG(aIndex);
    iPrivate->decryptAt(aIndex);
    iPrivate->emitQueuedSignals();
}

void FoilPicsModel::decryptAll()
{
    HDEBUG("Decrypting all");
    iPrivate->decryptAll();
    iPrivate->emitQueuedSignals();
}

bool FoilPicsModel::encryptFile(QUrl aUrl, QVariantMap aMetaData)
{
    const bool ok = iPrivate->encrypt(aUrl, aMetaData);
    iPrivate->emitQueuedSignals();
    return ok;
}

void FoilPicsModel::lock(bool aTimeout)
{
    iPrivate->lock(aTimeout);
    iPrivate->emitQueuedSignals();
}

bool FoilPicsModel::unlock(QString aPassword)
{
    const bool ok = iPrivate->unlock(aPassword);
    iPrivate->emitQueuedSignals();
    return ok;
}

bool FoilPicsModel::checkPassword(QString aPassword)
{
    return iPrivate->checkPassword(aPassword);
}

bool FoilPicsModel::changePassword(QString aOld, QString aNew)
{
    bool ok = iPrivate->changePassword(aOld, aNew);
    iPrivate->emitQueuedSignals();
    return ok;
}

void FoilPicsModel::generateKey(int aBits, QString aPassword)
{
    iPrivate->generate(aBits, aPassword);
    iPrivate->emitQueuedSignals();
}

void FoilPicsModel::imageRequest(QString aPath, FoilPicsImageRequest aRequest)
{
    iPrivate->imageRequest(aPath, aRequest);
    iPrivate->emitQueuedSignals();
}

void FoilPicsModel::setTitleAt(int aIndex, QString aTitle)
{
    iPrivate->setTitleAt(aIndex, aTitle);
    iPrivate->emitQueuedSignals();
}

#include "FoilPicsModel.moc"
