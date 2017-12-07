import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All
    property var details

    // These are used from the native code:
    //% "%n B"
    readonly property string _formatB: qsTrId("foilpics-file_size-bytes")
    //% "%1 kB"
    readonly property string _formatKB: qsTrId("foilpics-file_size-kilobytes")
    //% "%1 MB"
    readonly property string _formatMB: qsTrId("foilpics-file_size-megabytes")
    //% "%1 GB"
    readonly property string _formatGB: qsTrId("foilpics-file_size-gigabytes")
    //% "%1 TB"
    readonly property string _formatTB: qsTrId("foilpics-file_size-terabytes")

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
                //: Details label
                //% "File name"
                label: qsTrId("foilpics-details-file_name-label")
                value: ("fileName" in details) ? details.fileName : ""
                visible: value.length > 0
            }
            DetailItem {
                //: Details label
                //% "Original file size"
                label: qsTrId("foilpics-details-original_file_size-label")
                value: ("originalFileSize" in details && details.originalFileSize) ?
                    FileUtil.formatFileSize(details.originalFileSize) : ""
                visible: value.length > 0
            }
            DetailItem {
                //: Details label
                //% "Encrypted file size"
                label: qsTrId("foilpics-details-encrypted_file_size-label")
                value: ("encryptedFileSize" in details && details.encryptedFileSize) ?
                    FileUtil.formatFileSize(details.encryptedFileSize) : ""
                visible: value.length > 0
            }
            DetailItem {
                //: Details label
                //% "Type"
                label: qsTrId("foilpics-details-mime_type-label")
                value: ("mimeType" in details) ? details.mimeType : ""
                visible: value.length > 0
            }
            DetailItem {
                //: Details label
                //% "Width"
                label: qsTrId("foilpics-details-width-label")
                value: ("imageWidth" in details) ? details.imageWidth : ""
                visible: value.length > 0
            }
            DetailItem {
                //: Details label
                //% "Height"
                label: qsTrId("foilpics-details-height-label")
                value: ("imageHeight" in details) ? details.imageHeight : ""
                visible: value.length > 0
            }
            DetailItem {
                //: Details label
                //% "Date"
                label: qsTrId("foilpics-details-date-label")
                value: ("imageDate" in details && details.imageDate) ?
                    Format.formatDate(details.imageDate, Format.Timepoint) : ""
                visible: value.length > 0
            }
            DetailItem {
                //: Details label
                //% "Camera manufacturer"
                label: qsTrId("foilpics-details-camera_manufacturer-label")
                value: ("cameraManufacturer" in details) ? details.cameraManufacturer : ""
                visible: value.length > 0
            }
            DetailItem {
                //: Details label
                //% "Camera model"
                label: qsTrId("foilpics-details-camera_model-label")
                value: ("cameraModel" in details) ? details.cameraModel : ""
                visible: value.length > 0
            }
            DetailItem {
                //: Details label
                //% "Coordinates"
                label: qsTrId("foilpics-details-coordinates-label")
                value: ("latitude" in details && "longitude" in details && "altitude" in details &&
                    details.latitude !== undefined && details.longitude !== undefined && details.altitude !== undefined) ?
                    //: Coordinates
                    //% "Latitude %1, Longitude %2, Altitude %3"
                    qsTrId("foilpics-details-coordinates-value").arg(details.latitude).arg(details.longitude).arg(details.altitude) :
                    ""
                visible: value.length > 0
            }
        }
        VerticalScrollDecorator { }
    }
}
