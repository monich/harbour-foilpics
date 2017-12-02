import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

Item {
    id: view
    property var foilModel
    property alias title: title.text
    readonly property int minPassphraseLen: 8
    readonly property bool generating: foilModel.foilState === FoilPicsModel.FoilGeneratingKey

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

    Column {
        id: column
        spacing: Theme.paddingLarge
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            right: parent.right
        }

        InfoLabel {
            id: title
            //: Label text
            //% "You need to generate the key and select the password before you can encrypt your pictures"
            text: qsTrId("foilpics-generate_key_view-label-key_needed")
        }

        ComboBox {
            id: keySize
            //: Combo box label
            //% "Key size"
            label: qsTrId("foilpics-generate_key_view-label-key_size")
            enabled: !generating
            menu: ContextMenu {
                MenuItem { text: "1024" }
                MenuItem { text: "1500" }
                MenuItem { text: "2048" }
            }
            Component.onCompleted: currentIndex = 1 // default
        }

        PasswordInputField {
            id: passphrase
            label: text.length < minPassphraseLen ?
                //: Password field label
                //% "Type at least %0 character(s)"
                qsTrId("foilpics-generate_key_view-label-minimum_length",minPassphraseLen).arg(minPassphraseLen) :
                placeholderText
            enabled: !generating
            EnterKey.onClicked: generateKey()
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: generating ?
                //: Button label
                //% "Generating..."
                qsTrId("foilpics-generate_key_view-button-generating_key") :
                //: Button label
                //% "Generate key"
                qsTrId("foilpics-generate_key_view-button-generate_key")
            enabled: passphrase.text.length >= minPassphraseLen && !generating
            onClicked: generateKey()
        }
    }
}
