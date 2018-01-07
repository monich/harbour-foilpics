import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: panel
    height: doneButton.height + 2 * Theme.paddingMedium
    y: parent.height - visiblePart
    visible: visiblePart > 0

    property bool active
    property real visiblePart: active ? height : 0

    signal done()

    anchors {
        left: parent.left
        right: parent.right
    }
    Button {
        id: doneButton
        anchors {
            verticalCenter: parent.verticalCenter
            horizontalCenter: parent.horizontalCenter
        }

        //: Button that exits selection mode
        //% "Done"
        text: qsTrId("foilpics-selection_panel-done_button")
        onClicked: panel.done()
    }
    Behavior on visiblePart { SmoothedAnimation { duration: 250  } }
}
