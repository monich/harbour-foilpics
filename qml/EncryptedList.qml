import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

SilicaListView {
    id: view

    property var hints
    property var foilModel
    property alias pulleyFlickable: pullDownMenu.flickable

    readonly property bool busy: foilModel.busy || progressTimer.running
    readonly property bool ready: foilModel.foilState === FoilPicsModel.FoilPicsReady

    property int requestedGroupIndex: -1
    property var requestedGroup

    model: foilModel.groupModel

    RemorsePopup {
        id: decryptAllRemorse
    }

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

    PullDownMenu {
        id: pullDownMenu
        visible: view.ready
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
        onDecryptionStarted: leftSwipeToDecryptedHintLoader.armed = true
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
        isDefault: defaultGroup
        onDecryptItem: view.decryptItem(globalIndex, cellSize, columnCount)
        onDeleteItem: view.deleteItem(globalIndex, cellSize, columnCount)
        onRequestIndex: view.jumpToIndex(globalIndex, cellSize, columnCount)
        modelIndex: model.index

        Connections {
            target: view
            onRequestedGroupIndexChanged: {
                if (requestedGroupIndex === delegate.modelIndex) {
                    requestedGroup = delegate
                }
            }
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
        enabled: !view.busy && foilModel && foilModel.count === 0
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

    VerticalScrollDecorator { }
}
