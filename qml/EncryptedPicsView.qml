import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

SilicaFlickable {
    id: view

    property var hints
    property var foilModel
    property var pulleyFlickable
    property bool isCurrentView
    readonly property bool ready: foilModel.foilState === FoilPicsModel.FoilPicsReady

    property alias selecting: list.selecting
    property alias selectionModel: list.selectionModel
    readonly property int selectedPicturesCount: selectionModel ? selectionModel.count : 0

    PullDownMenu {
        visible: view.ready
        flickable: pulleyFlickable
        MenuItem {
            id: generateKeyMenuItem
            //: Pulley menu item
            //% "Generate a new key"
            text: qsTrId("foilpics-pulley_menu-generate_key")
            visible: !foilModel.count && !selecting
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
            id: selectPhotosMenuItem
            //: Pulley menu item
            //% "Select photos"
            text: qsTrId("foilpics-pulley_menu-select_photos")
            visible: !selecting && foilModel.count > 0
            onClicked: selecting = true
        }
        MenuItem {
            id: selectNoneMenuItem
            //: Pulley menu item
            //% "Select none"
            text: qsTrId("foilpics-pulley_menu-select_none")
            visible: selecting && selectedPicturesCount > 0
            onClicked: selectionModel.clearSelection()
        }
        MenuItem {
            id: selectAllMenuItem
            //: Pulley menu item
            //% "Select all"
            text: qsTrId("foilpics-pulley_menu-select_all")
            visible: selecting && selectedPicturesCount < foilModel.count
            onClicked: selectionModel.selectAll()
        }
    }

    RemorsePopup {
        id: bulkActionRemorse
        function cancelNicely() {
            // To avoid flickering, do it only when really necessary
            if (visible) cancel()
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
        onCountChanged: if (!foilModel.count) selecting = false
        onDecryptionStarted: leftSwipeToDecryptedHintLoader.armed = true
    }

    // The parent loader is supposed to have isCurrentItem property

    Connections {
        target: parent
        onIsCurrentItemChanged: isCurrentView = target.isCurrentItem
    }

    Component.onCompleted: isCurrentView = parent.isCurrentItem
    onIsCurrentViewChanged: if (!isCurrentView) selecting = false

    // Selection model is fairly expensive, we create and maintain it
    // only in selection mode

    Component {
        id: selectionModelComponent
        FoilPicsSelection {
            model: foilModel
            role: "imageId"
            duplicatesAllowed: false
            onSelectionChanged: bulkActionRemorse.cancelNicely()
            onSelectionCleared: bulkActionRemorse.cancelNicely()
        }
    }

    onSelectingChanged: {
        if (selecting) {
            selectionModel = selectionModelComponent.createObject(view)
        } else {
            bulkActionRemorse.cancelNicely()
            selectionModel.destroy()
            selectionModel = null
        }
    }

    Timer {
        id: progressTimer
        interval: 1000
        running: true
    }

    EncryptedList {
        id: list
        clip: true
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: selectionPanel.top
        }
        foilModel: view.foilModel
        model: view.foilModel.groupModel
        busy: foilModel.busy || progressTimer.running
    }

    EncryptedSelectionPanel {
        id: selectionPanel
        active: selecting
        enableActions: selectedPicturesCount > 0
        //: Generic menu item
        //% "Delete"
        onDeleteHint: showHint(qsTrId("foilpics-menu-delete"))
        onDeleteSelected: bulkActionRemorse.execute(
            //: Generic remorse popup text
            //% "Deleting %0 selected pictures"
            qsTrId("foilpics-remorse-deleting_selected", selectedPicturesCount).arg(selectedPicturesCount),
            function() {
                foilModel.removeFiles(selectionModel.makeSelectionBusy())
            })
        //: Generic menu item
        //% "Group"
        onGroupHint: showHint(qsTrId("foilpics-menu-group"))
        onGroupSelected: {
            var groupPage = pageStack.push(Qt.resolvedUrl("EditGroupPage.qml"), {
                groupModel: foilModel.groupModel
            })
            groupPage.groupSelected.connect(function(index) {
                if (selectionModel) {
                    var rows = selectionModel.getSelectedRows()
                    selectionModel.clearSelection()
                    foilModel.setGroupIdForRows(rows, foilModel.groupModel.groupId(index))
                }
                pageStack.pop()
            })
        }
        //: Generic menu item
        //% "Decrypt"
        onDecryptHint: showHint(qsTrId("foilpics-menu-decrypt"))
        onDecryptSelected: bulkActionRemorse.execute(
            //: Remorse popup text
            //% "Decrypting %0 selected pictures"
            qsTrId("foilpics-encrypted_pics_view-remorse-decrypting_selected", selectedPicturesCount).arg(selectedPicturesCount),
            function() {
                foilModel.decryptFiles(selectionModel.makeSelectionBusy())
            })
        //: Button that exits selection mode
        //% "Done"
        onDoneHint: showHint(qsTrId("foilpics-selection_panel-done_button"))
        onDone: selecting = false
    }

    function showHint(text) {
        selectionHint.text = text
        selectionHintTimer.restart()
    }

    InteractionHintLabel {
        id: selectionHint
        anchors.fill: list
        visible: opacity > 0
        opacity: selectionHintTimer.running ? 1.0 : 0.0
        Behavior on opacity { FadeAnimation { duration: 1000 } }
    }

    Timer {
        id: selectionHintTimer
        interval: 1000
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
