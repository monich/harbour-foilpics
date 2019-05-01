import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

Item {
    id: view

    property var foilModel
    property alias prompt: promptLabel.text
    readonly property int minPassphraseLen: 8
    readonly property bool generating: foilModel.foilState === FoilPicsModel.FoilGeneratingKey
    readonly property bool landscapeLayout: appLandscapeMode && Screen.sizeCategory < Screen.Large

    function generateKey() {
        var dialog = pageStack.push(Qt.resolvedUrl("ConfirmPasswordDialog.qml"), {
            password: passphrase.text
        })
        dialog.passwordConfirmed.connect(function() {
            dialog.backNavigation = false
            foilModel.generateKey(keySize.value, passphrase.text)
            dialog.forwardNavigation = true
            dialog.accept()
        })
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        y: (parent.height > height) ? Math.floor((parent.height - height)/2) : (parent.height - height)

        InfoLabel {
            id: promptLabel

            height: implicitHeight
            opacity: (parent.y >= 0) ? 1 : 0

            Behavior on opacity { FadeAnimation { } }
        }

        ComboBox {
            id: keySize

            //: Combo box label
            //% "Key size"
            label: qsTrId("foilpics-generate_key_view-label-key_size")
            enabled: !generating
            width: parent.width
            anchors {
                top: promptLabel.bottom
                topMargin: Theme.paddingLarge
            }
            menu: ContextMenu {
                MenuItem { text: "1024" }
                MenuItem { text: "1500" }
                MenuItem { text: "2048" }
            }
            Component.onCompleted: currentIndex = 2 // default
        }

        HarbourPasswordInputField {
            id: inputField

            anchors {
                left: parent.left
                top: keySize.bottom
                topMargin: Theme.paddingLarge
            }
            label: text.length < minPassphraseLen ?
                //: Password field label
                //% "Type at least %0 character(s)"
                qsTrId("foilpics-generate_key_view-label-minimum_length",minPassphraseLen).arg(minPassphraseLen) :
                placeholderText
            enabled: !generating
            EnterKey.onClicked: generateKey()
        }

        Button {
            id: button

            anchors.topMargin: Theme.paddingLarge
            text: generating ?
                //: Button label
                //% "Generating..."
                qsTrId("foilpics-generate_key_view-button-generating_key") :
                //: Button label
                //% "Generate key"
                qsTrId("foilpics-generate_key_view-button-generate_key")
            enabled: inputField.text.length >= minPassphraseLen && !generating
            onClicked: generateKey()
        }

        // Theme.paddingLarge pixels below the button in portrait
        Item {
            height: landscapeLayout ? 0 : Theme.paddingLarge
            anchors {
                top: button.bottom
                left: button.left
                right: button.right
            }
        }
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
                    anchors.rightMargin: 0
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
                        top: keySize.bottom
                        right: panel.right
                        horizontalCenter: undefined
                    }
                },
                PropertyChanges {
                    target: button
                    anchors.rightMargin: Theme.horizontalPageMargin
                }
            ]
        }
    ]
}
