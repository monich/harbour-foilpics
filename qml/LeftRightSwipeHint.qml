import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: hint
    anchors.fill: parent

    property bool hintEnabled
    property bool hintRunning
    property bool swipeRight
    property alias text: label.text

    signal hintShown()

    function showHint() {
        hintRunning = true
        hintShownTimer.restart()
        touchInteractionHint.start()
    }

    onHintEnabledChanged: if (hint.hintEnabled) showHint();
    Component.onCompleted: if (hint.hintEnabled) showHint();

    InteractionHintLabel {
        id: label
        anchors.bottom: parent.bottom
        opacity: touchInteractionHint.running ? 1.0 : 0.0
        Behavior on opacity { FadeAnimation { duration: 1000 } }
    }

    TouchInteractionHint {
        id: touchInteractionHint
        direction: swipeRight ? TouchInteraction.Right : TouchInteraction.Left
        anchors.verticalCenter: parent.verticalCenter
        onRunningChanged: hintRunning = running
    }

    Timer {
        id: hintShownTimer
        interval: 1000
        onTriggered: hint.hintShown()
    }
}
