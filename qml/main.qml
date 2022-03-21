import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

ApplicationWindow {
    id: app

    readonly property bool jailed: HarbourProcessState.jailedApp
    readonly property int appAllowedOrientations: Orientation.All
    readonly property bool appLandscapeMode: orientation === Qt.LandscapeOrientation ||
        orientation === Qt.InvertedLandscapeOrientation
    allowedOrientations: appAllowedOrientations

    FoilPicsModel {
        id: appFoilModel
        thumbnailSize: Qt.size(Theme.itemSizeHuge,Theme.itemSizeHuge)
        onMediaDeleted: FoilPics.mediaDeleted(url)
    }

    FoilPicsHints {
        id: appHints
    }

    Timer {
        id: lockTimer

        interval: FoilPicsSettings.autoLockTime
        onTriggered: appFoilModel.lock(true);
    }

    SystemState {
        property bool wasDimmed

        onDisplayStatusChanged: {
            if (displayStatus === MCE_DISPLAY_DIM) {
                wasDimmed = true
            } else if (displayStatus === MCE_DISPLAY_ON) {
                wasDimmed = false
            }
        }

        onLockedChanged: {
            lockTimer.stop()
            if (locked) {
                if (wasDimmed) {
                    // Give the user some time to wake wake up the screen
                    // and prevent encrypted pictures from being locked
                    lockTimer.start()
                } else {
                    appFoilModel.lock(false);
                }
            }
        }
    }

    //: Application title
    //% "Foil Pics"
    readonly property string appTitle: qsTrId("foilpics-app_name")

    initialPage: jailed ? jailPageComponent : encryptedPageComponent

    Component.onCompleted: if (!jailed) pageStack.pushAttached(galleryPageComponent)

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
        }
    }

    Component {
        id: jailPageComponent
        JailPage {
            allowedOrientations: appAllowedOrientations
        }
    }

    cover: Component {
        CoverPage {
            foilModel: appFoilModel
        }
    }
}
