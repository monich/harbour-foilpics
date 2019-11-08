import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0
import org.nemomobile.notifications 1.0

import "harbour"

Page {
    id: page
    property var hints
    property var foilModel

    // nextPage is either
    // a) our attached page; or
    // b) the page we pushed to the stack
    readonly property Page nextPage: pageStack.nextPage(page)
    readonly property bool isCurrentPage: status === PageStatus.Active || status === PageStatus.Activating ||
        (nextPage && page.parent && nextPage.parent !== page.parent.attachedContainer)

    Connections {
        target: page.foilModel
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
                pageStack.pop(page, true)
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
        contentHeight: height

        // GenerateKeyView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilKeyMissing) ? 1 : 0
            sourceComponent: Component {
                GenerateKeyView {
                    //: Prompt label
                    //% "You need to generate the key and select the password before you can encrypt your pictures"
                    prompt: qsTrId("foilpics-generate_key_view-label-key_needed")
                    foilModel: page.foilModel
                }
            }
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
            sourceComponent: Component { EnterPasswordView { foilModel: page.foilModel } }
            Behavior on opacity { FadeAnimation {} }
        }

        // DecryptingView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilDecrypting ||
                      decryptingTimer.running) ? 1 : 0
            sourceComponent: Component { DecryptingView { foilModel: page.foilModel } }
            Behavior on opacity { FadeAnimation {} }
        }

        // EncryptedPicsView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilPicsReady &&
                      !generatingKeyTimer.running && !decryptingTimer.running) ? 1 : 0
            readonly property bool isCurrentItem: page.isCurrentPage
            sourceComponent: Component {
                EncryptedPicsView {
                    hints: page.hints
                    foilModel: page.foilModel
                }
            }
            Behavior on opacity { FadeAnimation {} }
        }
    }

    Loader {
        id: leftSwipeToGalleryHintLoader
        anchors.fill: parent
        active: opacity > 0
        opacity: (hints.leftSwipeToGallery < MaximumHintCount | running) ? 1 : 0
        property bool running
        sourceComponent: Component {
            HarbourHorizontalSwipeHint {
                //: Left swipe hint text
                //% "Swipe left to access the picture gallery"
                text: qsTrId("foilpics-hint-swipe_left_to_gallery")
                property bool hintCanBeEnabled: page.isCurrentPage &&
                    foilModel.foilState === FoilPicsModel.FoilPicsReady &&
                    hints.leftSwipeToGallery < MaximumHintCount
                hintEnabled: hintCanBeEnabled && !hintDelayTimer.running
                onHintShown: hints.leftSwipeToGallery++
                onHintRunningChanged: leftSwipeToGalleryHintLoader.running = hintRunning
                onHintCanBeEnabledChanged: if (hintCanBeEnabled) hintDelayTimer.restart()
                Component.onCompleted: if (hintCanBeEnabled) hintDelayTimer.restart()
                Timer {
                    id: hintDelayTimer
                    interval: 1000
                }
            }
        }
        Behavior on opacity { FadeAnimation {} }
    }
}
