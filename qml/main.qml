import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

ApplicationWindow {
    id: app

    allowedOrientations: Orientation.All
    initialPage: _jailed ? jailPageComponent : encryptedPageComponent

    //: Application title
    //% "Foil Pics"
    readonly property string _appTitle: qsTrId("foilpics-app_name")
    readonly property bool _jailed: HarbourProcessState.jailedApp

    Component.onCompleted: if (!_jailed) pageStack.pushAttached(galleryPageComponent)

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

    Connections {
        target: HarbourSystemState

        property bool wasDimmed

        onDisplayStatusChanged: {
            if (target.displayStatus === HarbourSystemState.MCE_DISPLAY_DIM) {
                wasDimmed = true
            } else if (target.displayStatus === HarbourSystemState.MCE_DISPLAY_ON) {
                wasDimmed = false
            }
        }

        onLockedChanged: {
            lockTimer.stop()
            if (FoilPicsSettings.autoLock && target.locked) {
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

    Connections {
        target: FoilPicsSettings

        onAutoLockChanged: {
            lockTimer.stop()
            // It's so unlikely that settings change when the device is locked
            // But it's possible!
            if (target.autoLock && HarbourSystemState.locked) {
                appFoilModel.lock(false);
            }
        }
    }

    Component {
        id: encryptedPageComponent
        EncryptedPage {
            hints: appHints
            foilModel: appFoilModel
            allowedOrientations: app.allowedOrientations
        }
    }

    Component {
        id: galleryPageComponent
        GalleryPage {
            hints: appHints
            foilModel: appFoilModel
            allowedOrientations: app.allowedOrientations
        }
    }

    Component {
        id: jailPageComponent
        JailPage {
            allowedOrientations: app.allowedOrientations
        }
    }

    cover: Component {
        CoverPage {
            foilModel: appFoilModel
        }
    }
}
