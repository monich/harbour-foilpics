import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0
//import QtDocGallery 5.0
import harbour.foilpics.QtDocGallery 1.0

Column {
    property alias item: galleryItem.item

    DocumentGalleryItem {
        id: galleryItem

        autoUpdate: false

        // https://github.com/sailfishos/qtdocgallery/blob/mer-master/src/gallery/qdocumentgallery.h
        properties: [
            "filePath", "fileSize", "mimeType", "width", "height",
            "dateTaken", "cameraManufacturer", "cameraModel",
            "latitude", "longitude", "altitude"
        ]

        onStatusChanged: {
            if (status === DocumentGalleryItem.Finished) {
                var metadata = galleryItem.metaData
                fileNameItem.value = metadata.filePath
                fileSizeItem.value = FileUtil.formatFileSize(metadata.fileSize)
                mimeTypeItem.value = metadata.mimeType
                widthItem.value = metadata.width
                heightItem.value = metadata.height
                dateTakenItem.value = metadata.dateTaken ?
                    Format.formatDate(metadata.dateTaken, Format.Timepoint) : ""
                cameraManufacturerItem.value = metadata.cameraManufacturer
                cameraModelItem.value = metadata.cameraModel
                coordinatesItem.value = metadata.latitude != "" && metadata.longitude != "" && !(metadata.longitude == 0 && metadata.longitude == 0)?
                    //: Coordinates
                    //% "Latitude %1, Longitude %2, Altitude %3"
                    qsTrId("foilpics-details-coordinates-value").arg(metadata.latitude).arg(metadata.longitude).arg(metadata.altitude) :
                    ""
            }
        }
    }

    CopyableDetailItem {
        id: fileNameItem
        //: Details label
        //% "File name"
        label: qsTrId("foilpics-details-file_name-label")
    }
    CopyableDetailItem {
        id: fileSizeItem
        //: Details label
        //% "File size"
        label: qsTrId("foilpics-details-file_size-label")
    }
    CopyableDetailItem {
        id: mimeTypeItem
        //: Details label
        //% "Type"
        label: qsTrId("foilpics-details-mime_type-label")
    }
    CopyableDetailItem {
        id: widthItem
        //: Details label
        //% "Width"
        label: qsTrId("foilpics-details-width-label")
    }
    CopyableDetailItem {
        id: heightItem
        //: Details label
        //% "Height"
        label: qsTrId("foilpics-details-height-label")
    }
    CopyableDetailItem {
        id: dateTakenItem
        //: Details label
        //% "Date"
        label: qsTrId("foilpics-details-date-label")
        visible: value.length > 0
    }
    CopyableDetailItem {
        id: cameraManufacturerItem
        //: Details label
        //% "Camera manufacturer"
        label: qsTrId("foilpics-details-camera_manufacturer-label")
        visible: value.length > 0
    }
    CopyableDetailItem {
        id: cameraModelItem
        //: Details label
        //% "Camera model"
        label: qsTrId("foilpics-details-camera_model-label")
        visible: value.length > 0
    }
    CopyableDetailItem {
        id: coordinatesItem
        //: Details label
        //% "Coordinates"
        label: qsTrId("foilpics-details-coordinates-label")
        visible: value.length > 0
    }
}