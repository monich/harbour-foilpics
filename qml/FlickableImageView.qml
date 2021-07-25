import QtQuick 2.0
import Sailfish.Silica 1.0

SlideshowView {
    id: view

    property string itemTitle
    property string itemGroupName
    readonly property bool itemScaled: currentItem && currentItem.itemScaled
    readonly property bool itemTouchingVerticalEdge: currentItem && currentItem.itemTouchingVerticalEdge
    property bool isPortrait
    property bool menuOpen

    property real contentWidth: width
    property real contentHeight: height

    signal clicked

    interactive: (!itemScaled || itemTouchingVerticalEdge) && count > 1

    Component.onCompleted: {
        view.positionViewAtIndex(view.currentIndex, PathView.Center)
        // Simply doing this:
        //
        // property string itemTitle: currentItem ? currentItem.itemTitle : ""
        // property string itemGroupName: currentItem ? currentItem.itemGroupName : ""
        //
        // doesn't quite work with Qt 5.2 (e.g. Sailfish OS 2.0). The
        // initial values remain empty even though currentItem isn't null
        // and its properties are not empty.
        //
        // Setting up binding like this fixes that problem:
        itemTitle = Qt.binding(function() { return currentItem ? currentItem.itemTitle : "" })
        itemGroupName = Qt.binding(function() { return currentItem ? currentItem.itemGroupName : "" })
    }

    delegate: Item {
        id: delegate

        readonly property alias itemScaled: imageViewer.scaled
        readonly property bool itemTouchingVerticalEdge: imageViewer.atXBeginning || imageViewer.atXEnd
        readonly property string itemTitle: model.title
        readonly property string itemGroupName: model.groupName ? model.groupName : ""

        width: view.width
        height: view.height
        opacity: Math.abs(x) <= view.width ? 1.0 -  (Math.abs(x) / view.width) : 0

        ImageViewer {
            id: imageViewer

            width: view.contentWidth
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
