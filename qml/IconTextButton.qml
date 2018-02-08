import QtQuick 2.0
import Sailfish.Silica 1.0

MouseArea {
    property alias icon: image
    property bool down: pressed && containsMouse
    property bool highlighted: down
    property alias iconSource: image.imageSource
    property bool _showPress: highlighted || pressTimer.running
    property alias text: label.text

    onPressedChanged: {
        if (pressed) {
            pressTimer.start()
        }
    }

    onCanceled: pressTimer.stop()

    width: Math.max(image.width, label.width)
    height: image.height + label.height

    Image {
        id: image
        readonly property color highlightColor: Theme.highlightColor
        property url imageSource
        property string highlightSource: {
            if (source != "") {
                var tmpSource = image.source.toString()
                var index = tmpSource.lastIndexOf("?")
                if (index !== -1) {
                    tmpSource = tmpSource.substring(0, index)
                }
                return tmpSource + "?" + highlightColor
            } else {
                return ""
            }
        }
        sourceSize.width: Theme.itemSizeSmall
        source: highlighted ? highlightSource : imageSource
        opacity: parent.enabled ? 1.0 : 0.4
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
        }
    }

    Label {
        id: label
        height: text.length ? (implicitHeight + Theme.paddingMedium) : 0
        opacity: parent.enabled ? 1.0 : 0.4
        anchors {
            top: image.bottom
            horizontalCenter: parent.horizontalCenter
        }
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSizeExtraSmall
        color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
    }

    Timer {
        id: pressTimer
        interval: ('minimumPressHighlightTime' in Theme) ? Theme.minimumPressHighlightTime : 64
    }
}
