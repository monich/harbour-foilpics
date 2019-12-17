import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property var foilModel
    property alias model: imageList.model
    property alias currentIndex: imageList.currentIndex
    property var currentImageItem: model ? model.get(currentIndex) : null

    signal encryptItem(int index)
    signal deleteItem(int index)
    signal requestIndex(int index)

    allowedOrientations: Orientation.All
    backNavigation: drawer.open

    Drawer {
        id: drawer
        dock: page.orientation & Orientation.PortraitMask ? Dock.Top: Dock.Left
        hideOnMinimize: true
        anchors.fill: parent
        open: true

        background: ShareMethodList {
            id: shareMethodsList

            anchors.fill: parent
            source: currentImageItem ? currentImageItem.url : ""

            PullDownMenu {
                MenuItem {
                    //: Generic menu item
                    //% "Image details"
                    text: qsTrId("foilpics-menu-details")
                    onClicked: pageStack.push(Qt.resolvedUrl("GalleryDetailsPage.qml"), {
                        item: currentImageItem.itemId
                    })
                }
                MenuItem {
                    //: Generic menu item
                    //% "Delete"
                    text: qsTrId("foilpics-menu-delete")
                    onClicked: page.deleteItem(page.currentIndex)
                }
                MenuItem {
                    //: Generic menu item
                    //% "Encrypt"
                    text: qsTrId("foilpics-menu-encrypt")
                    visible: foilModel.keyAvailable
                    onClicked: page.encryptItem(page.currentIndex)
                }
            }

            header: PageHeader {
                title: currentImageItem.title
                //: Page header
                //% "Share"
                description: qsTrId("foilpics-gallery_fullscreen_view-header")
                rightMargin: page.isPortrait ? Theme.horizontalPageMargin : Theme.paddingLarge
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

    onCurrentIndexChanged: {
        if (status !== PageStatus.Active) {
            return
        }
        if (model === undefined || currentIndex >= model.count) {
            // This can happen if all of the images are deleted
            var firstPage = pageStack.previousPage(page)
            while (pageStack.previousPage(firstPage)) {
                firstPage = pageStack.previousPage(firstPage)
            }
            pageStack.pop(firstPage)
            return
        }
        requestIndex(currentIndex)
    }
}
