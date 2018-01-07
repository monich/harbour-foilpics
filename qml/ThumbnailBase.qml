import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

// Base item for thumbnails in a Grid to get default behavior for free.
// Make sure that this is a top level delegate item for a grid or
// some functionality (opacity, ...) will be lost
MouseArea {
    id: thumbnail

    property url source
    property bool down: pressed && containsMouse
    property string mimeType: model && model.mimeType ? model.mimeType : ""
    property bool pressedAndHolded
    property int size: GridView.view.cellSize
    property real contentYOffset
    property real contentXOffset
    property GridView grid: GridView.view
    property var selectionModel
    property string selectionKey

    width: size
    height: size
    opacity: grid ?
        (grid.currentIndex === index && grid.unfocusHighlightEnabled ? 1.0 : grid._unfocusedOpacity) : 1.0

    // Default behavior for each thumbnail
    onPressed: if (grid) grid.currentIndex = index
    onPressAndHold: pressedAndHolded = true
    onReleased: pressedAndHolded = false
    onCanceled: pressedAndHolded = false

    FoilPicsSelectionState {
        id: selectionState
        model: selectionModel ? selectionModel : null
        key: selectionKey
    }

    FoilPicsBusyState {
        id: busyState
        model: selectionModel ? selectionModel : null
        key: selectionKey
    }

    Loader {
        z: thumbnail.z + 1
        anchors.fill: parent
        active: opacity > 0
        opacity: selectionState.selected ? 1 : 0
        sourceComponent: Component {
            Item {
                anchors.fill: parent
                Rectangle {
                    anchors.fill: parent
                    color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
                }
                Image {
                    anchors {
                        bottom: parent.bottom
                        bottomMargin: Theme.paddingSmall
                        left: parent.left
                        leftMargin: Theme.paddingSmall
                    }
                    source: "image://theme/icon-s-installed"
                }
            }
        }
        Behavior on opacity { FadeAnimation { duration: 100 } }
    }


    Loader {
        z: thumbnail.z + 2
        anchors.fill: parent
        active: opacity > 0
        opacity: busyState.busy ? 1 : 0
        sourceComponent: Component {
            Item {
                anchors.fill: parent
                Rectangle {
                    id: background
                    width: parent.width
                    height: width
                    color: Theme.highlightDimmerColor
                    opacity: 0.7
                }
                BusyIndicator {
                    size: BusyIndicatorSize.Medium
                    anchors.centerIn: background
                    running: busyState.busy
                }
            }
        }
        Behavior on opacity { FadeAnimation { duration: 100 } }
    }
}
