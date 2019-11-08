import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

MouseArea {
    id: thumbnail

    property url source
    property bool down: pressed && containsMouse
    property string mimeType: model && model.mimeType ? model.mimeType : ""
    property bool pressedAndHeld
    property int size: GridView.view.cellSize
    property real contentYOffset
    property real contentXOffset
    property GridView grid: GridView.view
    property var selectionModel
    property string selectionKey

    property var selectionState
    property var busyState

    readonly property bool isBusy: busyState ? busyState.busy : false
    readonly property bool isSelected: selectionState ? selectionState.selected : false

    width: size
    height: size
    opacity: grid ?
        (grid.currentIndex === index && grid.unfocusHighlightEnabled ? 1.0 : grid._unfocusedOpacity) : 1.0

    // Default behavior for each thumbnail
    onPressed: if (grid) grid.currentIndex = index
    onPressAndHold: pressedAndHeld = true
    onReleased: pressedAndHeld = false
    onCanceled: pressedAndHeld = false

    onSelectionModelChanged: selectionStateCheck()
    onSelectionKeyChanged: selectionStateCheck()

    function selectionStateCheck() {
        if (selectionState) {
            selectionState.destroy()
            selectionState = null
            busyState.destroy()
            busyState = null
        }
        if (selectionModel && selectionKey) {
            selectionState = selectionStateComponent.createObject(thumbnail, {
                model: selectionModel,
                key: selectionKey})
            busyState = busyStateComponent.createObject(thumbnail, {
                model: selectionModel,
                key: selectionKey})
        }
    }

    Component {
        id: selectionStateComponent

        FoilPicsSelectionState { }
    }

    Component {
        id: busyStateComponent

        FoilPicsBusyState { }
    }

    Loader {
        z: thumbnail.z + 1
        anchors.fill: parent
        active: opacity > 0
        opacity: isSelected ? 1 : 0
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
        opacity: isBusy ? 1 : 0
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
                    running: isBusy
                }
            }
        }
        Behavior on opacity { FadeAnimation { duration: 100 } }
    }
}
