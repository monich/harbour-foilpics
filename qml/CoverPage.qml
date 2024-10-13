import QtQuick 2.0
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

import "harbour"

CoverBackground {
    id: cover

    property var foilModel
    property string fullScreenThumbnail

    readonly property bool _darkOnLight: ('colorScheme' in Theme) && Theme.colorScheme === 1
    readonly property bool _jailed: HarbourProcessState.jailedApp
    property string _effectiveFullScreenThumbnail
    property bool _locking

    onFullScreenThumbnailChanged: {
        // Keep showing cached image during lock animation
        if (!_locking) {
            _effectiveFullScreenThumbnail = fullScreenThumbnail
        }
    }

    Label {
        id: title

        y: Theme.paddingLarge
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        //: Application title
        //% "Foil Pics"
        text: qsTrId("foilpics-app_name")
    }

    Rectangle {
        anchors.centerIn: flipable
        width: flipable.circleSize + 2 * Theme.paddingSmall
        height: width
        color: Theme.highlightBackgroundColor
        radius: width/2
        opacity: 0.2
    }

    Flipable {
        id: flipable

        readonly property real circleSize: Math.floor(parent.width * 0.8)
        readonly property bool flipped: cover.foilModel.keyAvailable

        width: parent.width
        anchors {
            top: title.bottom
            bottom: parent.bottom
            bottomMargin: Theme.itemSizeSmall
        }

        front: Item {
            anchors.fill: parent

            Rectangle {
                anchors.centerIn: parent
                width: flipable.circleSize
                height: width
                color: "white"
                radius: width/2
                opacity: 0.2
            }

            Image {
                source: "images/lock.svg"
                sourceSize.height: galleryImage.height
                anchors.centerIn: parent
            }
        }

        back: Item {
            anchors.fill: parent

            Rectangle {
                id: imageMask

                anchors.centerIn: parent
                width: flipable.circleSize
                height: width
                color: "white"
                radius: width/2
                opacity: 0.2
            }

            Image {
                visible: !!_effectiveFullScreenThumbnail
                source: _effectiveFullScreenThumbnail
                anchors.fill: imageMask
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: imageMask
                }
            }

            Image {
                id: galleryImage

                visible: !_effectiveFullScreenThumbnail
                source: "images/gallery.svg"
                sourceSize.width: Math.floor(cover.width/2)
                anchors.centerIn: parent
            }
        }

        transform: Rotation {
            id: rotation

            origin {
                x: flipable.width/2
                y: flipable.height/2
            }
            axis {
                x: 0
                y: 1
                z: 0
            }
        }

        states: [
            State {
                name: "front"
                when: !flipable.flipped
                PropertyChanges {
                    target: rotation
                    angle: 0
                }
            },
            State {
                name: "back"
                when: flipable.flipped
                PropertyChanges {
                    target: rotation
                    angle: 180
                }
            }
        ]

        transitions: Transition {
            to: "front"
            SequentialAnimation {
                alwaysRunToEnd: true
                NumberAnimation {
                    target: rotation
                    property: "angle"
                    duration: 500
                }
                ScriptAction { script: cover._effectiveFullScreenThumbnail = "" }
            }
        }
    }

    HarbourHighlightIcon {
        anchors.centerIn: cover
        sourceSize.height: flipable.circleSize + 2 * Theme.paddingMedium
        highlightColor: Theme.secondaryColor
        visible: _jailed
        source: _jailed ? "images/jail.svg" : ""
    }

    CoverActionList {
        enabled: cover.foilModel.keyAvailable
        CoverAction {
            iconSource: Qt.resolvedUrl("images/" + (_darkOnLight ? "cover-lock-dark.svg" :  "cover-lock.svg"))
            onTriggered: {
                _locking = true
                cover.foilModel.lock(false)
                _locking = false
            }
        }
    }
}
