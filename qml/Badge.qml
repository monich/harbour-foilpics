import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: badge
    property alias text: label.text
    readonly property real radius: height/2
    property real maxWidth

    width: maxWidth > 0 ? Math.max(label.paintedWidth + radius, height) : height
    visible: opacity > 0
    Behavior on opacity { FadeAnimation {} }

    Rectangle {
        id: background
        anchors.fill: parent
        color: Theme.rgba(Theme.primaryColor, 0.2)
        radius: badge.radius
    }

    FitLabel {
        id: label
        font.bold: true
        width: Math.max((maxWidth > 0 ? maxWidth : parent.width) - radius, height)
        height: Math.floor(parent.height - radius/2)
        anchors.centerIn: background
    }
}
