import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

Page {
    id: page

    property var foilModel
    property var dataModel
    property var selectionModel

    signal deletePictures(var list)
    signal encryptPictures(var list)

    SilicaFlickable {
        contentHeight: height
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                id: selectNoneMenuItem

                //: Pulley menu item
                //% "Select none"
                text: qsTrId("foilpics-pulley_menu-select_none")
                enabled: selectionModel.count > 0
                onClicked: selectionModel.clearSelection()
            }
            MenuItem {
                id: selectAllMenuItem

                //: Pulley menu item
                //% "Select all"
                text: qsTrId("foilpics-pulley_menu-select_all")
                enabled: selectionModel.count < dataModel.count
                onClicked: selectionModel.selectAll()
            }
            onActiveChanged: {
                if (!active) {
                    selectNoneMenuItem.visible = selectNoneMenuItem.enabled
                    selectAllMenuItem.visible = selectAllMenuItem.enabled
                }
            }
        }

        ImageGridView {
            id: grid

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: selectionPanel.top
            }
            model: dataModel
            clip: true

            header: PageHeader {
                id: header

                //: Gallery grid title in selection mode
                //% "Select photos"
                title: qsTrId("foilpics-gallery_grid-selection_title")

                HarbourBadge {
                    anchors {
                        left: header.extraContent.left
                        verticalCenter: header.extraContent.verticalCenter
                    }
                    maxWidth: header.extraContent.width
                    text: grid.count ? grid.count : ""
                }
            }

            delegate: ThumbnailImage {
                id: delegate

                source: url
                size: grid.cellSize
                height: grid.cellSize
                selectionModel: page.selectionModel
                selectionKey: url

                onClicked: selectionModel.toggleSelection(selectionKey)

                GridView.onAdd: AddAnimation { target: delegate }
                GridView.onRemove: SequentialAnimation {
                    PropertyAction { target: delegate; property: "GridView.delayRemove"; value: true }
                    NumberAnimation { target: delegate; properties: "opacity,scale"; to: 0; duration: 250; easing.type: Easing.InOutQuad }
                    PropertyAction { target: delegate; property: "GridView.delayRemove"; value: false }
                }
            }
        }

        GallerySelectionPanel {
            id: selectionPanel

            active: selectionModel.count > 0
            canDelete: active
            canEncrypt: foilModel.keyAvailable
            //: Generic menu item
            //% "Delete"
            onDeleteHint: showHint(qsTrId("foilpics-menu-delete"))
            onDeleteSelected: page.deletePictures(selectionModel.makeSelectionBusy())
            //: Generic menu item
            //% "Encrypt"
            onEncryptHint: showHint(qsTrId("foilpics-menu-encrypt"))
            onEncryptSelected: page.encryptPictures(selectionModel.makeSelectionBusy())

            function showHint(text) {
                selectionHint.text = text
                selectionHintTimer.restart()
            }
        }

        InteractionHintLabel {
            id: selectionHint

            invert: true
            anchors.fill: grid
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
