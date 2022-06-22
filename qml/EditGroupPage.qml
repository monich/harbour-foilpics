import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: thisPage

    allowedOrientations: Orientation.All
    property alias groupModel: list.model
    property string selectedGroupId
    property var editItem
    property var dragItem

    signal groupSelected(var index)

    SilicaListView {
        id: list

        anchors.fill: parent
        interactive: !Drag.active
        readonly property real maxContentY: Math.max(originY + contentHeight - height, originY)

        header: PageHeader {
            //: Page header
            //% "Groups"
            title: qsTrId("foilpics-edit_group_page-header")
        }

        delegate: ListItem {
            id: delegate

            width: parent.width
            showMenuOnPressAndHold: !defaultGroup && !dragging
            highlighted: down && !dragging
            property int modelIndex: index

            menu: Component {
                ContextMenu {
                    MenuItem {
                        //: Group context menu item
                        //% "Rename"
                        text: qsTrId("foilpics-edit_group_page-menu-rename")
                        onClicked: delegate.startEditing()
                    }
                    MenuItem {
                        //: Group context menu item
                        //% "Clear"
                        text: qsTrId("foilpics-edit_group_page-menu-clear")
                        visible: !defaultGroup && groupPicsCount > 0
                        onClicked: {
                            if (selectedGroupId == groupId) {
                                selectedGroupId = ""
                            }
                            groupModel.clearGroupAt(index)
                        }
                    }
                    MenuItem {
                        //: Group context menu item
                        //% "Remove"
                        text: qsTrId("foilpics-edit_group_page-menu-remove")
                        visible: !defaultGroup && !groupPicsCount
                        onClicked: groupModel.removeGroupAt(index)
                    }
                }
            }

            function startEditing() {
                groupItem.editText = groupName
                editItem = delegate
                groupItem.startEditing()
            }

            GroupListItem {
                id: groupItem

                width: delegate.width
                height: delegate.contentHeight
                defaultGroup: model.defaultGroup
                groupName: model.groupName
                count: model.groupPicsCount
                selected: selectedGroupId === model.groupId
                highlighted: delegate.highlighted
                editing: editItem === delegate
                scale: dragging ? 1.05 : 1
                color: dragging ? Theme.rgba(Theme.highlightBackgroundColor, 0.2) : "transparent"
                onDoneEditing: {
                    editItem = null
                    if (editText.length > 0) {
                        model.groupName = editText
                    }
                }
                onYChanged: {
                    if (dragging) {
                        var i = Math.floor((y + list.contentY + height/2)/height)
                        if (i >= 0 && i !== modelIndex) {
                            // Swap the items
                            groupModel.setDragPos(i)
                        }
                        if ((y + height) > list.height) {
                            // Scroll the list down
                            list.contentY = Math.min(list.maxContentY, list.contentY + y + height - list.height)
                        } else if (y < 0) {
                            // Scroll the list up
                            list.contentY = Math.max(list.originY, list.contentY + y)
                        }
                    }
                }
                states: [
                    State {
                        when: dragging
                        ParentChange {
                            target: groupItem
                            parent: list
                            y: groupItem.mapToItem(list, 0, 0).y
                        }
                    },
                    State {
                        when: !dragging
                        ParentChange {
                            target: groupItem
                            parent: delegate
                            y: 0
                        }
                    }
                ]
                Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.InOutQuad } }
                Behavior on color { ColorAnimation { duration: 150 } }
            }

            onClicked: {
                selectedGroupId = groupId
                thisPage.groupSelected(index)
            }

            readonly property real dragEdge: Theme.horizontalPageMargin
            readonly property real dragThresholdX: Theme.itemSizeSmall/2
            readonly property real dragThresholdY: Theme.itemSizeSmall/5
            readonly property bool dragging: dragItem === delegate
            property real pressX
            property real pressY
            property real dragStartY

            Timer {
                id: dragTimer

                interval: 150
            }

            onPressed: {
                pressX = mouseX
                pressY = mouseY
                dragTimer.restart()
            }

            function startDrag(mouseY) {
                editItem = null
                dragStartY = mouseY
                dragItem = delegate
                groupModel.setDragIndex(modelIndex)
            }

            function stopDrag() {
                groupModel.setDragIndex(-1)
                if (dragItem === delegate) {
                    dragItem = null
                }
            }

            onReleased: stopDrag()
            onCanceled: stopDrag()
            onMouseXChanged: {
                // Don't start drag if the initial touch was too close to the edge
                if (!dragging && pressed && !menuOpen && !dragTimer.running &&
                        pressX > dragEdge && pressX < (width - dragEdge) &&
                        Math.abs(mouseX - pressX) > dragThresholdX) {
                    startDrag(mouseY)
                }
            }
            onMouseYChanged: {
                // Don't start drag if the initial touch was too close to the edge
                if (!dragging && pressed && !menuOpen && !dragTimer.running &&
                        pressX > dragEdge && pressX < (width - dragEdge) &&
                        Math.abs(mouseY - pressY) > dragThresholdY) {
                    startDrag(mouseY)
                }
            }

            drag.target: dragging ? groupItem : null
            drag.axis: Drag.YAxis

            ListView.onAdd: AddAnimation { target: delegate }
            ListView.onRemove: RemoveAnimation { target: delegate }
        }

        moveDisplaced: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 150
                easing.type: Easing.InOutQuad
            }
        }

        footer: BackgroundItem {
            id: addGroupItem

            Image {
                id: addIcon

                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                source: "image://theme/icon-m-add" + (addGroupItem.highlighted ? "?" + Theme.highlightColor : "")
            }

            Label {
                //: Button label
                //% "Add group"
                text: qsTrId("foilpics-edit_group_page-add_group")
                anchors {
                    left: addIcon.right
                    leftMargin: Theme.paddingSmall
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                }
                color: addGroupItem.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            onClicked: {
                //: Name for the newly created group
                //% "New group"
                groupModel.addGroup(qsTrId("foilpics-edit_group_page-new_group_name"))
                list.currentIndex = groupModel.count - 1
                list.currentItem.startEditing()
            }
        }

        VerticalScrollDecorator { }
    }
}
