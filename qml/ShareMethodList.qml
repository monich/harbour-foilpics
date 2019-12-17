import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.dbus 2.0
import harbour.foilpics 1.0

SilicaListView {
    id: view

    property url source
    readonly property bool accountIconSupported: model.accountIconSupported

    width: parent.width
    height: Theme.itemSizeSmall * model.count
    model: TransferMethodsModel

    DBusInterface {
        id: settings

        service: "com.jolla.settings"
        path: "/com/jolla/settings/ui"
        iface: "com.jolla.settings.ui"
    }

    delegate: BackgroundItem {
        id: backgroundItem
        width: view.width

        Image {
            id: icon

            x: Theme.horizontalPageMargin
            anchors.verticalCenter: parent.verticalCenter
            source: accountIcon ? accountIcon :
                view.accountIconSupported ? "image://theme/icon-m-share" : ""
            visible: view.accountIconSupported
        }

        Label {
            id: displayNameLabel

            text: displayName
            color: backgroundItem.highlighted ? Theme.highlightColor : Theme.primaryColor
            truncationMode: TruncationMode.Fade
            x: Theme.horizontalPageMargin
            anchors {
                left: icon.visible ? icon.right : parent.left
                leftMargin: icon.visible ? Theme.paddingMedium : Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            width: Math.min(implicitWidth, parent.width - 2*Theme.horizontalPageMargin)
        }

        Label {
            text: userName
            font.pixelSize: Theme.fontSizeMedium
            color: backgroundItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            truncationMode: TruncationMode.Fade
            anchors {
                left: displayNameLabel.right
                leftMargin: Theme.paddingSmall
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            visible: text.length > 0
        }

        onClicked: {
            pageStack.push(shareUIPath, {
                source: view.source,
                methodId: methodId,
                displayName: displayName,
                accountId: accountId,
                accountName: userName
            })
        }
    }

    footer: BackgroundItem {
        id: addItem

        Image {
            id: addAccountIcon

            x: Theme.horizontalPageMargin
            anchors.verticalCenter: parent.verticalCenter
            source: view.accountIconSupported ?
                ("image://theme/icon-m-add" + (addItem.highlighted ? "?" + Theme.highlightColor : "")) : ""
            visible: view.accountIconSupported
        }

        Label {
            //: Share list item
            //% "Add account"
            text: qsTrId("foilpics-share_method_list-add_account")
            x: Theme.horizontalPageMargin
            anchors {
                left: addAccountIcon.visible ? addAccountIcon.right : parent.left
                leftMargin: addAccountIcon.visible ? Theme.paddingMedium : Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            color: highlighted ? Theme.highlightColor : Theme.primaryColor
        }
        onClicked: settings.call("showAccounts", undefined)
    }
}
