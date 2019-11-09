import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

Item {
    id: panel

    height: Math.max(deleteButton.height, groupButton.height, decryptButton.height) + 2 * Theme.paddingMedium
    y: parent.height - visiblePart
    visible: visiblePart > 0

    property bool active
    property real visiblePart: active ? height : 0
    readonly property string imageProvider: HarbourTheme.darkOnLight ? HarbourImageProviderDarkOnLight : HarbourImageProviderDefault

    signal deleteSelected()
    signal deleteHint()
    signal groupSelected()
    signal groupHint()
    signal decryptSelected()
    signal decryptHint()

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
        enabled: active
        iconSource: "image://theme/icon-m-delete"
        onClicked: panel.deleteSelected()
        onPressAndHold: panel.deleteHint()
    }

    HarbourIconTextButton {
        id: groupButton

        x: Math.floor(panel.width/2 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        enabled: active
        iconSource: "image://" + imageProvider + "/" + Qt.resolvedUrl("images/folder.svg")
        onClicked: panel.groupSelected()
        onPressAndHold: panel.groupHint()
    }

    HarbourIconTextButton {
        id: decryptButton

        x: Math.floor(5*panel.width/6 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        enabled: active
        iconSource: "image://theme/icon-m-image"
        onClicked: panel.decryptSelected()
        onPressAndHold: panel.decryptHint()
    }

    Behavior on visiblePart { SmoothedAnimation { duration: 250  } }
}
