import QtQuick 2.0
import Sailfish.Silica 1.0

ImageGridView {
    id: grid
    anchors.fill: parent

    property var hints
    property var foilModel
    readonly property int animationDuration: 150
    property var transferMethodsModel
    property alias contextMenu: contextMenuItem
    property Item expandItem
    property real expandHeight: contextMenu.height
    property int minOffsetIndex: expandItem != null ?
        expandItem.modelIndex + columnCount - (expandItem.modelIndex % columnCount) : 0

    function encryptItem(index) {
        pageStack.pop()
        grid.currentIndex = index
        grid.currentItem.encrypt()
        grid.positionViewAtIndex(index, GridView.Visible)
    }

    header: PageHeader {
        id: header
        //: Gallery grid title
        //% "Photos"
        title: qsTrId("foilpics-gallery_grid-title")
        Badge {
            anchors {
                left: header.extraContent.left
                verticalCenter: header.extraContent.verticalCenter
            }
            maxWidth: header.extraContent.width
            height: Theme.itemSizeSmall/2
            text: model.count ? model.count : ""
            opacity: model.count ? 1 : 0
        }
    }

    delegate: ThumbnailImage {
        id: delegate

        property bool isItemExpanded: grid.expandItem === delegate
        property url mediaUrl: url
        property int modelIndex: index

        source: mediaUrl
        size: grid.cellSize
        height: isItemExpanded ? grid.contextMenu.height + grid.cellSize : grid.cellSize
        contentYOffset: index >= grid.minOffsetIndex ? grid.expandHeight : 0.0
        z: isItemExpanded ? 1000 : 1
        enabled: isItemExpanded || !grid.contextMenu.active

        function encrypt() {
            foilModel.encryptFile(delegate.mediaUrl, {
                orientation: model.orientation,
                imageDate: model.dateTaken,
                cameraManufacturer: model.cameraManufacturer,
                cameraModel: model.cameraModel
            })
            // Count this as a hint:
            if (hints.letsEncryptSomething < MaximumHintCount) hints.letsEncryptSomething++
            rightSwipeToEncryptedHintLoader.armed = true
        }

        function remove() {
            requestDelete(delegate, function() { FileRemover.deleteMedia(delegate.mediaUrl) })
        }

        onClicked: {
            if (!grid.contextMenu.active) {
                var page = pageStack.push(Qt.resolvedUrl("GalleryFullscreenPage.qml"), {
                    currentIndex: index,
                    model: grid.model,
                    foilModel: grid.foilModel,
                    transferMethodsModel: transferMethodsModel
                })
                if (page) {
                    page.encryptItem.connect(grid.encryptItem)
                    page.deleteItem.connect(grid.deleteItem)
                    page.requestIndex.connect(grid.jumpToIndex)
                }
            }
        }

        onPressAndHold: {
            grid.expandItem = delegate
            grid.contextMenu.show(delegate)
        }

        GridView.onAdd: AddAnimation { target: delegate; duration: animationDuration }
        GridView.onRemove: SequentialAnimation {
            PropertyAction { target: delegate; property: "GridView.delayRemove"; value: true }
            NumberAnimation { target: delegate; properties: "opacity,scale"; to: 0; duration: 250; easing.type: Easing.InOutQuad }
            PropertyAction { target: delegate; property: "GridView.delayRemove"; value: false }
        }
    }

    ContextMenu {
        id: contextMenuItem
        x: parent !== null ? -parent.x : 0.0

        MenuItem {
            //: Generic menu item
            //% "Encrypt"
            text: qsTrId("foilpics-menu-encrypt")
            visible: foilModel.keyAvailable
            onClicked: grid.expandItem.encrypt()
        }
        MenuItem {
            //: Generic menu item
            //% "Delete"
            text: qsTrId("foilpics-menu-delete")
            onClicked: grid.expandItem.remove()
        }
    }

    Loader {
        id: rightSwipeToEncryptedHintLoader
        anchors.fill: parent
        active: opacity > 0
        opacity: ((hints.rightSwipeToEncrypted < MaximumHintCount && armed) | running) ? 1 : 0
        property bool armed
        property bool running
        sourceComponent: Component {
            LeftRightSwipeHint {
                //: Right swipe hint text
                //% "Encrypted pictures are moved there to the left"
                text: qsTrId("foilpics-hint-swipe_right_to_encrypted")
                swipeRight: true
                hintEnabled: rightSwipeToEncryptedHintLoader.armed && !hintDelayTimer.running
                onHintShown: {
                    hints.rightSwipeToEncrypted++
                    rightSwipeToEncryptedHintLoader.armed = false
                }
                onHintRunningChanged: rightSwipeToEncryptedHintLoader.running = hintRunning
                Timer {
                    id: hintDelayTimer
                    interval: 1000
                    running: true
                }
            }
        }
        Behavior on opacity { FadeAnimation {} }
    }
}
