import QtQuick 2.0
import Sailfish.Silica 1.0

SlideshowView {
    id: view

    readonly property string itemTitle: currentItem !== null ? currentItem.itemTitle : ""
    readonly property bool itemScaled: currentItem !== null && currentItem.itemScaled
    readonly property bool itemTouchingVerticalEdge: currentItem !== null && currentItem.itemTouchingVerticalEdge
    property bool isPortrait
    property bool menuOpen

    property real contentWidth: width
    property real contentHeight: height

    signal clicked

    interactive: (!itemScaled || itemTouchingVerticalEdge) && count > 1

    Component.onCompleted: {
        view.positionViewAtIndex(view.currentIndex, PathView.Center)
    }

    delegate: Item {
        id: delegate
        readonly property alias itemScaled: imageViewer.scaled
        readonly property bool itemTouchingVerticalEdge: imageViewer.atXBeginning || imageViewer.atXEnd
        readonly property string itemTitle: title

        width: view.width
        height: view.height
        opacity: Math.abs(x) <= view.width ? 1.0 -  (Math.abs(x) / view.width) : 0

        ImageViewer {
            id: imageViewer
            width: view.isPortrait ? Screen.width : Screen.height
            height: view.contentHeight
            anchors.centerIn: parent

            source: model.url
            menuOpen: view.menuOpen
            fitWidth: view.isPortrait
            orientation: model.orientation
            viewMoving: view.moving
            isCurrentItem: delegate.PathView.isCurrentItem

            onClicked: view.clicked()
        }
    }   
}
