import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Item {
    id: panel
    height: Math.max(deleteButton.height, encryptButton.height, dismissButton.height) + 2 * Theme.paddingMedium
    y: parent.height - visiblePart
    visible: visiblePart > 0

    property bool active
    property real visiblePart: active ? height : 0
    property alias canDelete: deleteButton.enabled
    property alias canEncrypt: encryptButton.enabled
    readonly property string imageProvider: HarbourTheme.darkOnLight ? HarbourImageProviderDarkOnLight : HarbourImageProviderDefault

    signal done()
    signal doneHint()
    signal deleteSelected()
    signal deleteHint()
    signal encryptSelected()
    signal encryptHint()

    anchors {
        left: parent.left
        right: parent.right
    }

    HarbourIconTextButton {
        id: deleteButton
        x: Math.floor(panel.width/6 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        iconSource: "image://theme/icon-m-delete"
        onClicked: panel.deleteSelected()
        onPressAndHold: panel.deleteHint()
    }

    HarbourIconTextButton {
        id: encryptButton
        x: Math.floor(panel.width/2 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        iconSource: "image://theme/icon-m-device-lock"
        onClicked: panel.encryptSelected()
        onPressAndHold: panel.encryptHint()
    }

    HarbourIconTextButton {
        id: dismissButton
        x: Math.floor(5*panel.width/6 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        iconSource: "image://" + imageProvider + "/" + Qt.resolvedUrl("images/close.svg")
        onClicked: panel.done()
        onPressAndHold: panel.doneHint()
    }

    Behavior on visiblePart { SmoothedAnimation { duration: 250  } }
}
