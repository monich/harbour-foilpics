import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

SilicaFlickable {
    id: view

    property var foilModel
    property bool wrongPassword

    readonly property bool landscapeLayout: appLandscapeMode && Screen.sizeCategory < Screen.Large
    readonly property bool unlocking: foilModel.foilState !== FoilPicsModel.FoilLocked &&
                                    foilModel.foilState !== FoilPicsModel.FoilLockedTimedOut

    function enterPassword() {
        if (!foilModel.unlock(inputField.text)) {
            wrongPassword = true
            wrongPasswordAnimation.start()
            inputField.requestFocus()
        }
    }

    PullDownMenu {
        id: pullDownMenu

        MenuItem {
            //: Pulley menu item
            //% "Generate a new key"
            text: qsTrId("foilpics-pulley_menu-generate_key")
            onClicked: {
                if (foilModel.mayHaveEncryptedPictures) {
                    var warning = pageStack.push(Qt.resolvedUrl("GenerateKeyWarning.qml"));
                    warning.accepted.connect(function() {
                        // Replace the warning page with a slide. This may
                        // occasionally generate "Warning: cannot pop while
                        // transition is in progress" if the user taps the
                        // page stack indicator (as opposed to the Accept
                        // button) but this warning is fairly harmless:
                        //
                        // _dialogDone (Dialog.qml:124)
                        // on_NavigationChanged (Dialog.qml:177)
                        // navigateForward (PageStack.qml:291)
                        // onClicked (private/PageStackIndicator.qml:174)
                        //
                        warning.canNavigateForward = false
                        pageStack.replace(Qt.resolvedUrl("GenerateKeyPage.qml"), {
                            foilModel: foilModel
                        })
                    })
                } else {
                    pageStack.push(Qt.resolvedUrl("GenerateKeyPage.qml"), {
                        foilModel: foilModel
                    })
                }
            }
        }
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        anchors.verticalCenter: parent.verticalCenter

        readonly property bool showLongPrompt: y >= 0

        InfoLabel {
            id: longPrompt

            height: implicitHeight
            visible: panel.showLongPrompt
            //: Password prompt label (long)
            //% "Secret pictures are locked. Please enter your password"
            text: qsTrId("foilpics-enter_password_view-label-enter_password")
        }

        InfoLabel {
            height: implicitHeight
            anchors.bottom: longPrompt.bottom
            visible: !panel.showLongPrompt
            //: Password prompt label (short)
            //% "Please enter your password"
            text: qsTrId("foilpics-enter_password_view-label-enter_password_short")
        }

        HarbourPasswordInputField {
            id: inputField

            anchors {
                left: panel.left
                top: longPrompt.bottom
                topMargin: Theme.paddingLarge
            }
            enabled: !unlocking
            EnterKey.onClicked: view.enterPassword()
            onTextChanged: view.wrongPassword = false
        }

        Button {
            id: button

            anchors.horizontalCenter: parent.horizontalCenter
            text: unlocking ?
                //: Button label
                //% "Unlocking..."
                qsTrId("foilpics-enter_password_view-button-unlocking") :
                //: Button label
                //% "Unlock"
                qsTrId("foilpics-enter_password_view-button-unlock")
            enabled: inputField.text.length > 0 && !unlocking && !wrongPasswordAnimation.running && !wrongPassword
            onClicked: view.enterPassword()
        }
    }

    HarbourShakeAnimation  {
        id: wrongPasswordAnimation

        target: panel
    }

    states: [
        State {
            name: "portrait"
            when: !landscapeLayout
            changes: [
                AnchorChanges {
                    target: inputField
                    anchors.right: panel.right
                },
                PropertyChanges {
                    target: inputField
                    anchors.rightMargin: 0
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: inputField.bottom
                        horizontalCenter: parent.horizontalCenter
                    }
                },
                PropertyChanges {
                    target: button
                    anchors {
                        topMargin: Theme.paddingLarge
                        rightMargin: 0
                    }
                }
            ]
        },
        State {
            name: "landscape"
            when: landscapeLayout
            changes: [
                AnchorChanges {
                    target: inputField
                    anchors.right: button.left
                },
                PropertyChanges {
                    target: inputField
                    anchors.rightMargin: Theme.horizontalPageMargin
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: longPrompt.bottom
                        right: panel.right
                        horizontalCenter: undefined
                    }
                },
                PropertyChanges {
                    target: button
                    anchors {
                        topMargin: Theme.paddingLarge
                        rightMargin: Theme.horizontalPageMargin
                    }
                }
            ]
        }
    ]
}
