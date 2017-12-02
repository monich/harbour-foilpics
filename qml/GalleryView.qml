import QtQuick 2.0
import Sailfish.Silica 1.0
//import QtDocGallery 5.0
import harbour.foilpics.QtDocGallery 1.0

SilicaFlickable {
    property alias hints: grid.hints
    property alias foilModel: grid.foilModel
    property alias transferMethodsModel: grid.transferMethodsModel

    DocumentGalleryModel {
        id: galleryModel
        rootType: DocumentGallery.Image
        properties: ["url", "mimeType", "title", "orientation", "dateTaken", "width", "height" ]
        sortProperties: ["-dateTaken"]
        autoUpdate: true
        filter: GalleryStartsWithFilter { property: "filePath"; value: StandardPaths.music; negated: true }
    }

    GalleryGrid {
        id: grid
        anchors.fill: parent
        model: galleryModel
    }

    ViewPlaceholder {
        //: Placeholder text
        //% "The photo gallery seems to be empty"
        text: qsTrId("foilpics-gallery_view-placeholder-no_pictures")
        enabled: !galleryModel.count
    }
}
