import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

ApplicationWindow {
    id: app
    readonly property int appAllowedOrientations: Orientation.All
    readonly property bool appLandscapeMode: orientation === Qt.LandscapeOrientation ||
        orientation === Qt.InvertedLandscapeOrientation
    allowedOrientations: appAllowedOrientations

    FoilPicsModel {
        id: appFoilModel
        thumbnailSize: Qt.size(Theme.itemSizeHuge,Theme.itemSizeHuge)
    }

    FoilPicsHints {
        id: appHints
    }

    TransferMethodsModel {
        id: appTransferMethodsModel
        filter: "image/*"
    }

    SystemState {
        onLockModeChanged: if (locked) appFoilModel.lock(false);
    }

    //: Application title
    //% "Foil Pics"
    readonly property string appTitle: qsTrId("foilpics-app_name")

    initialPage: encryptedPageComponent

    Component.onCompleted: pageStack.pushAttached(galleryPageComponent)

    Component {
        id: encryptedPageComponent
        EncryptedPage {
            hints: appHints
            foilModel: appFoilModel
            allowedOrientations: appAllowedOrientations
        }
    }

    Component {
        id: galleryPageComponent
        GalleryPage {
            hints: appHints
            foilModel: appFoilModel
            allowedOrientations: appAllowedOrientations
            transferMethodsModel: appTransferMethodsModel
        }
    }

    cover: Component {
        CoverPage {
            foilModel: appFoilModel
        }
    }
}
