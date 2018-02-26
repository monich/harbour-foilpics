import QtQuick 2.0
import Sailfish.Silica 1.0

TextField {
    id: field

    property bool showEchoModeToggle: activeFocus
    property int passwordEchoMode: TextInput.Password

    property bool _usePasswordEchoMode: true
    property int _buttonLeftMargin: Theme.paddingLarge

    width: (parent ? parent.width : Screen.width) - 2*x
    textRightMargin: textMargin
    echoMode: _usePasswordEchoMode ? passwordEchoMode : TextInput.Normal
    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase

    //: Default placeholder for password field
    //% "Password"
    placeholderText: qsTrId("foilpics-password_field-placeholder")

    //: Default label for password field
    //% "Password"
    label: qsTrId("foilpics-password_field-label")

    function requestFocus() {
        focusOutBehavior = FocusBehavior.KeepFocus
        forceActiveFocus()
        keepFocusTimer.start()
    }

    Connections {
        target: Qt.application
        onActiveChanged: {
            if (!Qt.application.active) {
                _usePasswordEchoMode = true
            }
        }
    }

    Timer {
        id: keepFocusTimer
        interval: 500
        onTriggered: field.focusOutBehavior = FocusBehavior.ClearItemFocus
    }

    states: State {
        name: "showToggle"
        when: field.showEchoModeToggle
        PropertyChanges {
            target: field
            textRightMargin: field._buttonLeftMargin/2 + passwordVisibilityButton.width + field.textMargin
        }
        PropertyChanges {
            target: passwordVisibilityButton
            opacity: 1
            enabled: true
        }
    }

    transitions: Transition {
        from: ""; to: "showToggle"
        reversible: true
        NumberAnimation {
            property: "textRightMargin"
            duration: 50
        }
        FadeAnimation { target: passwordVisibilityButton }
    }

    MouseArea {
        id: passwordVisibilityButton
        parent: field    // ensure the field is visible, avoid auto-parenting to TextBase contentItem
        x: field.width - width - field.textMargin
        width: Math.max(textAbc.implicitWidth, textDots.implicitWidth) + field._buttonLeftMargin/2
        height: field.height - Theme.paddingLarge
        opacity: 0
        enabled: false

        onClicked: {
            field._usePasswordEchoMode = !field._usePasswordEchoMode
        }

        Text {
            id: textAbc
            anchors {
                top: parent.top
                topMargin: field.textTopMargin
                horizontalCenter: parent.horizontalCenter
            }
            visible: field.echoMode == TextInput.Password
            font.pixelSize: Theme.fontSizeMedium
            text: "abc"
            textFormat: Text.PlainText
            color: parent.pressed && parent.containsMouse ? Theme.highlightColor : Theme.primaryColor
        }

        Text {
            id: textDots
            anchors {
                top: parent.top
                topMargin: field.textTopMargin
                horizontalCenter: parent.horizontalCenter
            }
            visible: field.echoMode == TextInput.Normal
            font.pixelSize: Theme.fontSizeMedium
            text: "\u2022\u2022\u2022"
            textFormat: Text.PlainText
            color: parent.pressed && parent.containsMouse ? Theme.highlightColor : Theme.primaryColor
        }
    }
}
