import QtQuick 2.0
import Sailfish.Silica 1.0

SlideshowView {
    id: view

    property bool itemScaled: currentItem !== null && currentItem.itemScaled
    property bool isPortrait
    property bool menuOpen

    property real contentWidth: width
    property real contentHeight: height

    signal clicked

    function currentItemUrl() {
        return model.get(currentIndex).url
    }

    interactive: !itemScaled && count > 1

    Component.onCompleted: {
        view.positionViewAtIndex(view.currentIndex, PathView.Center)
    }

    delegate: Item {
        id: mediaItem

        property QtObject modelData: model
        property alias itemScaled: imageViewer.scaled
        readonly property url source: model.url

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
