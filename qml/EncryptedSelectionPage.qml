import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

Page {
    id: page

    property alias foilModel: listView.foilModel
    property alias selectionModel: listView.selectionModel

    signal deletePictures(var list)
    signal groupPictures(var list)
    signal decryptPictures(var list)

    readonly property int selectionCount: selectionModel ? selectionModel.count : 0

    onStatusChanged: {
        // Maximize cache buffer once slide-in animation has completed
        if (status === PageStatus.Active) {
            listView.maximizeCacheBuffer()
        }
    }


    EncryptedList {
        id: listView

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: selectionPanel.top
        }
        //: Encrypted grid title in selection mode
        //% "Select photos"
        title: qsTrId("foilpics-encrypted_grid-selection_title")
        model: foilModel.groupModel
        selectable: true
        clip: true

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                id: selectNoneMenuItem

                //: Pulley menu item
                //% "Select none"
                text: qsTrId("foilpics-pulley_menu-select_none")
                enabled: selectionCount > 0
                onEnabledChanged: if (!pullDownMenu.active) visible = enabled
                onClicked: selectionModel.clearSelection()
            }
            MenuItem {
                id: selectAllMenuItem

                //: Pulley menu item
                //% "Select all"
                text: qsTrId("foilpics-pulley_menu-select_all")
                enabled: selectionCount < foilModel.count
                onEnabledChanged: if (!pullDownMenu.active) visible = enabled
                onClicked: selectionModel.selectAll()
            }

            Component.onCompleted: updateMenuItems()
            onActiveChanged: updateMenuItems()

            function updateMenuItems() {
                if (!active) {
                    selectNoneMenuItem.visible = selectNoneMenuItem.enabled
                    selectAllMenuItem.visible = selectAllMenuItem.enabled
                }
            }
        }
    }

    EncryptedSelectionPanel {
        id: selectionPanel

        active: selectionCount > 0
        //: Generic menu item
        //% "Delete"
        onDeleteHint: showHint(qsTrId("foilpics-menu-delete"))
        onDeleteSelected: page.deletePictures(selectionModel.getSelectedRows())
        //: Generic menu item
        //% "Group"
        onGroupHint: showHint(qsTrId("foilpics-menu-group"))
        onGroupSelected: page.groupPictures(selectionModel.getSelectedRows())
        //: Generic menu item
        //% "Decrypt"
        onDecryptHint: showHint(qsTrId("foilpics-menu-decrypt"))
        onDecryptSelected: page.decryptPictures(selectionModel.getSelectedRows())

        function showHint(text) {
            selectionHint.text = text
            selectionHintTimer.restart()
        }
    }

    InteractionHintLabel {
        id: selectionHint

        invert: true
        anchors.fill: listView
        visible: opacity > 0
        opacity: selectionHintTimer.running ? 1.0 : 0.0
        Behavior on opacity { FadeAnimation { duration: 1000 } }
    }

    Timer {
        id: selectionHintTimer
        interval: 1000
    }
}
