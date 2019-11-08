import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Item {
    id: panel

    height: Math.max(deleteButton.height, encryptButton.height) + 2 * Theme.paddingMedium
    y: parent.height - visiblePart
    visible: visiblePart > 0

    property bool active
    property real visiblePart: active ? height : 0
    property alias canDelete: deleteButton.enabled
    property alias canEncrypt: encryptButton.enabled

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

        x: Math.floor(panel.width/4 - width/2)
        anchors.verticalCenter: parent.verticalCenter
        iconSource: "image://theme/icon-m-delete"
        onClicked: panel.deleteSelected()
        onPressAndHold: panel.deleteHint()
    }

    HarbourIconTextButton {
        id: encryptButton

        x: Math.floor(3*panel.width/4 - width/2)
        anchors.verticalCenter: parent.verticalCenter
        iconSource: "image://theme/icon-m-device-lock"
        onClicked: panel.encryptSelected()
        onPressAndHold: panel.encryptHint()
    }

    Behavior on visiblePart { SmoothedAnimation { duration: 250  } }
}
