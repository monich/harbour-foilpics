import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    Label {
        width: parent.width - 2*Theme.horizontalPageMargin
        height: implicitHeight
        anchors {
            bottom: busyIndicator.top
            bottomMargin: Theme.paddingLarge
            horizontalCenter: parent.horizontalCenter
        }
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.highlightColor
        //: Progress view label
        //% "Generating new key..."
        text: qsTrId("foilpics-generating_key_view-generating_new_key")
    }

    BusyIndicator {
        id: busyIndicator
        y: Math.floor(((appLandscapeMode ? Screen.width : Screen.height) - height) /2)
        anchors.horizontalCenter: parent.horizontalCenter
        size: BusyIndicatorSize.Large
        running: true
    }
}
