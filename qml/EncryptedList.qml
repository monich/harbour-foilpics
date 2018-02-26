import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

SilicaListView {
    id: view

    property var foilModel
    property var selectionModel
    property bool selecting
    property bool busy

    property int requestedGroupIndex: -1
    property var requestedGroup

    function jumpToIndex(index, cellSize, columnCount) {
        requestedGroupIndex = foilModel.groupIndexAt(index)
        var groupIndex = model.offsetWithinGroup(requestedGroupIndex, index)
        var count = model.groupPicsCountAt(requestedGroupIndex)
        // This (hopefully) instantiates delegate and updates requestedGroup
        positionViewAtIndex(requestedGroupIndex, ListView.Visible)
        requestedGroup.currentIndex = groupIndex
        var groupTop = requestedGroup.y + requestedGroup.headerHeight
        var cellTop = groupTop + Math.floor(groupIndex/columnCount) * cellSize
        if (cellTop < contentY) {
            contentY = cellTop
        } else {
            var cellBottom = cellTop + cellSize
            var viewportBottom = contentY + view.height
            if (cellBottom > viewportBottom) {
                contentY += (cellBottom - viewportBottom)
            }
        }
    }

    function decryptItem(index, cellSize, columnCount) {
        jumpToIndex(index, cellSize, columnCount)
        pageStack.pop()
        var item = requestedGroup.currentItem
        if (item) {
            var remorse = decryptRemorseComponent.createObject(null)
            remorse.z = item.z + 1
            remorse.execute(remorseContainerComponent.createObject(item),
                //: Decrypting image in 5 seconds
                //% "Decrypting"
                qsTrId("foilpics-encrypted_grid-remorse-decrypting"),
                function() { foilModel.decryptAt(index) })
        }
    }

    function deleteItem(index, cellSize, columnCount) {
        jumpToIndex(index, cellSize, columnCount)
        pageStack.pop()
        var item = requestedGroup.currentItem
        if (item) {
            var remorse = decryptRemorseComponent.createObject(null)
            remorse.z = item.z + 1
            remorse.execute(remorseContainerComponent.createObject(item),
                //: Deleting image in 5 seconds
                //% "Deleting"
                qsTrId("foilpics-image_grid_view-remorse-deleting"),
                function() { foilModel.removeAt(index) })
        }
    }

    header: PageHeader {
        id: header
        title: view.selecting ?
            //: Encrypted grid title in selection mode
            //% "Select photos"
            qsTrId("foilpics-encrypted_grid-selection_title") :
            //: Encrypted grid title
            //% "Encrypted"
            qsTrId("foilpics-encrypted_grid-title")
        Badge {
            id: badge
            anchors {
                left: header.extraContent.left
                verticalCenter: header.extraContent.verticalCenter
            }
            maxWidth: header.extraContent.width
            text: foilModel.count ? foilModel.count : ""
        }
        ProgressBar {
            x: foilModel.count ? (badge.x + badge.width) : header.extraContent.x
            width: header.extraContent.x + header.extraContent.width - x
            anchors.verticalCenter: header.extraContent.verticalCenter
            leftMargin: header.leftMargin
            rightMargin: header.rightMargin
            indeterminate: true
            visible: opacity > 0
            opacity: view.busy ? 1 : 0
            Behavior on opacity { FadeAnimation {} }
            Behavior on x { NumberAnimation {} }
        }
    }

    delegate: EncryptedGroup {
        id: delegate
        width: parent.width
        flickable: view
        foilModel: view.foilModel
        title: groupName
        picsModel: groupPicsModel
        picsCount: groupPicsCount
        isFirstGroup: firstGroup
        isDefault: defaultGroup
        onDecryptItem: view.decryptItem(globalIndex, cellSize, columnCount)
        onDeleteItem: view.deleteItem(globalIndex, cellSize, columnCount)
        onRequestIndex: view.jumpToIndex(globalIndex, cellSize, columnCount)
        modelIndex: model.index
        selectionModel: view.selectionModel
        selecting: view.selecting

        Connections {
            target: view
            onRequestedGroupIndexChanged: {
                if (requestedGroupIndex === delegate.modelIndex) {
                    requestedGroup = delegate
                }
            }
        }
    }

    // This container is used for making RemorseItem to follow
    // offset changes if there are multiple deletions ongoing
    // at the same time.
    Component {
        id: remorseContainerComponent
        Item {
            y: parent.contentYOffset
            width: parent.width
            height: parent.height
        }
    }

    Component {
        id: decryptRemorseComponent
        RemorseItem {
            //: RemorseItem cancel help text
            //% "Cancel"
            cancelText: qsTrId("foilpics-encrypted_grid-remorse-cancel_decrypt")
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }
    }

    VerticalScrollDecorator { }
}
