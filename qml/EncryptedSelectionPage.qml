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

    onStatusChanged: {
        if (status === PageStatus.Active) {
            listView.maximizeCacheBuffer()
        }
    }

    SilicaFlickable {
        contentHeight: height
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                id: selectNoneMenuItem

                //: Pulley menu item
                //% "Select none"
                text: qsTrId("foilpics-pulley_menu-select_none")
                visible: selectionModel.count > 0
                onClicked: selectionModel.clearSelection()
            }
            MenuItem {
                id: selectAllMenuItem

                //: Pulley menu item
                //% "Select all"
                text: qsTrId("foilpics-pulley_menu-select_all")
                visible: selectionModel.count < foilModel.count
                onClicked: selectionModel.selectAll()
            }
            onActiveChanged: {
                if (!active) {
                    selectNoneMenuItem.visible = selectNoneMenuItem.enabled
                    selectAllMenuItem.visible = selectAllMenuItem.enabled
                }
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
        }

        EncryptedSelectionPanel {
            id: selectionPanel

            active: selectionModel.count > 0
            //: Generic menu item
            //% "Delete"
            onDeleteHint: showHint(qsTrId("foilpics-menu-delete"))
            onDeleteSelected: page.deletePictures(selectionModel.makeSelectionBusy())
            //: Generic menu item
            //% "Group"
            onGroupHint: showHint(qsTrId("foilpics-menu-group"))
            onGroupSelected: page.groupPictures(selectionModel.makeSelectionBusy())
            //: Generic menu item
            //% "Decrypt"
            onDecryptHint: showHint(qsTrId("foilpics-menu-decrypt"))
            onDecryptSelected: page.decryptPictures(selectionModel.makeSelectionBusy())

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
}
