import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

SilicaFlickable {
    id: view
    property var foilModel
    property bool wrongPassword
    readonly property bool unlocking: foilModel.foilState !== FoilPicsModel.FoilLocked &&
                                    foilModel.foilState !== FoilPicsModel.FoilLockedTimedOut

    function enterPassword() {
        if (!foilModel.unlock(passphrase.text)) {
            wrongPassword = true
            wrongPasswordAnimation.start()
            passphrase.requestFocus()
        }
    }

    PullDownMenu {
        id: pullDownMenu
        MenuItem {
            //: Pulley menu item
            //% "Generate a new key"
            text: qsTrId("foilpics-pulley_menu-generate_key")
            onClicked: {
                if (foilModel.mayHaveEncryptedPictures) {
                    var warning = pageStack.push(Qt.resolvedUrl("GenerateKeyWarning.qml"));
                    warning.accepted.connect(function() {
                        // Replace the warning page with a slide. This may
                        // occasionally generate "Warning: cannot pop while
                        // transition is in progress" if the user taps the
                        // page stack indicator (as opposed to the Accept
                        // button) but this warning is fairly harmless:
                        //
                        // _dialogDone (Dialog.qml:124)
                        // on_NavigationChanged (Dialog.qml:177)
                        // navigateForward (PageStack.qml:291)
                        // onClicked (private/PageStackIndicator.qml:174)
                        //
                        warning.canNavigateForward = false
                        pageStack.replace(Qt.resolvedUrl("GenerateKeyPage.qml"), {
                            foilModel: foilModel
                        })
                    })
                } else {
                    pageStack.push(Qt.resolvedUrl("GenerateKeyPage.qml"), {
                        foilModel: foilModel
                    })
                }
            }
        }
    }

    Column {
        id: column
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.paddingLarge

        InfoLabel {
            height: implicitHeight
            //: Password prompt label
            //% "Secret pictures are locked. Please enter your password"
            text: qsTrId("foilpics-enter_password_view-label-enter_password")
        }
        HarbourPasswordInputField {
            id: passphrase
            enabled: !unlocking
            EnterKey.onClicked: view.enterPassword()
            onTextChanged: view.wrongPassword = false
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: unlocking ?
                //: Button label
                //% "Unlocking..."
                qsTrId("foilpics-enter_password_view-button-unlocking") :
                //: Button label
                //% "Unlock"
                qsTrId("foilpics-enter_password_view-button-unlock")
            enabled: passphrase.text.length > 0 && !unlocking && !wrongPasswordAnimation.running && !wrongPassword
            onClicked: view.enterPassword()
        }
    }

    HarbourShakeAnimation  {
        id: wrongPasswordAnimation
        target: column
    }
}
