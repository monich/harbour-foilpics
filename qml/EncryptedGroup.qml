import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

Item {
    id: group
    height: grid.y + grid.height
    readonly property real headerHeight: grid.y
    readonly property bool headerVisible: (!isDefault || !isFirstGroup) && picsCount > 0

    property var foilModel
    property var flickable
    property alias title: groupHeader.text
    property alias picsModel: grid.model
    property alias cellSize: grid.cellSize
    property alias columnCount: grid.columnCount
    property alias currentIndex: grid.currentIndex
    property alias currentItem: grid.currentItem
    property int picsCount
    property bool isDefault
    property bool isFirstGroup
    property int modelIndex

    property var selectionModel
    property bool selecting

    signal decryptItem(int globalIndex)
    signal deleteItem(int globalIndex)
    signal requestIndex(int globalIndex)

    SectionHeader {
        id: groupHeader
        visible: headerVisible
    }

    ImageGridView {
        id: grid
        y: headerVisible ? groupHeader.height : 0
        width: parent.width
        height: contentHeight + expandHeight
        contentHeight: grid.cellWidth * Math.floor((modelCount + columnCount - 1)/columnCount)

        readonly property int modelCount: model ? model.count : 0
        property alias contextMenu: contextMenuItem
        property Item expandItem
        property real expandHeight: contextMenu.height
        readonly property int minOffsetIndex: expandItem != null ?
            (expandItem.modelIndex + columnCount - (expandItem.modelIndex % columnCount)) : 0

        delegate: ThumbnailBase {
            id: delegate
            width: grid.cellWidth
            height: isItemExpanded ? grid.contextMenu.height + grid.cellHeight : grid.cellHeight
            contentYOffset: index >= grid.minOffsetIndex ? grid.expandHeight : 0.0
            z: isItemExpanded ? 1000 : 1
            enabled: isItemExpanded || !grid.contextMenu.active
            selectionModel: group.selectionModel
            selectionKey: imageId

            readonly property bool isItemExpanded: grid.expandItem === delegate
            readonly property int modelIndex: index

            Image {
                y: contentYOffset
                x: contentXOffset
                asynchronous: true
                cache: false
                fillMode: Image.PreserveAspectCrop
                source: thumbnail
                width:  grid.cellWidth
                height: grid.cellHeight
                clip: true
            }

            function decrypt() {
                group.decryptItem(picsModel.mapToSource(modelIndex))
            }

            function remove() {
                group.deleteItem(picsModel.mapToSource(modelIndex))
            }

            onClicked: {
                if (selecting) {
                    selectionModel.toggleSelection(selectionKey)
                } else if (!grid.contextMenu.active) {
                    var sourceIndex = picsModel.mapToSource(modelIndex)
                    var page = pageStack.push(Qt.resolvedUrl("EncryptedFullscreenPage.qml"),{
                        currentIndex: sourceIndex,
                        model: foilModel
                    })
                    if (page) {
                        group.requestIndex(sourceIndex)
                        page.decryptItem.connect(group.decryptItem)
                        page.deleteItem.connect(group.deleteItem)
                        page.requestIndex.connect(group.requestIndex)
                    }
                }
            }

            onPressAndHold: {
                if (!selecting) {
                    grid.expandItem = delegate
                    contextMenuItem.openMenu(delegate)
                }
            }

            GridView.onAdd: AddAnimation { target: delegate }
            GridView.onRemove: SequentialAnimation {
                PropertyAction { target: delegate; property: "GridView.delayRemove"; value: true }
                NumberAnimation { target: delegate; properties: "opacity,scale"; to: 0; duration: 250; easing.type: Easing.InOutQuad }
                PropertyAction { target: delegate; property: "GridView.delayRemove"; value: false }
            }
        }

        ContextMenu {
            id: contextMenuItem
            x: parent !== null ? -parent.x : 0.0

            function openMenu(item) {
                // ContextMenu::show is deprecated in Sailfish Silica package 0.25.6 (Dec 2017)
                if ("open" in contextMenuItem) {
                    open(item)
                } else {
                    show(item)
                }
            }

            // ContextMenu positioning is slightly broken because it takes
            // ImageGridView as a flickable but doesn't consider that it's
            // inside of the other flickable (i.e. ListView). So we need
            // compensate for that by watching the height and making sure
            // that menu is visible
            onHeightChanged: {
                if (parent) {
                    var bottom = parent.mapToItem(flickable, x, y).y + height
                    if (bottom > flickable.height) {
                        flickable.contentY += bottom - flickable.height
                    }
                }
            }

            MenuItem {
                //: Generic menu item
                //% "Decrypt"
                text: qsTrId("foilpics-menu-decrypt")
                onClicked: grid.expandItem.decrypt()
            }
            MenuItem {
                //: Generic menu item
                //% "Delete"
                text: qsTrId("foilpics-menu-delete")
                onClicked: grid.expandItem.remove()
            }
        }
    }
}
