import QtQuick 2.0
import Sailfish.Silica 1.0
//import QtDocGallery 5.0
import harbour.foilpics.QtDocGallery 1.0
import harbour.foilpics 1.0

SilicaFlickable {
    id: view
    contentHeight: height
    property alias hints: grid.hints
    property alias foilModel: grid.foilModel
    property alias transferMethodsModel: grid.transferMethodsModel
    property bool isCurrentView

    property alias selecting: grid.selecting
    property alias selectionModel: grid.selectionModel
    readonly property int selectedPicturesCount: selectionModel ? selectionModel.count : 0
    readonly property bool haveAnyPictures: galleryModel.count > 0

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
        onCountChanged: if (!count) selecting = false
    }

    // The parent loader is supposed to have isCurrentItem property

    Connections {
        target: parent
        onIsCurrentItemChanged: isCurrentView = target.isCurrentItem
    }

    Component.onCompleted: isCurrentView = parent.isCurrentItem
    onIsCurrentViewChanged: if (!isCurrentView) selecting = false

    // Selection model is fairly expensive, we create and maintain it
    // only in selection mode

    Component {
        id: selectionModelComponent
        FoilPicsSelection {
            model: galleryModel
            role: galleryModel.keyRole
            onSelectionChanged: bulkActionRemorse.cancelNicely()
            onSelectionCleared: bulkActionRemorse.cancelNicely()
        }
    }

    onSelectingChanged: {
        if (selecting) {
            selectionModel = selectionModelComponent.createObject(view)
        } else {
            bulkActionRemorse.cancelNicely()
            selectionModel.destroy()
            selectionModel = null
        }
    }

    PullDownMenu {
        id: pullDownMenu
        visible: haveAnyPictures
        MenuItem {
            id: selectPhotosMenuItem
            //: Pulley menu item
            //% "Select photos"
            text: qsTrId("foilpics-pulley_menu-select_photos")
            visible: !selecting && haveAnyPictures
            onClicked: selecting = true
        }
        MenuItem {
            id: selectNoneMenuItem
            //: Pulley menu item
            //% "Select none"
            text: qsTrId("foilpics-pulley_menu-select_none")
            visible: selecting && selectedPicturesCount > 0
            onClicked: selectionModel.clearSelection()
        }
        MenuItem {
            id: selectAllMenuItem
            //: Pulley menu item
            //% "Select all"
            text: qsTrId("foilpics-pulley_menu-select_all")
            visible: selecting && selectedPicturesCount < galleryModel.count
            onClicked: selectionModel.selectAll()
        }
    }

    RemorsePopup {
        id: bulkActionRemorse
        function cancelNicely() {
            // To avoid flickering, do it only when really necessary
            if (visible) cancel()
        }
    }

    GalleryGrid {
        id: grid
        clip: true
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: selectionPanel.top
        }
        model: galleryModel
    }

    GallerySelectionPanel {
        id: selectionPanel
        active: selecting
        canDelete: selectedPicturesCount > 0
        canEncrypt: selectedPicturesCount > 0 && foilModel.keyAvailable
        //: Generic menu item
        //% "Delete"
        onDeleteHint: showHint(qsTrId("foilpics-menu-delete"))
        onDeleteSelected: bulkActionRemorse.execute(
            //: Generic remorse popup text
            //% "Deleting %0 selected pictures"
            qsTrId("foilpics-remorse-deleting_selected", selectedPicturesCount).arg(selectedPicturesCount),
            function() {
                FileUtil.deleteLocalFilesFromModel(galleryModel, galleryModel.keyRole, selectionModel.makeSelectionBusy())
            })
        //: Generic menu item
        //% "Encrypt"
        onEncryptHint: showHint(qsTrId("foilpics-menu-encrypt"))
        onEncryptSelected: bulkActionRemorse.execute(
            //: Remorse popup text
            //% "Encrypting %0 selected pictures"
            qsTrId("foilpics-gallery_view-remorse-encrypting_selected", selectedPicturesCount).arg(selectedPicturesCount),
            function() {
                foilModel.encryptFiles(galleryModel, selectionModel.makeSelectionBusy())
            })
        //: Button that exits selection mode
        //% "Done"
        onDoneHint: showHint(qsTrId("foilpics-selection_panel-done_button"))
        onDone: selecting = false
    }

    function showHint(text) {
        selectionHint.text = text
        selectionHintTimer.restart()
    }

    InteractionHintLabel {
        id: selectionHint
        anchors.fill: grid
        visible: opacity > 0
        opacity: selectionHintTimer.running ? 1.0 : 0.0
        Behavior on opacity { FadeAnimation { duration: 1000 } }
    }

    Timer {
        id: selectionHintTimer
        interval: 1000
    }

    ViewPlaceholder {
        //: Placeholder text
        //% "The photo gallery seems to be empty"
        text: qsTrId("foilpics-gallery_view-placeholder-no_pictures")
        enabled: !galleryModel.count
    }
}
