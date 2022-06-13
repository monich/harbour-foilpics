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
    readonly property bool _sailfishShare: HarbourSystemInfo.compareVersions(_sharingApiVersion, "0.4.0") >= 0 // QML API break

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
            model: HarbourTransferMethodsModel

            Binding {
                target: HarbourTransferMethodsModel
                property: "filter"
                value: "image/*"
            }

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
        id: detailsBackgroundComponent

        SilicaFlickable {
            anchors.fill: parent
            contentHeight: detailsColumn.height

            PullDownMenu {
                MenuItem {
                    //: Generic menu item
                    //% "Delete"
                    text: qsTrId("foilpics-menu-delete")
                    onClicked: page.deleteItem(page.currentIndex)
                }
                MenuItem {
                    //: Generic menu item
                    //% "Share"
                    text: qsTrId("foilpics-menu-share")
                    visible: _sailfishShare
                    onClicked: {
                        if (!shareAction) {
                            shareAction = Qt.createQmlObject("import Sailfish.Share 1.0;ShareAction {}",
                                page, "SailfishShare")
                        }
                        if (shareAction) {
                            shareAction.resources = [ currentImageItem.url ]
                            shareAction.trigger()
                        }
                    }
                    property var shareAction
                }
                MenuItem {
                    //: Generic menu item
                    //% "Encrypt"
                    text: qsTrId("foilpics-menu-encrypt")
                    visible: foilModel.keyAvailable
                    onClicked: page.encryptItem(page.currentIndex)
                }
            }

            Column {
                id: detailsColumn

                width: parent.width

                PageHeader {
                    width: parent.width
                    title: imageList.itemTitle
                    //: Page header
                    //% "Details"
                    description: qsTrId("foilpics-details-header")
                }

                GalleryDetailsView {
                    id: view

                    width: parent.width
                    item: currentImageItem.itemId
                }
            }

            VerticalScrollDecorator { }
        }
    }

    Drawer {
        id: drawer

        dock: page.isPortrait ? Dock.Top: Dock.Left
        hideOnMinimize: true
        anchors.fill: parent
        backgroundSize: Math.min(Math.min(page.width, page.height), Math.max(page.width/2, page.height/2))
        open: true

        background: Loader {
            anchors.fill: parent
            sourceComponent: _sailfishShare ? detailsBackgroundComponent : sharingBackgroundComponent
        }

        foreground: FlickableImageView {
            id: imageList

            contentWidth: page.width
            contentHeight: page.height

            pathItemCount: 3
            width: drawer.foregroundItem.width
            height: drawer.foregroundItem.height
            isPortrait: page.isPortrait
            menuOpen: drawer.open

            onClicked: drawer.open = !drawer.open
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
