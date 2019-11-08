import QtQuick 2.0
import Sailfish.Silica 1.0
//import QtDocGallery 5.0
import harbour.foilpics.QtDocGallery 1.0
import harbour.foilpics 1.0

Page {
    id: page

    property alias hints: grid.hints
    property alias foilModel: grid.foilModel
    property alias transferMethodsModel: grid.transferMethodsModel
    property alias selectionModel: grid.selectionModel
    property bool dropSelectionModelWhenEncryptionDone

    DocumentGalleryModel {
        id: galleryModel
        rootType: DocumentGallery.Image
        readonly property string keyRole: "url"
        properties: [
            "url", "mimeType", "title", "orientation", "dateTaken",
            "width", "height", "cameraManufacturer", "cameraModel",
            "latitude", "longitude", "altitude"
        ]
        sortProperties: ["-dateTaken"]
        autoUpdate: true
        filter: GalleryStartsWithFilter {
            property: "filePath"
            value: StandardPaths.music
            negated: true
        }
    }

    // Selection model is fairly expensive, we create and maintain it
    // only in selection mode
    Component {
        id: selectionModelComponent

        FoilPicsSelection {
            model: galleryModel
            role: galleryModel.keyRole
        }
    }

    GalleryGrid {
        id: grid

        clip: true
        anchors.fill: parent
        model: galleryModel

        PullDownMenu {
            visible: galleryModel.count > 0
            MenuItem {
                //: Pulley menu item
                //% "Select photos"
                text: qsTrId("foilpics-pulley_menu-select_photos")
                onClicked: selectPictures()
            }
        }

        ViewPlaceholder {
            //: Placeholder text
            //% "The photo gallery seems to be empty"
            text: qsTrId("foilpics-gallery_view-placeholder-no_pictures")
            enabled: !galleryModel.count
        }
    }

    // This is to drop selection mode when encryption is done:
    Connections {
        target: foilModel
        onBusyChanged: {
            if (dropSelectionModelWhenEncryptionDone && !foilModel.busy) {
                dropSelectionModelWhenEncryptionDone = false
                dropSelectionModel()
            }
        }
    }

    function dropSelectionModel() {
        if (selectionModel) {
            selectionModel.destroy()
            selectionModel = null
        }
    }

    function selectPictures() {
        dropSelectionModel()
        bulkActionRemorse.cancelNicely()
        selectionModel = selectionModelComponent.createObject(page)
        var selectionPage = pageStack.push(Qt.resolvedUrl("GallerySelectionPage.qml"), {
            allowedOrientations: page.allowedOrientations,
            foilModel: grid.foilModel,
            dataModel: galleryModel,
            selectionModel: selectionModel
        })
        selectionPage.deletePictures.connect(function(list) {
            pageStack.pop()
            grid.positionViewAtIndex(list[0], GridView.Visible)
            //: Generic remorse popup text
            //% "Deleting %0 selected pictures"
            bulkActionRemorse.execute(qsTrId("foilpics-remorse-deleting_selected", list.length).arg(list.length), function() {
                FileUtil.deleteLocalFilesFromModel(galleryModel, galleryModel.keyRole, list)
                dropSelectionModel()
            })
        })
        selectionPage.encryptPictures.connect(function(list) {
            pageStack.pop()
            grid.positionViewAtIndex(list[0], GridView.Visible)
            //: Remorse popup text
            //% "Encrypting %0 selected pictures"
            bulkActionRemorse.execute(qsTrId("foilpics-gallery_view-remorse-encrypting_selected", list.length).arg(list.length), function() {
                foilModel.encryptFiles(galleryModel, list)
                if (foilModel.busy) {
                    dropSelectionModelWhenEncryptionDone = true
                } else {
                    // Well, this shouldn't happen but just in case...
                    dropSelectionModel()
                }
            })
        })
    }

    RemorsePopup {
        id: bulkActionRemorse

        function cancelNicely() {
            // To avoid flickering, do it only when really necessary
            if (visible) cancel()
        }
        onCanceled: dropSelectionModel()
    }
}
