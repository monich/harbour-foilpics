import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

Page {
    id: thisPage

    property var details
    property var foilModel

    allowedOrientations: Orientation.All

    property alias _groupId: groupIdWatch.value
    readonly property var _groupModel: foilModel ? foilModel.groupModel : null

    signal titleChanged(var title)
    signal requestIndex(int index)

    // These are used from the native code:
    //% "%1 B"
    readonly property string _formatB: qsTrId("foilpics-file_size-bytes")
    //% "%1 kB"
    readonly property string _formatKB: qsTrId("foilpics-file_size-kilobytes")
    //% "%1 MB"
    readonly property string _formatMB: qsTrId("foilpics-file_size-megabytes")
    //% "%1 GB"
    readonly property string _formatGB: qsTrId("foilpics-file_size-gigabytes")
    //% "%1 TB"
    readonly property string _formatTB: qsTrId("foilpics-file_size-terabytes")

    Component.onCompleted: groupIdWatch.keyValue = details.imageId

    onStatusChanged: {
        if (status === PageStatus.Deactivating) {
            thisPage.titleChanged(titleDetail.value)
        }
    }

    // ImageId is the hash of the original file, it doesn't change
    // when we move the image between the groups.
    FoilPicsModelWatch {
        id: groupIdWatch

        model: foilModel
        keyRole: "imageId"
        role: "groupId"
        onIndexChanged: thisPage.requestIndex(index)
    }

    FoilPicsModelWatch {
        id: groupNameWatch

        model: _groupModel
        keyRole: "groupId"
        keyValue: _groupId
        role: "groupName"
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
            CopyableDetailItem {
                //: Details label
                //% "Original file"
                label: qsTrId("foilpics-details-original_path-label")
                value: ("originalPath" in details) ? details.originalPath : ""
                visible: value.length > 0
            }
            CopyableDetailItem {
                //: Details label
                //% "Original file size"
                label: qsTrId("foilpics-details-original_file_size-label")
                value: ("originalFileSize" in details && details.originalFileSize) ?
                    FoilPics.formatFileSize(details.originalFileSize) : ""
                visible: value.length > 0
            }
            CopyableDetailItem {
                //: Details label
                //% "Encrypted file size"
                label: qsTrId("foilpics-details-encrypted_file_size-label")
                value: details.encryptedFileSize ? FoilPics.formatFileSize(details.encryptedFileSize) : ""
                visible: value.length > 0
            }
            CopyableDetailItem {
                //: Details label
                //% "Type"
                label: qsTrId("foilpics-details-mime_type-label")
                value: details.mimeType
            }
            CopyableDetailItem {
                //: Details label
                //% "Width"
                label: qsTrId("foilpics-details-width-label")
                value: details.imageWidth
            }
            CopyableDetailItem {
                //: Details label
                //% "Height"
                label: qsTrId("foilpics-details-height-label")
                value: details.imageHeight
            }
            CopyableDetailItem {
                //: Details label
                //% "Date"
                label: qsTrId("foilpics-details-date-label")
                value: ("imageDate" in details && details.imageDate) ?
                    Format.formatDate(details.imageDate, Format.Timepoint) : ""
                visible: value.length > 0
            }
            CopyableDetailItem {
                //: Details label
                //% "Camera manufacturer"
                label: qsTrId("foilpics-details-camera_manufacturer-label")
                value: ("cameraManufacturer" in details) ? details.cameraManufacturer : ""
                visible: value.length > 0
            }
            CopyableDetailItem {
                //: Details label
                //% "Camera model"
                label: qsTrId("foilpics-details-camera_model-label")
                value: ("cameraModel" in details) ? details.cameraModel : ""
                visible: value.length > 0
            }
            CopyableDetailItem {
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

            SectionHeader {
                //: Section header for editable details
                //% "Editable details"
                text: qsTrId("foilpics-details-section_header-editable")
            }

            EditableDetail {
                id: titleDetail
                //: Details label
                //% "Title"
                label: qsTrId("foilpics-details-image_title-label")
                value: details.title
                defaultValue: details.defaultTitle
                onApply: thisPage.titleChanged(value)
            }

            Item {
                id: groupDetail

                width: parent.width
                height: Math.max(groupDetailLabel.height, groupDetailItem.height)

                Text {
                    id: groupDetailLabel

                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        right: parent.horizontalCenter
                        rightMargin: Theme.paddingSmall
                        verticalCenter: parent.verticalCenter
                    }
                    horizontalAlignment: Text.AlignRight
                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    //: Details label
                    //% "Group"
                    text: qsTrId("foilpics-details-group-label")
                }

                ListItem {
                    id: groupDetailItem

                    y: Theme.paddingSmall
                    anchors {
                        left: parent.horizontalCenter
                        leftMargin: Theme.paddingSmall
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }

                    Label {
                        id: groupDetailName

                        text: _groupId ? groupNameWatch.value :
                            //: Name of the default group
                            //% "Default"
                            qsTrId("foilpics-default_group")
                        anchors {
                            fill: parent
                            leftMargin: Theme.paddingSmall
                        }
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Theme.fontSizeSmall
                        truncationMode: TruncationMode.Fade
                    }

                    onClicked: {
                        var groupPage = pageStack.push(Qt.resolvedUrl("EditGroupPage.qml"), {
                            allowedOrientations: thisPage.allowedOrientations,
                            groupModel: _groupModel,
                            selectedGroupId: _groupId
                        })
                        groupPage.groupSelected.connect(function(index) {
                            foilModel.setGroupIdAt(groupIdWatch.index, _groupModel.groupId(index))
                            pageStack.pop()
                        })
                    }
                }
            }
        }
        VerticalScrollDecorator { }
    }
}
