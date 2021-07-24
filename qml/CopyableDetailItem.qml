import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem {
    contentHeight: detailItem.height
    height: contentHeight + (copyMenu ? copyMenu.height : 0)
    width: parent.width

    property alias label: detailItem.label
    property alias value: detailItem.value

    menu: ContextMenu {
        id: copyMenu

        MenuItem {
            //: Context menu item
            //% "Copy to clipboard"
            text: qsTrId("foilpics-menu-copy")
            onClicked: Clipboard.text = detailItem.value
        }
    }

    DetailItem {
        id: detailItem
    }
}
