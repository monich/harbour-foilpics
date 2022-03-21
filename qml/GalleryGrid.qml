import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

ImageGridView {
    id: grid

    property var hints
    property var foilModel
    property var selectionModel
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
        HarbourBadge {
            anchors {
                left: header.extraContent.left
                verticalCenter: header.extraContent.verticalCenter
            }
            maxWidth: header.extraContent.width
            text: model.count ? model.count : ""
        }
    }

    delegate: ThumbnailImage {
        id: delegate

        source: mediaUrl
        size: grid.cellSize
        height: isItemExpanded ? grid.contextMenu.height + grid.cellSize : grid.cellSize
        contentYOffset: index >= grid.minOffsetIndex ? grid.expandHeight : 0.0
        z: isItemExpanded ? 1000 : 1
        enabled: isItemExpanded || !grid.contextMenu.active
        selectionModel: grid.selectionModel
        selectionKey: mediaUrl

        readonly property url mediaUrl: url
        readonly property bool isItemExpanded: grid.expandItem === delegate
        readonly property int modelIndex: index

        function metadata() {
            return {
                orientation: model.orientation,
                imageDate: model.dateTaken,
                cameraManufacturer: model.cameraManufacturer,
                cameraModel: model.cameraModel,
                latitude: model.latitude,
                longitude: model.longitude,
                altitude: model.altitude
            }
        }

        function encrypt() {
            foilModel.encryptFile(delegate.mediaUrl, metadata())
            // Count this as a hint:
            if (hints.letsEncryptSomething < MaximumHintCount) hints.letsEncryptSomething++
            rightSwipeToEncryptedHintLoader.armed = true
        }

        function remove() {
            requestDelete(delegate, function() {
                FoilPics.deleteLocalFile(delegate.mediaUrl)
            })
        }

        function showDetails() {
            pageStack.push(Qt.resolvedUrl("GalleryDetailsPage.qml"), {
                item: model.itemId
            })
        }

        onClicked: {
            if (!isBusy && !grid.contextMenu.active) {
                var page = pageStack.push(Qt.resolvedUrl("GalleryFullscreenPage.qml"), {
                    currentIndex: index,
                    model: grid.model,
                    foilModel: grid.foilModel
                })
                if (page) {
                    page.encryptItem.connect(grid.encryptItem)
                    page.deleteItem.connect(grid.deleteItem)
                    page.requestIndex.connect(grid.jumpToIndex)
                }
            }
        }

        onPressAndHold: {
            if (!isBusy) {
                grid.expandItem = delegate
                contextMenuItem.openMenu(delegate)
            }
        }

        GridView.onAdd: AddAnimation { target: delegate }
        GridView.onRemove: SequentialAnimation {
            PropertyAction { target: delegate; property: "GridView.delayRemove"; value: true }
            NumberAnimation { target: delegate; properties: "opacity,scale"; to: 0; duration: 250; easing.type: Easing.InOutQuad }
            PropertyAction { target: delegate; property: "GridView.delayRemove"; value: false }
        }
    }

    ContextMenu {
        id: contextMenuItem

        x: parent !== null ? -parent.x : 0.0

        function openMenu(item) {
            // ContextMenu::show is deprecated in Sailfish Silica package 0.25.6 (Dec 2017)
            if ("open" in contextMenuItem) {
                open(item)
            } else {
                show(item)
            }
        }

        function closeMenu() {
            // ContextMenu::hide is deprecated in Sailfish Silica package 0.25.6 (Dec 2017)
            if ("close" in contextMenuItem) {
                close()
            } else {
                hide()
            }
        }

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
        MenuItem {
            //: Generic menu item
            //% "Image details"
            text: qsTrId("foilpics-menu-details")
            onClicked: grid.expandItem.showDetails()
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
            HarbourHorizontalSwipeHint {
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
