import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

Item {
    id: thisItem

    height: grid.y + grid.height

    readonly property real headerHeight: grid.y
    readonly property bool headerVisible: (!isDefault || !isFirstGroup) && picsCount > 0

    property var foilModel
    property var flickable
    property alias title: groupHeaderLabel.text
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
    property bool selectable
    property bool expanded: true

    property bool _constructed

    signal decryptItem(int globalIndex)
    signal deleteItem(int globalIndex)
    signal requestIndex(int globalIndex)
    signal toggleExpanded()

    Component.onCompleted: _constructed = true

    BackgroundItem {
        id: groupHeader

        height: Theme.itemSizeSmall
        visible: headerVisible
        enabled: isInteractive

        readonly property bool isInteractive: !selectable && !isDefault

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.15) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        HarbourBadge {
            id: badge

            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            text: picsModel.count ? picsModel.count : ""
            opacity: (picsModel.count > 0 && expanded) ? 1 : 0
            backgroundColor: Theme.rgba(groupHeaderLabel.color, 0.2)
            textColor: groupHeaderLabel.color
        }

        SectionHeader {
            id: groupHeaderLabel

            color: (groupHeader.highlighted || selectable)? Theme.highlightColor : Theme.primaryColor
            anchors {
                left: badge.visible ? badge.right : parent.left
                leftMargin: badge.visible ? Theme.paddingLarge : Theme.horizontalPageMargin
                right: groupHeader.isInteractive ? arrow.left : parent.right
                rightMargin: groupHeader.isInteractive ? Theme.paddingMedium : Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }

            // Label/SectionHeader didn't have "topPadding" in earlier versions of SFOS
            Component.onCompleted: {
                if ('topPadding' in groupHeaderLabel) {
                    groupHeaderLabel.topPadding = 0
                }
            }
        }

        HarbourHighlightIcon {
            id: arrow

            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
                rightMargin: Theme.paddingMedium
            }
            highlightColor: groupHeaderLabel.color
            source: groupHeader.isInteractive ? "image://theme/icon-m-left" : ""
            visible: groupHeader.isInteractive
        }

        onClicked: thisItem.toggleExpanded()
    }

    ImageGridView {
        id: grid

        y: headerVisible ? groupHeader.height : 0
        width: parent.width
        contentHeight: grid.cellWidth * Math.floor((modelCount + columnCount - 1)/columnCount)
        clip: true

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
            selectionModel: thisItem.selectionModel
            selectionKey: imageId

            readonly property bool isItemExpanded: grid.expandItem === delegate
            readonly property int modelIndex: index

            Image {
                y: contentYOffset
                x: contentXOffset
                cache: false
                fillMode: Image.PreserveAspectCrop
                source: thumbnail
                width:  grid.cellWidth
                height: grid.cellHeight
                clip: true
            }

            function decrypt() {
                thisItem.decryptItem(picsModel.mapToSource(modelIndex))
            }

            function remove() {
                thisItem.deleteItem(picsModel.mapToSource(modelIndex))
            }

            function editDetails() {
                var model = grid.model
                var modelIndex = index
                var imageData = picsModel.get(modelIndex)
                var page = pageStack.push(Qt.resolvedUrl("EncryptedDetailsPage.qml"), {
                    details: imageData,
                    foilModel: thisItem.foilModel
                })
                page.titleChanged.connect(function(title) {
                    model.setTitleAt(modelIndex, title)
                })
                page.requestIndex.connect(thisItem.requestIndex)
            }

            onClicked: {
                if (selectable) {
                    selectionModel.toggleSelection(selectionKey)
                } else if (!grid.contextMenu.active && !delegate.isBusy) {
                    var sourceIndex = picsModel.mapToSource(modelIndex)
                    var page = pageStack.push(Qt.resolvedUrl("EncryptedFullscreenPage.qml"),{
                        currentIndex: sourceIndex,
                        model: foilModel
                    })
                    if (page) {
                        thisItem.requestIndex(sourceIndex)
                        page.decryptItem.connect(thisItem.decryptItem)
                        page.deleteItem.connect(thisItem.deleteItem)
                        page.requestIndex.connect(thisItem.requestIndex)
                    }
                }
            }

            onPressAndHold: {
                if (!selectable && !delegate.isBusy) {
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
            MenuItem {
                //: Generic menu item
                //% "Image details"
                text: qsTrId("foilpics-menu-details")
                onClicked: grid.expandItem.editDetails()
            }
        }
    }

    states: [
        State {
            when: expanded
            PropertyChanges {
                target: arrow
                rotation: -90
            }
            PropertyChanges {
                target: grid
                height: grid.contentHeight + grid.expandHeight
            }
        },
        State {
            when: !expanded
            PropertyChanges {
                target: arrow
                rotation: 0
            }
            PropertyChanges {
                target: grid
                height: 0
            }
        }
    ]

    transitions: Transition {
        enabled: _constructed
        to: "*"
        NumberAnimation {
            target: grid
            duration: 200
            easing.type: Easing.InOutQuad
            property: "height"
        }
        NumberAnimation {
            target: arrow
            duration: 200
            easing.type: Easing.InOutQuad
            property: "rotation"
        }
    }
}
