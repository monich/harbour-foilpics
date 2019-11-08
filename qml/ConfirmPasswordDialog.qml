import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Dialog {
    id: dialog

    allowedOrientations: window.allowedOrientations
    forwardNavigation: false

    property string password
    property bool wrongPassword
    readonly property bool landscapeLayout: appLandscapeMode && Screen.sizeCategory < Screen.Large
    readonly property bool canCheckPassword: inputField.text.length > 0 &&
                                             inputField.text.length > 0 && !wrongPassword

    signal passwordConfirmed()

    onStatusChanged: {
        if (status === PageStatus.Activating) {
            inputField.requestFocus()
        }
    }

    function checkPassword() {
        if (inputField.text === password) {
            dialog.passwordConfirmed()
        } else {
            wrongPassword = true
            wrongPasswordAnimation.start()
            inputField.requestFocus()
        }
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height

        y: (parent.height > height) ? Math.floor((parent.height - height)/2) : (parent.height - height)

        InfoLabel {
            id: prompt

            //: Password confirmation label
            //% "Please type in your new password one more time"
            text: qsTrId("foilpics-confirm_password_page-info_label")

            // Hide it when it's only partially visible
            opacity: (panel.y < 0) ? 0 : 1
            Behavior on opacity { FadeAnimation {} }
        }

        Label {
            id: warning

            //: Password confirmation description
            //% "Make sure you don't forget your password. It's impossible to either recover it or to access the encrypted pictures without knowing it. Better take it seriously."
            text: qsTrId("foilpics-confirm_password_page-description")

            anchors {
                left: prompt.left
                right: prompt.right
                top: prompt.bottom
                topMargin: Theme.paddingLarge
            }
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
        }

        HarbourPasswordInputField {
            id: inputField

            anchors {
                left: panel.left
                top: warning.bottom
                topMargin: Theme.paddingLarge
            }
            //: Placeholder for the password confirmation prompt
            //% "New password again"
            placeholderText: qsTrId("foilpics-confirm_password_page-text_field_placeholder-new_password")
            //: Label for the password confirmation prompt
            //% "New password"
            label: qsTrId("foilpics-confirm_password_page-text_field_label-new_password")
            onTextChanged: dialog.wrongPassword = false
            EnterKey.enabled: dialog.canCheckPassword
            EnterKey.onClicked: dialog.checkPassword()
        }

        Button {
            id: button

            //: Button label (confirm password)
            //% "Confirm"
            text: qsTrId("foilpics-confirm_password_page-button-confirm")
            enabled: dialog.canCheckPassword
            onClicked: dialog.checkPassword()
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
                        top: warning.bottom
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
