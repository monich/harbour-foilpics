import QtQuick 2.0
import Sailfish.Silica 1.0
//import QtDocGallery 5.0
import harbour.foilpics.QtDocGallery 1.0

Page {
    property alias item: galleryItem.item

    allowedOrientations: Orientation.All

    DocumentGalleryItem {
        id: galleryItem
        autoUpdate: false

        // https://git.merproject.org/mer-core/qtdocgallery/blob/mer-master/src/gallery/qdocumentgallery.h
        properties: [
            "fileName", "fileSize", "mimeType", "width", "height",
            "dateTaken", "cameraManufacturer", "cameraModel",
            "latitude", "longitude", "altitude"
        ]

        onStatusChanged: {
            if (status === DocumentGalleryItem.Finished) {
                fileNameItem.value = galleryItem.metaData.fileName
                fileSizeItem.value = FileUtil.formatFileSize(galleryItem.metaData.fileSize)
                mimeTypeItem.value = galleryItem.metaData.mimeType
                widthItem.value = galleryItem.metaData.width
                heightItem.value = galleryItem.metaData.height
                dateTakenItem.value = galleryItem.metaData.dateTaken ?
                    Format.formatDate(galleryItem.metaData.dateTaken, Format.Timepoint) : ""
                cameraManufacturerItem.value = galleryItem.metaData.cameraManufacturer
                cameraModelItem.value = galleryItem.metaData.cameraModel
                gpsItem.value = galleryItem.metaData.latitude != "" &&
                    galleryItem.metaData.longitude != "" &&
                    galleryItem.metaData.altitude != "" ?
                    //: Coordinates
                    //% "Latitude %1, Longitude %2, Altitude %3"
                    qsTrId("foilpics-details-coordinates-value").arg(galleryItem.metaData.latitude).arg(galleryItem.metaData.longitude).arg(galleryItem.metaData.altitude) :
                    ""
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                //: Page header
                //% "Details"
                title: qsTrId("foilpics-details-header")
            }
            DetailItem {
                id: fileNameItem
                //: Details label
                //% "File name"
                label: qsTrId("foilpics-details-file_name-label")
            }
            DetailItem {
                id: fileSizeItem
                //: Details label
                //% "File size"
                label: qsTrId("foilpics-details-file_size-label")
            }
            DetailItem {
                id: mimeTypeItem
                //: Details label
                //% "Type"
                label: qsTrId("foilpics-details-mime_type-label")
            }
            DetailItem {
                id: widthItem
                //: Details label
                //% "Width"
                label: qsTrId("foilpics-details-width-label")
            }
            DetailItem {
                id: heightItem
                //: Details label
                //% "Height"
                label: qsTrId("foilpics-details-height-label")
            }
            DetailItem {
                id: dateTakenItem
                //: Details label
                //% "Date"
                label: qsTrId("foilpics-details-date-label")
                visible: value.length > 0
            }
            DetailItem {
                id: cameraManufacturerItem
                //: Details label
                //% "Camera manufacturer"
                label: qsTrId("foilpics-details-camera_manufacturer-label")
                visible: value.length > 0
            }
            DetailItem {
                id: cameraModelItem
                //: Details label
                //% "Camera model"
                label: qsTrId("foilpics-details-camera_model-label")
                visible: value.length > 0
            }
            DetailItem {
                id: gpsItem
                //: Details label
                //% "Coordinates"
                label: qsTrId("foilpics-details-coordinates-label")
                visible: value.length > 0
            }
        }
        VerticalScrollDecorator { }
    }
}
