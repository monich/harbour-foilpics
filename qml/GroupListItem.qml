import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    id: root
    property bool defaultGroup
    property string groupName
    property int count
    property bool selected
    property bool highlighted
    property bool editing
    property alias editText: editor.text
    color: "transparent"

    signal doneEditing()

    function startEditing() {
        editor.forceActiveFocus()
    }

    Badge {
        id: badge
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        maxWidth: parent.width/2
        text: count ? count : ""
    }

    Label {
        visible: !editing
        anchors {
            left: parent.left
            right: parent.right
            rightMargin: Theme.horizontalPageMargin + (badge.visible ? (badge.width + Theme.paddingMedium) : 0)
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        truncationMode: TruncationMode.Fade
        text: defaultGroup ?
            //: Name of the default group
            //% "Default"
            qsTrId("foilpics-default_group") :
            groupName
        color: selected ? Theme.highlightColor : (highlighted ? Theme.highlightColor : Theme.primaryColor)
    }

    TextField {
        id: editor
        y: Theme.paddingSmall
        visible: editing
        anchors {
            left: parent.left
            right: parent.right
            rightMargin: Theme.horizontalPageMargin + (badge.visible ? (badge.width + Theme.paddingMedium) : 0)
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        textLeftMargin: 0
        textRightMargin: 0
        horizontalAlignment: Text.AlignLeft
        labelVisible: false
        onActiveFocusChanged: if (!activeFocus) root.doneEditing()
        EnterKey.onClicked: root.focus = true
        EnterKey.iconSource: "image://theme/icon-m-enter-close"
    }
}
