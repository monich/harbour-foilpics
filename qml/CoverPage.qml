import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

CoverBackground {
    id: cover
    property var foilModel

    Rectangle {
        id: backgroundCircle
        anchors.centerIn: parent
        width: Math.floor(parent.width * 0.8)
        height: width
        color: "white"
        radius: width/2
        opacity: 0.2
    }

    Image {
        id: unlockedImage
        visible: width > 0
        source: "images/gallery.svg"
        width: fullWidth
        height: fullHeight
        sourceSize.width: width
        anchors.centerIn: parent
        readonly property real fullWidth: Math.floor(cover.width/2)
        readonly property real fullHeight: Math.floor(fullWidth*implicitHeight/implicitWidth)
    }

    Image {
        id: lockedImage
        visible: width > 0
        source: "images/lock.svg"
        height: unlockedImage.fullHeight
        sourceSize.height: height
        anchors.centerIn: parent
        readonly property real fullWidth: Math.floor(unlockedImage.fullHeight*implicitWidth/implicitHeight)
    }

    Label {
        width: backgroundCircle.width
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors {
            top: parent.top
            bottom: backgroundCircle.top
            horizontalCenter: parent.horizontalCenter
        }
        //: Application title
        //% "Foil Pics"
        text: qsTrId("foilpics-app_name")
    }

    CoverActionList {
        enabled: cover.foilModel.keyAvailable
        CoverAction {
            iconSource: Qt.resolvedUrl("images/" + (HarbourTheme.darkOnLight ? "cover-lock-dark.svg" :  "cover-lock.svg"))
            onIconSourceChanged: console.log(iconSource)
            onTriggered: cover.foilModel.lock(false)
        }
    }

    Component.onCompleted: {
        if (cover.foilModel.keyAvailable) {
            lockedImage.width = 0
        } else {
            unlockedImage.width = 0
        }
    }

    Connections {
        target: cover.foilModel
        onKeyAvailableChanged: {
            if (cover.foilModel.keyAvailable) {
                // This transition is not visible, there's no reason to animate it
                lockedImage.width = 0
                unlockedImage.width = unlockedImage.fullWidth
            } else {
                lockAnimation.start()
            }
        }
    }

    // Flip animation
    SequentialAnimation  {
        id: lockAnimation
        alwaysRunToEnd: true
        PropertyAction {
            target: lockedImage
            property: "width"
            value: 0
        }
        PropertyAction {
            target: unlockedImage
            property: "width"
            value: unlockedImage.fullWidth
        }
        NumberAnimation {
            target: unlockedImage
            property: "width"
            to: 0
            duration: 125
            easing.type: Easing.InQuad
        }
        NumberAnimation {
            target: lockedImage
            property: "width"
            to: lockedImage.fullWidth
            duration: 125
            easing.type: Easing.OutQuad
        }
    }
}
