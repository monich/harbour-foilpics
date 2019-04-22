import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

Dialog {
    id: dialog
    forwardNavigation: false
    allowedOrientations: window.allowedOrientations
    property var foilModel
    property bool wrongPassword
    property alias currentPassword: currentPasswordInput.text
    property alias newPassword: newPasswordInput.text

    function canChangePassword() {
        return currentPassword.length > 0 && newPassword.length > 0 && currentPassword !== newPassword && !wrongPassword
    }

    function invalidPassword() {
        wrongPassword = true
        wrongPasswordAnimation.start()
        currentPasswordInput.requestFocus()
    }

    function changePassword() {
        if (canChangePassword()) {
            if (foilModel.checkPassword(currentPassword)) {
                pageStack.push(Qt.resolvedUrl("ConfirmPasswordDialog.qml"), {
                    password: newPassword
                }).passwordConfirmed.connect(function() {
                    if (foilModel.changePassword(currentPassword, newPassword)) {
                        pageStack.pop(pageStack.previousPage(dialog))
                    } else {
                        invalidPassword()
                    }
                })
            } else {
                invalidPassword()
            }
        }
    }

    onStatusChanged: {
        if (status === DialogStatus.Opening) {
            currentPasswordInput.requestFocus()
        }
    }

    Connections {
        target: foilModel
        onFoilStateChanged: {
            // Pop the page when the model gets locked
            if (foilModel.foilState !== FoilPicsModel.FoilPicsReady) {
                pageStack.pop(pageStack.previousPage(dialog))
            }
        }
    }

    Column {
        id: column
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.paddingLarge

        InfoLabel {
            //: Password change prompt
            //% "Please enter the current and the new password"
            text: qsTrId("foilpics-change_password_page-label-enter_passwords")
        }
        HarbourPasswordInputField {
            id: currentPasswordInput
            //: Placeholder and label for the current password prompt
            //% "Current password"
            label: qsTrId("foilpics-change_password_page-text_field_label-current_password")
            placeholderText: label
            EnterKey.onClicked: newPasswordInput.focus = true
            onTextChanged: dialog.wrongPassword = false
        }
        HarbourPasswordInputField {
            id: newPasswordInput
            //: Placeholder and label for the new password prompt
            //% "New password"
            placeholderText: qsTrId("foilpics-change_password_page-text_field_label-new_password")
            label: placeholderText
            EnterKey.onClicked: dialog.changePassword()
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            //: Button label
            //% "Change password"
            text: qsTrId("foilpics-change_password_page-button-change_password")
            enabled: canChangePassword()
            onClicked: dialog.changePassword()
        }
    }

    HarbourShakeAnimation  {
        id: wrongPasswordAnimation
        target: column
    }
}
