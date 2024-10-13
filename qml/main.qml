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

    function resetAutoLock() {
        lockTimer.stop()
        if (appFoilModel.keyAvailable && FoilPicsSettings.autoLock && HarbourSystemState.locked) {
            lockTimer.start()
        }
    }

    FoilPicsModel {
        id: appFoilModel

        thumbnailSize: Qt.size(Theme.itemSizeHuge,Theme.itemSizeHuge)
        onMediaDeleted: FoilPics.mediaDeleted(url)
        onFoilStateChanged: resetAutoLock()
    }

    FoilPicsHints {
        id: appHints
    }

    HarbourWakeupTimer {
        id: lockTimer

        interval: FoilPicsSettings.autoLockTime
        onTriggered: appFoilModel.lock(false);
    }

    Connections {
        target: HarbourSystemState
        onLockedChanged: resetAutoLock()
    }

    Connections {
        target: FoilPicsSettings
        onAutoLockChanged: resetAutoLock()
    }

    Component {
        id: encryptedPageComponent

        EncryptedPage {
            hints: appHints
            foilModel: appFoilModel
            allowedOrientations: app.allowedOrientations
            onFullScreenThumbnailChanged: app.cover.fullScreenThumbnail = fullScreenThumbnail
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

    cover: CoverPage {
        foilModel: appFoilModel
    }
}
