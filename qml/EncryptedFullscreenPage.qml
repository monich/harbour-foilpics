import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

Page {
    id: page

    property alias model: imageList.model
    property alias currentIndex: imageList.currentIndex
    property var currentImageItem

    signal decryptItem(int index)
    signal deleteItem(int index)
    signal requestIndex(int index)

    allowedOrientations: Orientation.All
    backNavigation: drawer.open

    onCurrentIndexChanged: {
        updateCurrentImageItem()
        if (updateCurrentImageItem() && status === PageStatus.Active) {
            requestIndex(currentIndex)
        }
    }

    Component.onCompleted: updateCurrentImageItem()

    function updateCurrentImageItem() {
        if (model && currentIndex >= 0 && currentIndex < model.count) {
            currentImageItem = model.get(currentIndex)
            return true
        } else {
            currentImageItem = null
            return false
        }
    }

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            visible: drawer.open
            MenuItem {
                //: Generic menu item
                //% "Image details"
                text: qsTrId("foilpics-menu-details")
                onClicked: {
                    var details = pageStack.push(Qt.resolvedUrl("EncryptedDetailsPage.qml"), {
                        details: currentImageItem
                    })
                    details.titleChanged.connect(function(title) {
                        imageList.model.setTitleAt(currentIndex, title)
                    })
                }
            }
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
                    title: imageList.itemTitle
                    rightMargin: Theme.horizontalPageMargin
                    description: currentImageItem ? currentImageItem.imageWidth + "×" + currentImageItem.imageHeight : ""
                    visible: page.isPortrait
                }
                PageHeader {
                    id: landscapeHeader
                    title: imageList.itemTitle
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

                onItemTitleChanged: updateCurrentImageItem()
            }
        }
    }
}
