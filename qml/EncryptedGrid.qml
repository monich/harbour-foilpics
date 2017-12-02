import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

ImageGridView {
    id: grid

    property var hints
    property alias foilModel: grid.model
    property alias pulleyFlickable: pullDownMenu.flickable
    readonly property int animationDuration: 150

    property alias contextMenu: contextMenuItem
    property Item expandItem
    property real expandHeight: contextMenu.height
    readonly property bool busy: foilModel.busy || progressTimer.running
    readonly property bool ready: foilModel.foilState === FoilPicsModel.FoilPicsReady
    readonly property int minOffsetIndex: expandItem != null ?
                                            (expandItem.modelIndex + columnCount - (expandItem.modelIndex % columnCount)) : 0

    function decryptItem(index) {
        pageStack.pop()
        grid.currentIndex = index
        grid.currentItem.decrypt()
        grid.positionViewAtIndex(index, GridView.Visible)
    }

    function requestDecrypt(item, callback) {
        var remorse = decryptRemorseComponent.createObject(null)
        remorse.z = item.z + 1
        remorse.execute(createRemorseContainer(item),
            //: Decrypting image in 5 seconds
            //% "Decrypting"
            qsTrId("foilpics-encrypted_grid-remorse-decrypting"),
            callback)
    }

    PullDownMenu {
        id: pullDownMenu
        visible: grid.ready
        MenuItem {
            //: Pulley menu item
            //% "Generate a new key"
            text: qsTrId("foilpics-pulley_menu-generate_key")
            visible: !foilModel.count
            onClicked: {
                pageStack.push(Qt.resolvedUrl("GenerateKeyPage.qml"), {
                    foilModel: foilModel
                })
            }
        }
        MenuItem {
            //: Pulley menu item
            //% "Change password"
            text: qsTrId("foilpics-pulley_menu-change_password")
            onClicked: {
                pageStack.push(Qt.resolvedUrl("ChangePasswordDialog.qml"), {
                    foilModel: foilModel
                })
            }
        }
        MenuItem {
            //: Pulley menu item
            //% "Decrypt all"
            text: qsTrId("foilpics-pulley_menu-decrypt_all")
            visible: foilModel.count > 0
            onClicked: decryptAllRemorse.execute(
                //: Decrypting all pictures in 5 seconds
                //% "Decrypting all pictures"
                qsTrId("foilpics-pulley_menu-remorse-decrypting_all"),
                function() { foilModel.decryptAll() })
        }
    }

    Connections {
        target: foilModel
        onBusyChanged: {
            if (foilModel.busy) {
                // Look busy for at least a second
                progressTimer.start()
            }
        }
    }

    Timer {
        id: progressTimer
        interval: 1000
        running: true
    }

    header: PageHeader {
        id: header
        //: Encrypted grid title
        //% "Encrypted"
        title: qsTrId("foilpics-encrypted_grid-title")
        Badge {
            id: badge
            anchors {
                left: header.extraContent.left
                verticalCenter: header.extraContent.verticalCenter
            }
            height: Theme.itemSizeSmall/2
            maxWidth: header.extraContent.width
            opacity: foilModel.count ? 1 : 0
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
            opacity: grid.busy ? 1 : 0
            Behavior on opacity { FadeAnimation {} }
            Behavior on x { NumberAnimation {} }
        }
    }

    delegate: ThumbnailBase {
        id: delegate
        width: grid.cellWidth
        height: isItemExpanded ? grid.contextMenu.height + grid.cellHeight : grid.cellHeight
        contentYOffset: index >= grid.minOffsetIndex ? grid.expandHeight : 0.0
        z: isItemExpanded ? 1000 : 1
        enabled: isItemExpanded || !grid.contextMenu.active

        property bool isItemExpanded: grid.expandItem === delegate
        property int modelIndex: index

        Image {
            y: contentYOffset
            x: contentXOffset
            fillMode: Image.PreserveAspectCrop
            source: thumbnail
            width:  grid.cellWidth
            height: grid.cellHeight
            clip: true
        }

        function remove() {
            requestDelete(delegate, function() { foilModel.removeAt(modelIndex) })
        }

        function decrypt() {
            requestDecrypt(delegate, function() {
                foilModel.decryptAt(modelIndex)
                leftSwipeToDecryptedHintLoader.armed = true
            })
        }

        onClicked: {
            if (!grid.contextMenu.active) {
                var page = pageStack.push(Qt.resolvedUrl("EncryptedFullscreenPage.qml"),{
                    currentIndex: index,
                    model: grid.model})
                if (page) {
                    page.decryptItem.connect(grid.decryptItem)
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
            //% "Delete"
            text: qsTrId("foilpics-menu-delete")
            onClicked: grid.expandItem.remove()
        }
        MenuItem {
            //: Generic menu item
            //% "Decrypt"
            text: qsTrId("foilpics-menu-decrypt")
            onClicked: grid.expandItem.decrypt()
        }
    }

    ViewPlaceholder {
        text: hints.letsEncryptSomething < MaximumHintCount ?
            //: Placeholder text with a hint
            //% "Why not to encrypt something?"
            qsTrId("foilpics-encrypted_grid-placeholder-no_pictures_hint") :
            //: Placeholder text
            //% "You don't have any encrypted pictures"
            qsTrId("foilpics-encrypted_grid-placeholder-no_pictures")
        enabled: !grid.busy && foilModel && foilModel.count === 0
    }

    RemorsePopup {
        id: decryptAllRemorse
    }

    Component {
        id: decryptRemorseComponent
        RemorseItem {
            //: RemorseItem cancel help text
            //% "Cancel"
            cancelText: qsTrId("foilpics-encrypted_grid-remorse-cancel_decrypt")
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeSmallBase
        }
    }

    Loader {
        id: leftSwipeToDecryptedHintLoader
        anchors.fill: parent
        active: opacity > 0
        opacity: ((hints.leftSwipeToDecrypted < MaximumHintCount && armed) | running) ? 1 : 0
        property bool armed
        property bool running
        sourceComponent: Component {
            LeftRightSwipeHint {
                //: Left swipe hint text
                //% "Decrypted pictures are moved back to the gallery"
                text: qsTrId("foilpics-hint-swipe_left_to_decrypted")
                hintEnabled: leftSwipeToDecryptedHintLoader.armed && !hintDelayTimer.running
                onHintShown: {
                    hints.leftSwipeToDecrypted++
                    leftSwipeToDecryptedHintLoader.armed = false
                }
                onHintRunningChanged: leftSwipeToDecryptedHintLoader.running = hintRunning
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
