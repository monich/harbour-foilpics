import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0
import org.nemomobile.notifications 1.0

import "foil-ui"
import "harbour"

Page {
    id: thisPage

    property var hints
    property var foilUi
    property var foilModel

    // nextPage is either
    // a) our attached page; or
    // b) the page we pushed to the stack
    readonly property Page nextPage: pageStack.nextPage(thisPage)
    readonly property bool isCurrentPage: status === PageStatus.Active || status === PageStatus.Activating ||
        (nextPage && parent && nextPage.parent !== parent.attachedContainer)
    readonly property real screenHeight: isPortrait ? Screen.height : Screen.width

    function getFoilUi() {
        if (!foilUi) {
            foilUi = foilUiComponent.createObject(thisPage)
        }
        return foilUi
    }

    Component {
        id: foilUiComponent

        QtObject {
            readonly property real opacityFaint: HarbourTheme.opacityFaint
            readonly property real opacityLow: HarbourTheme.opacityLow
            readonly property real opacityHigh: HarbourTheme.opacityHigh
            readonly property real opacityOverlay: HarbourTheme.opacityOverlay

            readonly property var settings: FoilPicsSettings
            readonly property bool otherFoilAppsInstalled: FoilPics.otherFoilAppsInstalled
            function isLockedState(foilState) {
                return foilState === FoilPicsModel.FoilLocked ||
                    foilState === FoilPicsModel.FoilLockedTimedOut
            }
            function isReadyState(foilState) {
                return foilState === FoilPicsModel.FoilPicsReady
            }
            function isGeneratingKeyState(foilState) {
                return foilState === FoilPicsModel.FoilGeneratingKey
            }
            function qsTrEnterPasswordViewMenuGenerateNewKey() {
                //: Pulley menu item
                //% "Generate a new key"
                return qsTrId("foilpics-pulley_menu-generate_key")
            }
            function qsTrEnterPasswordViewEnterPasswordLong() {
                //: Password prompt label (long)
                //% "Secret pictures are locked. Please enter your password"
                return qsTrId("foilpics-enter_password_view-label-enter_password_long")
            }
            function qsTrEnterPasswordViewEnterPasswordShort() {
                //: Password prompt label (short)
                //% "Please enter your password"
                return qsTrId("foilpics-enter_password_view-label-enter_password_short")
            }
            function qsTrEnterPasswordViewButtonUnlock() {
                //: Button label
                //% "Unlock"
                return qsTrId("foilpics-enter_password_view-button-unlock")
            }
            function qsTrEnterPasswordViewButtonUnlocking() {
                //: Button label
                //% "Unlocking..."
                return qsTrId("foilpics-enter_password_view-button-unlocking")
            }
            function qsTrAppsWarningText() {
                //: Warning text, small size label below the password prompt
                //% "Note that all Foil apps use the same encryption key and password."
                return qsTrId("foilpics-foil_apps_warning")
            }
            function qsTrGenerateKeyWarningTitle() {
                //: Title for the new key warning
                //% "Warning"
                return qsTrId("foilpics-generate_key_warning-title")
            }
            function qsTrGenerateKeyWarningText() {
                //: Warning shown prior to generating the new key
                //% "You seem to have some encrypted files in the storage folder. Once you have generated a new key, you are going to lose access to those files. If you have forgotten your password, then keep in mind that most likely it's computationally easier to brute-force your password and recover the old key than to crack the new key."
                return qsTrId("foilpics-generate_key_warning-text")
            }
            function qsTrGenerateNewKeyPrompt() {
                //: Prompt label
                //% "You are about to generate a new key"
                return qsTrId("foilpics-generate_key_page-title")
            }
            function qsTrGenerateKeySizeLabel() {
                //: Combo box label
                //% "Key size"
                return qsTrId("foilpics-generate_key_view-label-key_size")
            }
            function qsTrGenerateKeyPasswordDescription(minLen) {
                //: Password field label
                //% "Type at least %0 character(s)"
                return qsTrId("foilpics-generate_key_view-label-minimum_length",minLen).arg(minLen)
            }
            function qsTrGenerateKeyButtonGenerate() {
                //: Button label
                //% "Generate key"
                return qsTrId("foilpics-generate_key_view-button-generate_key")
            }
            function qsTrGenerateKeyButtonGenerating() {
                //: Button label
                //% "Generating..."
                return qsTrId("foilpics-generate_key_view-button-generating_key")
            }
            function qsTrConfirmPasswordPrompt() {
                //: Password confirmation label
                //% "Please type in your new password one more time"
                return qsTrId("foilpics-confirm_password_page-info_label")
            }
            function qsTrConfirmPasswordDescription() {
                //: Password confirmation description
                //% "Make sure you don't forget your password. It's impossible to either recover it or to access the encrypted pictures without knowing it. Better take it seriously."
                return qsTrId("foilpics-confirm_password_page-description")
            }
            function qsTrConfirmPasswordRepeatPlaceholder() {
                //: Placeholder for the password confirmation prompt
                //% "New password again"
                return qsTrId("foilpics-confirm_password_page-text_field_placeholder-new_password")
            }
            function qsTrConfirmPasswordRepeatLabel() {
                //: Label for the password confirmation prompt
                //% "New password"
                return qsTrId("foilpics-confirm_password_page-text_field_label-new_password")
            }
            function qsTrConfirmPasswordButton() {
                //: Button label (confirm password)
                //% "Confirm"
                return qsTrId("foilpics-confirm_password_page-button-confirm")
            }
        }
    }

    Connections {
        target: thisPage.foilModel
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
                pageStack.pop(thisPage, true)
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
                FoilUiGenerateKeyView {
                    //: Prompt label
                    //% "You need to generate the key and select the password before you can encrypt your pictures"
                    prompt: qsTrId("foilpics-generate_key_view-label-key_needed")
                    page: thisPage
                    foilUi: getFoilUi()
                    foilModel: thisPage.foilModel
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
            sourceComponent: Component {
                FoilUiGeneratingKeyView {
                    //: Progress view label
                    //% "Generating new key..."
                    text: qsTrId("foilpics-generating_key_view-generating_new_key")
                    isLandscape: thisPage.isLandscape
                }
            }
            Behavior on opacity { FadeAnimation {} }
        }

        // EnterPasswordView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilLocked ||
                        foilModel.foilState === FoilPicsModel.FoilLockedTimedOut) ? 1 : 0
            sourceComponent: Component {
                FoilUiEnterPasswordView {
                    foilUi: getFoilUi()
                    foilModel: thisPage.foilModel
                    page: thisPage
                    iconComponent: Component {
                        Rectangle {
                            id: circle

                            width: Theme.itemSizeHuge
                            height: width
                            color: Theme.rgba(Theme.primaryColor, HarbourTheme.opacityFaint * HarbourTheme.opacityLow)
                            radius: width/2

                            Image {
                                source: "images/lock.svg"
                                height: Math.floor(circle.height * 5 / 8)
                                sourceSize.height: height
                                anchors.centerIn: circle
                                visible: parent.opacity > 0
                            }
                        }
                    }
                }
            }
            Behavior on opacity { FadeAnimation {} }
        }

        // DecryptingView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilDecrypting ||
                      decryptingTimer.running) ? 1 : 0
            sourceComponent: Component { DecryptingView { foilModel: thisPage.foilModel } }
            Behavior on opacity { FadeAnimation {} }
        }

        // EncryptedPicsView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilPicsModel.FoilPicsReady &&
                      !generatingKeyTimer.running && !decryptingTimer.running) ? 1 : 0
            readonly property bool isCurrentItem: thisPage.isCurrentPage
            sourceComponent: Component {
                EncryptedPicsView {
                    mainPage: thisPage
                    hints: thisPage.hints
                    foilUi: getFoilUi()
                    foilModel: thisPage.foilModel
                }
            }
            Behavior on opacity { FadeAnimation {} }
        }
    }

    Loader {
        id: leftSwipeToGalleryHintLoader

        anchors.fill: parent
        active: opacity > 0
        opacity: (hints.leftSwipeToGallery < MaximumHintCount || running) ? 1 : 0
        property bool running
        sourceComponent: Component {
            HarbourHorizontalSwipeHint {
                //: Left swipe hint text
                //% "Swipe left to access the picture gallery"
                text: qsTrId("foilpics-hint-swipe_left_to_gallery")
                property bool hintCanBeEnabled: thisPage.isCurrentPage &&
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
