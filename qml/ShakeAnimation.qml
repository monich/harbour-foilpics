import QtQuick 2.0
import Sailfish.Silica 1.0

SequentialAnimation  {
    id: animation
    alwaysRunToEnd: true

    property var target
    property real amplitude: Theme.horizontalPageMargin * 4 / 5
    property real x: 0

    NumberAnimation {
        target: animation.target
        property: "x"
        to: animation.x + animation.amplitude
        duration: 25
        easing.type: Easing.InOutSine
    }

    NumberAnimation {
        target: animation.target
        property: "x"
        from: animation.x + animation.amplitude
        to: animation.x
        duration: 500
        easing.type: Easing.OutElastic
        easing.amplitude:  2
        easing.period: 0.2
    }
}
