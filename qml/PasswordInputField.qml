import QtQuick 2.0
import Sailfish.Silica 1.0

PasswordField {
    id: field
    width: parent.width - 2*x

    function requestFocus() {
        focusOutBehavior = FocusBehavior.KeepFocus
        forceActiveFocus()
        keepFocusTimer.start()
    }

    Timer {
        id: keepFocusTimer
        interval: 500
        onTriggered: field.focusOutBehavior = FocusBehavior.ClearItemFocus
    }

    Component.onCompleted: {
        if ("_usePasswordEchoMode" in field) {
            Qt.application.activeChanged.connect(onActiveChanged)
        }
    }

    Component.onDestruction: {
        Qt.application.activeChanged.disconnect(onActiveChanged)
    }

    function onActiveChanged() {
        if (!Qt.application.active) {
            field._usePasswordEchoMode = true
        }
    }
}
