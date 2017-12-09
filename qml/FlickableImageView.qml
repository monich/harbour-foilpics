import QtQuick 2.0
import Sailfish.Silica 1.0

SlideshowView {
    id: view

    property string itemTitle: currentItem !== null ? currentItem.itemTitle : ""
    property bool itemScaled: currentItem !== null && currentItem.itemScaled
    property bool isPortrait
    property bool menuOpen

    property real contentWidth: width
    property real contentHeight: height

    signal clicked

    interactive: !itemScaled && count > 1

    Component.onCompleted: {
        view.positionViewAtIndex(view.currentIndex, PathView.Center)
    }

    delegate: Item {
        property alias itemScaled: imageViewer.scaled
        property string itemTitle: title

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
            enableZoom: !view.moving

            onClicked: view.clicked()
        }
    }   
}
