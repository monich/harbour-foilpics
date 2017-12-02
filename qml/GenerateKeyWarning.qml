import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dialog
    allowedOrientations: window.allowedOrientations

    DialogHeader { id: header }

    Column {
        id: column
        anchors{
            top: header.bottom
            topMargin: Theme.paddingLarge
            left: parent.left
            right: parent.right
        }
        spacing: Theme.paddingLarge

        InfoLabel {
            //: Title for the new key warning
            //% "Warning"
            text: qsTrId("foilpics-generate_key_warning-title")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            //: Warning shown prior to generating the new key
            //% "You seem to have some encrypted files in the storage folder. Once you have generated a new key, you are going to lose access to those files. If you have forgotten your password, then keep in mind that most likely it's computationally easier to brute-force your password and recover the old key than to crack the new key."
            text: qsTrId("foilpics-generate_key_warning-text")
            wrapMode: Text.Wrap
            color: Theme.highlightColor
        }
    }
}
