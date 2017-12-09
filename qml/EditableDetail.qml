import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: item
    width: parent.width
    height: Math.max(labelText.height, valueField.height) + 2*Theme.paddingSmall

    property alias label: labelText.text
    property alias value: valueField.text
    property alias defaultValue: valueField.placeholderText
    property real leftMargin: Theme.horizontalPageMargin
    property real rightMargin: Theme.horizontalPageMargin

    signal apply()

    Text {
        id: labelText
        anchors {
            left: parent.left
            right: parent.horizontalCenter
            rightMargin: Theme.paddingSmall
            leftMargin: item.leftMargin
            // It would be nice to to align at baseline...
            verticalCenter: valueField.top
            verticalCenterOffset: valueField.textVerticalCenterOffset
        }
        horizontalAlignment: Text.AlignRight
        color: Theme.secondaryHighlightColor
        font.pixelSize: Theme.fontSizeSmall
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
    }

    TextField {
        id: valueField
        y: Theme.paddingSmall
        anchors {
            left: parent.horizontalCenter
            right: parent.right
        }
        horizontalAlignment: Text.AlignLeft
        font.pixelSize: Theme.fontSizeSmall
        labelVisible: false
        textLeftMargin: Theme.paddingSmall
        textRightMargin: item.rightMargin
        EnterKey.onClicked: {
            if (text.length < 1) text = defaultValue
            item.focus = true
            item.apply()
        }
        EnterKey.iconSource: "image://theme/icon-m-enter-close"
    }
}
