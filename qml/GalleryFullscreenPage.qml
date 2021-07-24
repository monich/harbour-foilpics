import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

Page {
    id: page

    property var foilModel
    property alias model: imageList.model
    property alias currentIndex: imageList.currentIndex
    property var currentImageItem: model ? model.get(currentIndex) : null

    readonly property string _sharingApiVersion: HarbourSystemInfo.packageVersion("declarative-transferengine-qt5")
    readonly property bool _sharingBroken: HarbourSystemInfo.compareVersions(_sharingApiVersion, "0.4.0") >= 0 // QML API break

    signal encryptItem(int index)
    signal deleteItem(int index)
    signal requestIndex(int index)

    allowedOrientations: Orientation.All
    backNavigation: drawer.open

    Component {
        id: sharingBackgroundComponent

        HarbourShareMethodList {
            readonly property int backgroundSize: Math.min(Math.min(page.width, page.height), Math.max(page.width/2, page.height/2))

            anchors.fill: parent
            source: currentImageItem ? currentImageItem.url : ""
            model: TransferMethodsModel

            //: Share list item
            //% "Add account"
            addAccountText: qsTrId("foilpics-share_method_list-add_account")

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
    }

    Component {
        id: nonSharingBackgroundComponent

        PageHeader {
            readonly property int backgroundSize: height

            width: page.width
            title: imageList.itemTitle
        }
    }

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            visible: _sharingBroken && drawer.open

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

        Drawer {
            id: drawer
            dock: (_sharingBroken || page.isPortrait) ? Dock.Top: Dock.Left
            hideOnMinimize: true
            anchors.fill: parent
            backgroundSize: backgroundLoader.item ? backgroundLoader.item.backgroundSize : 0
            open: true

            background: Loader {
                id: backgroundLoader

                anchors.fill: parent
                sourceComponent: _sharingBroken ? nonSharingBackgroundComponent : sharingBackgroundComponent
            }

            foreground: FlickableImageView {
                id: imageList

                contentWidth: page.width
                contentHeight: page.height

                pathItemCount: 3
                width: drawer.foregroundItem.width
                height: drawer.foregroundItem.height
                isPortrait: page.isPortrait
                menuOpen: !_sharingBroken && drawer.open

                onClicked: {
                    drawer.open = !drawer.open
                }
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
