import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    property var details
    property var foilModel
    readonly property var groupModel: foilModel ? foilModel.groupModel : null

    signal titleChanged(var title)
    signal requestIndex(int index)

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

    onStatusChanged: {
        if (status === PageStatus.Deactivating) {
            page.titleChanged(titleDetail.value)
        }
    }

    function groupLabel() {
        return details.groupId ?
            groupModel.groupName(details.groupId) :
            //: Name of the default group
            //% "Default"
            qsTrId("foilpics-default_group")
    }

    // ImageId is the hash of the original file, it doesn't change
    // when we move the image between the groups.
    FoilPicsModelWatch {
        id: groupIdWatch
        model: foilModel
        keyRole: "imageId"
        keyValue: details.imageId
        role: "groupId"
        onIndexChanged: page.requestIndex(index)
        onValueChanged: {
            details.groupId = value
            groupNameWatch.keyValue = value
            groupDetailName.text = page.groupLabel()
        }
    }

    FoilPicsModelWatch {
        id: groupNameWatch
        model: groupModel
        keyRole: "groupId"
        keyValue: details.groupId
        role: "groupName"
        onValueChanged: groupDetailName.text = page.groupLabel()
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
                //% "File name"
                label: qsTrId("foilpics-details-file_name-label")
                value: ("fileName" in details) ? details.fileName : ""
                visible: value.length > 0
            }
            CopyableDetailItem {
                //: Details label
                //% "Original file size"
                label: qsTrId("foilpics-details-original_file_size-label")
                value: ("originalFileSize" in details && details.originalFileSize) ?
                    FileUtil.formatFileSize(details.originalFileSize) : ""
                visible: value.length > 0
            }
            CopyableDetailItem {
                //: Details label
                //% "Encrypted file size"
                label: qsTrId("foilpics-details-encrypted_file_size-label")
                value: details.encryptedFileSize ? FileUtil.formatFileSize(details.encryptedFileSize) : ""
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
                onApply: page.titleChanged(value)
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
                        text: page.groupLabel()
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
                            groupModel: groupModel,
                            selectedGroupId: details.groupId
                        })
                        groupPage.groupSelected.connect(function(index) {
                            var groupId = groupModel.groupId(index)
                            groupDetailName.text = groupModel.defaultGroupAt(index) ?
                                //: Name of the default group
                                //% "Default"
                                qsTrId("foilpics-default_group") :
                                groupModel.groupNameAt(index)
                            foilModel.setGroupIdAt(groupIdWatch.index, groupId)
                            pageStack.pop()
                        })
                    }
                }
            }
        }
        VerticalScrollDecorator { }
    }
}
