import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

Page {
    id: page

    property alias model: imageList.model
    property alias currentIndex: imageList.currentIndex
    property var currentImageItem: model ? model.get(currentIndex) : null

    signal decryptItem(int index)
    signal deleteItem(int index)
    signal requestIndex(int index)

    allowedOrientations: Orientation.All
    backNavigation: drawer.open

    function pop() {
        pageStack.pop(pageStack.previousPage(page))
    }

    Connections {
        target: model
        onFoilStateChanged: {
            // Pop the page when the model gets locked
            if (status === PageStatus.Active &&
                ((page.model === undefined || page.model.foilState !== FoilPicsModel.FoilPicsReady))) {
                page.pop()
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            visible: drawer.open
            MenuItem {
                //: Generic menu item
                //% "Delete"
                text: qsTrId("foilpics-menu-delete")
                onClicked: page.deleteItem(page.currentIndex)
            }
            MenuItem {
                //: Generic menu item
                //% "Decrypt"
                text: qsTrId("foilpics-menu-decrypt")
                onClicked: page.decryptItem(page.currentIndex)
            }
        }

        Drawer {
            id: drawer
            dock: Dock.Top
            hideOnMinimize: true
            anchors.fill: parent
            open: true
            backgroundSize: header.height

            background: Item {
                id: header
                height: page.isPortrait ? portraitHeader.height : landscapeHeader.height
                width: parent.width
                PageHeader {
                    id: portraitHeader
                    title: currentImageItem.title
                    rightMargin: Theme.horizontalPageMargin
                    description: currentImageItem ? currentImageItem.imageWidth + "×" + currentImageItem.imageHeight : ""
                    visible: page.isPortrait
                }
                PageHeader {
                    id: landscapeHeader
                    title: currentImageItem.title
                    rightMargin: Theme.paddingLarge
                    visible: !page.isPortrait
                }
            }

            foreground: FlickableImageView {
                id: imageList

                pathItemCount: 3

                contentWidth: page.width
                contentHeight: page.height

                width: drawer.foregroundItem.width
                height: drawer.foregroundItem.height

                isPortrait: page.isPortrait
                menuOpen: drawer.open

                onClicked: {
                    drawer.open = !drawer.open
                }
            }
        }
    }

    onCurrentIndexChanged: {
        if (status === PageStatus.Active) {
            if (model === undefined || currentIndex >= model.count) {
                // This can happen if all of the images are deleted
                // or the model gets locked
                page.pop()
            } else {
                requestIndex(currentIndex)
            }
        }
    }
}
