import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: panel
    height: Math.max(deleteButton.height, groupButton.height, decryptButton.height, dismissButton.height) + 2 * Theme.paddingMedium
    y: parent.height - visiblePart
    visible: visiblePart > 0

    property bool active
    property real visiblePart: active ? height : 0
    property bool enableActions
    readonly property string imageProvider: HarbourTheme.darkOnLight ? HarbourImageProviderDarkOnLight : HarbourImageProviderDefault

    signal done()
    signal doneHint()
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

    IconTextButton {
        id: deleteButton
        x: Math.floor(panel.width/8 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        enabled: enableActions
        iconSource: "image://theme/icon-m-delete"
        onClicked: panel.deleteSelected()
        onPressAndHold: panel.deleteHint()
    }

    IconTextButton {
        id: groupButton
        x: Math.floor(3*panel.width/8 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        enabled: enableActions
        iconSource: "image://" + imageProvider + "/" + Qt.resolvedUrl("images/folder.svg")
        onClicked: panel.groupSelected()
        onPressAndHold: panel.groupHint()
    }

    IconTextButton {
        id: decryptButton
        x: Math.floor(5*panel.width/8 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        enabled: enableActions
        iconSource: "image://theme/icon-m-image"
        onClicked: panel.decryptSelected()
        onPressAndHold: panel.decryptHint()
    }

    IconTextButton {
        id: dismissButton
        x: Math.floor(7*panel.width/8 - width/2)
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
