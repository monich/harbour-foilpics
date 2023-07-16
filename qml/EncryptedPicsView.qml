import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

Item {
    id: thisView

    property Page mainPage
    property var hints
    property var foilUi
    property var foilModel
    property bool isCurrentView
    readonly property bool ready: foilModel.foilState === FoilPicsModel.FoilPicsReady
    property alias remorseSelectionModel: listView.selectionModel
    property var selectionModel
    property bool dropSelectionModelsWhenDecryptionDone

    // 400 ms is the pulley menu bounce-back duration
    Behavior on opacity { FadeAnimation { duration: 400 } }

    Component.onCompleted: {
        isCurrentView = parent.isCurrentItem
        listView.maximizeCacheBuffer()
    }

    onIsCurrentViewChanged: {
        if (!isCurrentView) {
            bulkActionRemorse.cancelNicely()
            dropSelectionModels()
        }
    }

    EncryptedList {
        id: listView

        clip: true
        anchors.fill: parent
        foilModel: thisView.foilModel
        model: foilModel.groupModel
        busy: foilModel.busy || progressTimer.running
        //: Encrypted grid title
        //% "Encrypted"
        title: qsTrId("foilpics-encrypted_grid-title")

        PullDownMenu {
            id: pullDownMenu

            property var _deferredAction: null

            visible: thisView.ready

            onActiveChanged: {
                if (!active && _deferredAction) {
                    var action = _deferredAction
                    _deferredAction = null
                    action()
                }
            }

            MenuItem {
                text: foilUi.qsTrEnterPasswordViewMenuGenerateNewKey()
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("foil-ui/FoilUiGenerateKeyWarning.qml"), {
                        allowedOrientations: mainPage.allowedOrientations,
                        foilUi: thisView.foilUi,
                        acceptDestinationProperties: {
                            allowedOrientations: mainPage.allowedOrientations,
                            mainPage: thisView.mainPage,
                            foilUi: thisView.foilUi,
                            foilModel: thisView.foilModel
                        },
                        acceptDestinationAction: PageStackAction.Replace,
                        acceptDestination: Qt.resolvedUrl("foil-ui/FoilUiGenerateKeyPage.qml")
                    })
                }
            }
            MenuItem {
                //: Pulley menu item
                //% "Change password"
                text: qsTrId("foilpics-pulley_menu-change_password")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("foil-ui/FoilUiChangePasswordPage.qml"), {
                        mainPage: thisView.mainPage,
                        foilUi: thisView.foilUi,
                        foilModel: thisView.foilModel,
                        //: Password change prompt
                        //% "Please enter the current and the new password"
                        promptText: qsTrId("foilpics-change_password_page-label-enter_passwords"),
                        //: Placeholder and label for the current password prompt
                        //% "Current password"
                        currentPasswordLabel: qsTrId("foilpics-change_password_page-text_field_label-current_password"),
                        //: Placeholder and label for the new password prompt
                        //% "New password"
                        newPasswordLabel: qsTrId("foilpics-change_password_page-text_field_label-new_password"),
                        //: Button label
                        //% "Change password"
                        buttonText: qsTrId("foilpics-change_password_page-button-change_password")
                    })
                }
            }
            MenuItem {
                //: Pulley menu item
                //% "Lock"
                text: qsTrId("foilpics-pulley_menu-lock")
                onClicked: {
                    thisView.opacity = 0 // with a 400 ms animation
                    pullDownMenu._deferredAction = action
                }
                function action() {
                    foilModel.lock(false)
                }
            }
            MenuItem {
                //: Pulley menu item
                //% "Select photos"
                text: qsTrId("foilpics-pulley_menu-select_photos")
                visible: foilModel.count > 0
                onClicked: selectPictures()
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
            enabled: foilModel && foilModel.count === 0
        }
    }

    Connections {
        target: foilModel
        onBusyChanged: {
            if (foilModel.busy) {
                // Look busy for at least a second
                progressTimer.start()
            } else if (dropSelectionModelsWhenDecryptionDone) {
                dropSelectionModelsWhenDecryptionDone = false
                dropSelectionModels()
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
        onCanceled: dropSelectionModels()
    }

    function dropSelectionModels() {
        if (remorseSelectionModel) {
            remorseSelectionModel.destroy()
            remorseSelectionModel = null
        }
        if (selectionModel) {
            selectionModel.destroy()
            selectionModel = null
        }
    }

    function bulkAction(text, list, callback) {
        listView.jumpToIndex(list[0])
        remorseSelectionModel = selectionModelComponent.createObject(mainPage)
        remorseSelectionModel.makeBusy(list)
        pageStack.pop()
        bulkActionRemorse.execute(text, callback)
    }

    function selectPictures() {
        dropSelectionModels()
        bulkActionRemorse.cancelNicely()
        selectionModel = selectionModelComponent.createObject(mainPage)
        var selectionPage = pageStack.push(Qt.resolvedUrl("EncryptedSelectionPage.qml"), {
            allowedOrientations: mainPage.allowedOrientations,
            selectionModel: thisView.selectionModel,
            foilModel: thisView.foilModel
        })
        selectionPage.deletePictures.connect(function(list) {
            //: Generic remorse popup text
            //% "Deleting %0 selected pictures"
            bulkAction(qsTrId("foilpics-remorse-deleting_selected", list.length).arg(list.length), list, function() {
                foilModel.removeFiles(list)
                dropSelectionModels()
            })
        })
        selectionPage.groupPictures.connect(function(list) {
            var groups = foilModel.groupModel
            var groupPage = pageStack.push(Qt.resolvedUrl("EditGroupPage.qml"), {
                groupModel: groups
            })
            groupPage.groupSelected.connect(function(index) {
                foilModel.setGroupIdForRows(list, groups.groupId(index))
                dropSelectionModels()
                pageStack.pop(mainPage)
            })
        })
        selectionPage.decryptPictures.connect(function(list) {
            //: Remorse popup text
            //% "Decrypting %0 selected pictures"
            bulkAction(qsTrId("foilpics-encrypted_pics_view-remorse-decrypting_selected", list.length).arg(list.length), list, function() {
                foilModel.decryptFiles(list)
                if (foilModel.busy) {
                    dropSelectionModelsWhenDecryptionDone = true
                } else {
                    // Well, this shouldn't happen but just in case...
                    dropSelectionModels()
                }
            })
        })
    }

    Connections {
        target: mainPage
        onStatusChanged: maybeDropSelectionModel()
    }

    Connections {
        target: pageStack
        onCurrentPageChanged: maybeDropSelectionModel()
    }

    function maybeDropSelectionModel() {
        if (selectionModel && mainPage.status === PageStatus.Active && pageStack.currentPage === mainPage) {
            selectionModel.destroy()
            selectionModel = null
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
