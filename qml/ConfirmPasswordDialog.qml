import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dialog
    allowedOrientations: window.allowedOrientations
    forwardNavigation: false
    property string password
    property bool wrongPassword

    signal passwordConfirmed()

    function checkPassword() {
        if (passwordInput.text === password) {
            dialog.passwordConfirmed()
        } else {
            wrongPassword = true
            wrongPasswordAnimation.start()
            passwordInput.requestFocus()
        }
    }

    function canCheckPassword() {
        return passwordInput.text.length > 0 && passwordInput.text.length > 0 && !wrongPassword
    }

    Column {
        id: column
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.paddingLarge

        InfoLabel {
            //: Password confirmation label
            //% "Please type in your new password one more time"
            text: qsTrId("foilpics-confirm_password_page-info_label")
        }
        Label {
            //: Password confirmation description
            //% "Make sure you don't forget your password. It's impossible to either recover it or to access the encrypted pictures without knowing it. Better take it seriously."
            text: qsTrId("foilpics-confirm_password_page-description")
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
        }
        PasswordInputField {
            id: passwordInput
            //: Placeholder for the password confirmation prompt
            //% "New password again"
            placeholderText: qsTrId("foilpics-confirm_password_page-text_field_placeholder-new_password")
            //: Label for the password confirmation prompt
            //% "New password"
            label: qsTrId("foilpics-confirm_password_page-text_field_label-new_password")
            onTextChanged: dialog.wrongPassword = false
            EnterKey.onClicked: dialog.checkPassword()
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            //: Button label (confirm password)
            //% "Confirm"
            text: qsTrId("foilpics-confirm_password_page-button-confirm")
            enabled: dialog.canCheckPassword()
            onClicked: dialog.checkPassword()
        }
    }

    ShakeAnimation  {
        id: wrongPasswordAnimation
        target: column
    }
}
