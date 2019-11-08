import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

SilicaFlickable {
    id: view

    property var hints
    property var foilModel
    property var pulleyFlickable
    property bool isCurrentView
    readonly property bool ready: foilModel.foilState === FoilPicsModel.FoilPicsReady
    property alias selectionModel: listView.selectionModel
    property bool dropSelectionModelWhenDecryptionDone

    Component.onCompleted: {
        isCurrentView = parent.isCurrentItem
        listView.maximizeCacheBuffer()
    }

    onIsCurrentViewChanged: {
        if (!isCurrentView) {
            bulkActionRemorse.cancelNicely()
            dropSelectionModel()
        }
    }

    PullDownMenu {
        visible: view.ready
        flickable: pulleyFlickable
        MenuItem {
            id: generateKeyMenuItem
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
            id: selectPhotosMenuItem
            //: Pulley menu item
            //% "Select photos"
            text: qsTrId("foilpics-pulley_menu-select_photos")
            visible: foilModel.count > 0
            onClicked: selectPictures()
        }
    }

    EncryptedList {
        id: listView

        clip: true
        anchors.fill: parent
        foilModel: view.foilModel
        model: foilModel.groupModel
        busy: foilModel.busy || progressTimer.running
        //: Encrypted grid title
        //% "Encrypted"
        title: qsTrId("foilpics-encrypted_grid-title")
    }

    Connections {
        target: foilModel
        onBusyChanged: {
            if (foilModel.busy) {
                // Look busy for at least a second
                progressTimer.start()
            } else if (dropSelectionModelWhenDecryptionDone) {
                dropSelectionModelWhenDecryptionDone = false
                dropSelectionModel()
            }
        }
        onDecryptionStarted: leftSwipeToDecryptedHintLoader.armed = true
    }

    // The parent loader is supposed to have isCurrentItem property
    Connections {
        target: parent
        onIsCurrentItemChanged: isCurrentView = target.isCurrentItem
    }

    Timer {
        id: progressTimer

        interval: 1000
        running: true
    }

    // Selection model is fairly expensive, we create and maintain it
    // only in selection mode
    Component {
        id: selectionModelComponent

        FoilPicsSelection {
            model: foilModel
            role: "imageId"
            duplicatesAllowed: false
        }
    }

    RemorsePopup {
        id: bulkActionRemorse

        function cancelNicely() {
            // To avoid flickering, do it only when really necessary
            if (visible) cancel()
        }
        onCanceled: dropSelectionModel()
    }

    function dropSelectionModel() {
        if (selectionModel) {
            selectionModel.destroy()
            selectionModel = null
        }
    }

    function selectPictures() {
        dropSelectionModel()
        bulkActionRemorse.cancelNicely()
        selectionModel = selectionModelComponent.createObject(view, { model: foilModel })
        var selectionPage = pageStack.push(Qt.resolvedUrl("EncryptedSelectionPage.qml"), {
            allowedOrientations: page.allowedOrientations,
            selectionModel: view.selectionModel,
            foilModel: view.foilModel
        })
        selectionPage.deletePictures.connect(function(list) {
            pageStack.pop()
            listView.jumpToIndex(list[0])
            //: Generic remorse popup text
            //% "Deleting %0 selected pictures"
            bulkActionRemorse.execute(qsTrId("foilpics-remorse-deleting_selected", list.length).arg(list.length), function() {
                foilModel.removeFiles(list)
                dropSelectionModel()
            })
        })
        selectionPage.groupPictures.connect(function(list) {
            var groups = foilModel.groupModel
            var groupPage = pageStack.replace(Qt.resolvedUrl("EditGroupPage.qml"), {
                groupModel: groups
            })
            groupPage.groupSelected.connect(function(index) {
                foilModel.setGroupIdForRows(list, groups.groupId(index))
                pageStack.pop()
                dropSelectionModel()
            })
        })
        selectionPage.decryptPictures.connect(function(list) {
            pageStack.pop()
            listView.jumpToIndex(list[0])
            //: Remorse popup text
            //% "Decrypting %0 selected pictures"
            bulkActionRemorse.execute(qsTrId("foilpics-encrypted_pics_view-remorse-decrypting_selected", list.length).arg(list.length), function() {
                console.log(list)
                foilModel.decryptFiles(list)
                if (foilModel.busy) {
                    dropSelectionModelWhenDecryptionDone = true
                } else {
                    // Well, this shouldn't happen but just in case...
                    dropSelectionModel()
                }
            })
        })
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
            HarbourHorizontalSwipeHint {
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
