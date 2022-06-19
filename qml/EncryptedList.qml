import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

SilicaListView {
    id: view

    property var foilModel
    property var selectionModel
    property string title
    property bool selectable
    property bool busy

    property int requestedGroupIndex: -1
    property var requestedGroup

    readonly property real cellSize: Math.floor(width / columnCount)
    readonly property int columnCount: Math.floor(width / Theme.itemSizeHuge)

    function maximizeCacheBuffer() {
        // cacheBuffer is just a rough estimate just to keep all delegate
        // instantiated, so that scroll indicator shows the actual position.
        cacheBuffer = cellSize * (Math.floor(foilModel.count / columnCount) + model.count)
    }

    function jumpToIndex(index) {
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

    function decryptItem(index) {
        jumpToIndex(index)
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

    function deleteItem(index) {
        jumpToIndex(index)
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

        title: view.title

        HarbourBadge {
            id: badge

            anchors {
                left: header.extraContent.left
                verticalCenter: header.extraContent.verticalCenter
            }
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

        // PageHeader didn't have "interactive" in earlier versions of SFOS
        Component.onCompleted: {
            if ('interactive' in header) {
                header.interactive = false
            }
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
        onDecryptItem: view.decryptItem(globalIndex)
        onDeleteItem: view.deleteItem(globalIndex)
        onRequestIndex: view.jumpToIndex(globalIndex)
        modelIndex: model.index
        selectionModel: view.selectionModel
        selectable: view.selectable
        expanded: groupExpanded

        onToggleExpanded: groupExpanded = !groupExpanded

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
