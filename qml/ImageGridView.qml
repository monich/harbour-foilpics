import QtQuick 2.0
import Sailfish.Silica 1.0

SilicaGridView {
    id: grid

    readonly property real cellSize: Math.floor(width / columnCount)
    readonly property int columnCount: Math.floor(width / Theme.itemSizeHuge)
    property bool highlightEnabled: true
    property bool unfocusHighlightEnabled
    property bool forceUnfocusHighlight
    property real _unfocusedOpacity: unfocusHighlightEnabled && (currentItem != null && currentItem.pressedAndHolded)
                                     || forceUnfocusHighlight
                                     ? 0.2 : 1.0
    Behavior on _unfocusedOpacity { FadeAnimation {} }

    currentIndex: -1
    cacheBuffer: 1000
    cellWidth: cellSize
    cellHeight: cellSize

    // Make header visible if it exists.
    Component.onCompleted: if (header) grid.positionViewAtBeginning()

    maximumFlickVelocity: 5000*Theme.pixelRatio

    VerticalScrollDecorator { }

    function deleteItem(index) {
        pageStack.pop()
        grid.currentIndex = index
        grid.currentItem.remove()
        grid.positionViewAtIndex(index, GridView.Visible)
    }

    function jumpToIndex(index) {
        if (index < grid.columnCount) {
            grid.positionViewAtBeginning()
        } else {
            grid.positionViewAtIndex(index, GridView.Visible)
        }
        grid.currentIndex = index
    }

    function createRemorseContainer(item) {
        return remorseContainerComponent.createObject(item)
    }

    // Item must have contentYOffset property
    function requestDelete(item, callback) {
        var remorse = removalComponent.createObject(null)
        remorse.z = item.z + 1
        remorse.execute(createRemorseContainer(item),
            //: Deleting image in 5 seconds
            //% "Deleting"
            qsTrId("foilpics-image_grid_view-remorse-deleting"),
            callback)
    }

    HighlightItem {
        id: highlightItem
        width: grid.cellWidth
        height: grid.cellHeight
        active: highlightEnabled && _unfocusedOpacity == 1 && grid.currentIndex > -1
                 && grid.currentItem.down
        x: grid.currentItem != null ? grid.currentItem.x : 0
        y: grid.currentItem != null ? grid.currentItem.y - grid.contentY : 0
        z: grid.currentItem != null ? grid.currentItem.z + 1 : 0
    }

    // This container is used for making RemorseItem to follow
    // offset changes if there are multiple deletions ongoing
    // at the same time.
    Component {
        id: remorseContainerComponent
        Item {
            y: parent.contentYOffset
            width: parent.width
            height: parent.height
        }
    }

    Component {
        id: removalComponent
        RemorseItem {
            //: RemorseItem cancel help text
            //% "Cancel"
            cancelText: qsTrId("foilpics-image_grid_view-remorse-cancel_deletion")
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
