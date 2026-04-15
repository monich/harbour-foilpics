import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

Item {
    property var foilModel
    property int shownCount: 0
    property bool isLandscape
    readonly property bool unlocking: !foilModel || !foilModel.count || unlockingTimer.running
    readonly property int realCount: foilModel ? foilModel.count : 0

    onRealCountChanged: syncCounts()
    onUnlockingChanged: syncCounts()

    function syncCounts() {
        if (!unlocking && realCount > shownCount) {
            shownCount++;
            if (realCount > shownCount) {
                // They still don't match (could be at the beginning)
                mismatchTimer.start()
            }
        }
    }

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
        text: unlocking ?
            //: Progress view label
            //% "Unlocking..."
            qsTrId("foilpics-decrypting_view-unlocking") :
            //: Progress view label
            //% "Decrypting..."
            qsTrId("foilpics-decrypting_view-decrypting")
    }

    Label {
        anchors {
            fill: busyIndicator
            margins: Theme.paddingMedium
        }
        text: (shownCount > 0) ? shownCount : ""
        opacity: (shownCount > 0 && foilModel && foilModel.foilState === FoilPicsModel.FoilDecrypting && !unlocking) ? 1 : 0
        visible: opacity > 0
        color: Theme.highlightColor
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: Theme.fontSizeTiny

        Behavior on opacity { FadeAnimation { } }
    }

    BusyIndicator {
        id: busyIndicator
        y: Math.floor(((isLandscape ? Screen.width : Screen.height) - height) /2)
        anchors.horizontalCenter: parent.horizontalCenter
        size: BusyIndicatorSize.Large
        running: true
    }

    Timer {
        id: unlockingTimer
        interval: 500
        running: true
    }

    Timer {
        id: mismatchTimer
        interval: 0
        onTriggered: {
            if (shownCount < realCount) {
                shownCount++
                if (shownCount < realCount) {
                    start();
                }
            }
        }
    }
}
