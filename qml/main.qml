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

    //: Application title
    //% "Foil Pics"
    readonly property string title: qsTrId("foilpics-app_name")

    initialPage: Component {
        MainPage {
            hints: appHints
            foilModel: appFoilModel
            foilTransferMethodsModel: appTransferMethodsModel
            allowedOrientations: appAllowedOrientations
        }
    }

    cover: Component {
        CoverPage {
            foilModel: appFoilModel
        }
    }
}
