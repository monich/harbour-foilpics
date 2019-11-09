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
    readonly property bool canEnterPassword: inputField.text.length > 0 && !unlocking &&
                                    !wrongPasswordAnimation.running && !wrongPassword

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

    Rectangle {
        id: circle

        anchors.horizontalCenter: parent.horizontalCenter
        width: Theme.itemSizeHuge
        y: (panel.y > height) ? Math.floor((panel.y - height)/2) : (panel.y - height)
        height: width
        color: Theme.primaryColor
        radius: width/2
        visible: opacity > 0

        // Hide it when it's only partially visible (i.e. in langscape)
        readonly property real maxOpacity: HarbourTheme.opacityFaint * HarbourTheme.opacityLow
        opacity: (y < 0) ? 0 : maxOpacity
        Behavior on opacity { FadeAnimation { duration: landscapeLayout ? 0 : 100 } }
    }

    Image {
        source: "images/lock.svg"
        height: Math.floor(circle.height * 5 / 8)
        sourceSize.height: height
        anchors.centerIn: circle
        opacity: circle.opacity/circle.maxOpacity
        visible: opacity > 0
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        y: (parent.height > height) ? Math.floor((parent.height - height)/2) : (parent.height - height)

        readonly property bool showLongPrompt: y >= 0

        InfoLabel {
            id: longPrompt

            height: implicitHeight
            visible: panel.showLongPrompt
            //: Password prompt label (long)
            //% "Secret pictures are locked. Please enter your password"
            text: qsTrId("foilpics-enter_password_view-label-enter_password_long")
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
            onTextChanged: view.wrongPassword = false
            EnterKey.onClicked: view.enterPassword()
            EnterKey.enabled: view.canEnterPassword
        }

        Button {
            id: button

            text: unlocking ?
                //: Button label
                //% "Unlocking..."
                qsTrId("foilpics-enter_password_view-button-unlocking") :
                //: Button label
                //% "Unlock"
                qsTrId("foilpics-enter_password_view-button-unlock")
            enabled: view.canEnterPassword
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
