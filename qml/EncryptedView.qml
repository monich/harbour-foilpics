import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0
import org.nemomobile.notifications 1.0

Item {
    id: view
    property Page mainPage
    property var hints
    property var foilModel

    Connections {
        target: view.foilModel
        property int lastFoilState
        onFoilStateChanged: {
            // Don't let the progress screens disappear too fast
            switch (foilModel.foilState) {
            case FoilPicsModel.FoilGeneratingKey:
                generatingKeyTimer.start()
                break
            case FoilPicsModel.FoilDecrypting:
                decryptingTimer.start()
                break
            }
            if (lastFoilState === FoilPicsModel.FoilPicsReady &&
                    target.foilState !== FoilPicsModel.FoilPicsReady) {
                console.log("popping the stack")
                pageStack.pop(mainPage, true)
            }
            lastFoilState = target.foilState
        }
        onKeyGenerated: {
            //: Pop-up notification
            //% "Generated new key"
            notification.previewBody = qsTrId("foilpics-notification-generated_key")
            notification.publish()
        }
        onPasswordChanged: {
            //: Pop-up notification
            //% "Password changed"
            notification.previewBody = qsTrId("foilpics-notification-password_changed")
            notification.publish()
        }
    }

    Notification {
        id: notification
    }

    Timer {
        id: generatingKeyTimer
        interval: 1000
    }

    Timer {
        id: decryptingTimer
        interval: 1000
    }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        // GenerateKeyView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilKeyMissing) ? 1 : 0
            sourceComponent: Component { GenerateKeyView { foilModel: view.foilModel } }
            Behavior on opacity { FadeAnimation {} }
        }

        // GeneratingKeyView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilGeneratingKey ||
                        generatingKeyTimer.running) ? 1 : 0
            sourceComponent: Component { GeneratingKeyView {} }
            Behavior on opacity { FadeAnimation {} }
        }

        // EnterPasswordView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilLocked ||
                        foilModel.foilState === FoilPicsModel.FoilLockedTimedOut) ? 1 : 0
            sourceComponent: Component { EnterPasswordView { foilModel: view.foilModel } }
            Behavior on opacity { FadeAnimation {} }
        }

        // DecryptingView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilDecrypting ||
                      decryptingTimer.running) ? 1 : 0
            sourceComponent: Component { DecryptingView { foilModel: view.foilModel } }
            Behavior on opacity { FadeAnimation {} }
        }

        // EncryptedGrid
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilPicsReady &&
                      !generatingKeyTimer.running && !decryptingTimer.running) ? 1 : 0
            sourceComponent: Component {
                EncryptedGrid {
                    hints: view.hints
                    foilModel: view.foilModel
                    pulleyFlickable: flickable
                }
            }
            Behavior on opacity { FadeAnimation {} }
        }
    }
}
