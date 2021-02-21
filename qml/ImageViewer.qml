import QtQuick 2.0
import Sailfish.Silica 1.0

SilicaFlickable {
    id: flickable

    property bool scaled
    property bool menuOpen
    property bool viewMoving
    property bool isCurrentItem
    property alias source: photo.source
    property bool fitWidth

    property real _fittedScale: Math.min(maximumZoom, Math.min(width / implicitWidth,
                                                               height / implicitHeight))
    property real _menuOpenScale: Math.max(_viewOpenWidth / implicitWidth,
                                           _viewOpenHeight / implicitHeight)
    property real _scale

    property int orientation: 0

    // Calculate a default value which produces approximately same level of zoom
    // on devices with different screen resolutions.
    readonly property real maximumZoom: Math.max(Screen.width, Screen.height) / 200
    readonly property int _maximumZoomedWidth: _fullWidth * maximumZoom
    readonly property int _maximumZoomedHeight: _fullHeight * maximumZoom
    readonly property int _minimumZoomedWidth: implicitWidth * _fittedScale
    readonly property int _minimumZoomedHeight: implicitHeight * _fittedScale
    readonly property bool _zoomAllowed: !viewMoving && !menuOpen && _fittedScale !== maximumZoom && !_menuAnimating
    readonly property int _fullWidth: _transpose ? Math.max(photo.implicitHeight, largePhoto.implicitHeight)
                                        : Math.max(photo.implicitWidth, largePhoto.implicitWidth)
    readonly property int _fullHeight: _transpose ? Math.max(photo.implicitWidth, largePhoto.implicitWidth)
                                         : Math.max(photo.implicitHeight, largePhoto.implicitHeight)

    readonly property int _viewOrientation: fitWidth ? Orientation.Portrait : Orientation.Landscape
    readonly property int _splitScreenHeight: Math.max(Screen.height / 2, Screen.height - Screen.width)
    readonly property int _viewOpenWidth: _viewOrientation === Orientation.Portrait ? Screen.width : _splitScreenHeight
    readonly property int _viewOpenHeight: _viewOrientation === Orientation.Portrait ? _splitScreenHeight : Screen.width

    readonly property bool _transpose: (orientation % 180) != 0
    property bool _menuAnimating
    property bool _allowParentFlick
    property bool _wasCurrentItem

    signal clicked

    // Override SilicaFlickable's pressDelay because otherwise it will
    // block touch events going to PinchArea in certain cases.
    pressDelay: 0

    flickableDirection: Flickable.HorizontalAndVerticalFlick

    implicitWidth: _transpose ? photo.implicitHeight : photo.implicitWidth
    implicitHeight: _transpose ? photo.implicitWidth : photo.implicitHeight

    contentWidth: container.width
    contentHeight: container.height

    interactive: scaled && !_allowParentFlick

    // Only update the scale when width and height are properly set by Silica.
    // If we do it too early, then calculating a new _fittedScale goes wrong
    on_ViewOrientationChanged: {
        _updateScale()
    }

    onViewMovingChanged: {
        if (!viewMoving) _allowParentFlick = false
        _maybeResetScale()
    }

    onIsCurrentItemChanged: {
        if (isCurrentItem) {
            _wasCurrentItem = true
        } else {
            _maybeResetScale()
        }
    }

    function _maybeResetScale() {
        if (!viewMoving && !isCurrentItem && _wasCurrentItem) {
            _wasCurrentItem = false
            _resetScale()
        }
    }

    function _resetScale() {
        if (scaled) {
            _scale = _fittedScale
            scaled = false
        }
    }

    function _scaleImage(scale, center, prevCenter) {
        if (largePhoto.source != photo.source) {
            largePhoto.source = photo.source
        }

        var newWidth
        var newHeight
        var oldWidth = contentWidth
        var oldHeight = contentHeight

        if (fitWidth) {
            // Scale and bounds check the width, and then apply the same scale to height.
            newWidth = (flickable._transpose ? photo.height : photo.width) * scale
            if (newWidth <= flickable._minimumZoomedWidth) {
                _resetScale()
                return
            } else {
                newWidth = Math.min(newWidth, flickable._maximumZoomedWidth)
                _scale = newWidth / implicitWidth
                newHeight = _transpose ? photo.width : photo.height
            }
        } else {
            // Scale and bounds check the height, and then apply the same scale to width.
            newHeight = (flickable._transpose ? photo.width : photo.height) * scale
            if (newHeight <= flickable._minimumZoomedHeight) {
                _resetScale()
                return
            } else {
                newHeight = Math.min(newHeight, flickable._maximumZoomedHeight)
                _scale = newHeight / implicitHeight
                newWidth = _transpose ? photo.height : photo.width
            }
        }

        // move center
        contentX += prevCenter.x - center.x
        contentY += prevCenter.y - center.y

        // scale about center
        if (newWidth > flickable.width)
            contentX -= (oldWidth - newWidth)/(oldWidth/prevCenter.x)
        if (newHeight > flickable.height)
            contentY -= (oldHeight - newHeight)/(oldHeight/prevCenter.y)

        scaled = true
    }

    function _updateScale() {
        if (photo.status === Image.Ready) {
            state = menuOpen ? "menuOpen" :
                _viewOrientation === Orientation.Portrait ? "portrait" :
                _viewOrientation === Orientation.Landscape ? "landscape" :
                "fullscreen" // fallback
        }
    }

    children: ScrollDecorator {}

    PinchArea {
        id: container
        enabled: photo.status == Image.Ready
        width: Math.max(flickable.width, flickable._transpose ? photo.height : photo.width)
        height: Math.max(flickable.height, flickable._transpose ? photo.width : photo.height)

        onPinchStarted: if (flickable.menuOpen) flickable.clicked()
        onPinchUpdated: {
            if (flickable._zoomAllowed) {
                flickable._scaleImage(1.0 + pinch.scale - pinch.previousScale, pinch.center, pinch.previousCenter)
            }
        }
        onPinchFinished: flickable.returnToBounds()

        Image {
            id: photo

            property var errorLabel

            smooth: !(flickable.movingVertically || flickable.movingHorizontally)
            width: Math.ceil(implicitWidth * flickable._scale)
            height: Math.ceil(implicitHeight * flickable._scale)
            sourceSize.width: Screen.height
            fillMode:  Image.PreserveAspectFit
            asynchronous: true
            anchors.centerIn: parent
            cache: false

            horizontalAlignment: Image.Left
            verticalAlignment: Image.Top

            onStatusChanged: {
                if (status == Image.Ready) {
                    flickable._updateScale()
                }
                if (status == Image.Error) {
                   errorLabel = errorLabelComponent.createObject(photo)
                }
            }

            onSourceChanged: {
                if (errorLabel) {
                    errorLabel.destroy()
                }
                flickable.scaled = false
            }

            rotation: -flickable.orientation

            opacity: status == Image.Ready ? 1 : 0
            Behavior on opacity { FadeAnimation{} }
        }

        Image {
            id: largePhoto
            sourceSize {
                width: 3264
                height: 3264
            }
            cache: false
            asynchronous: true
            anchors.fill: photo
            rotation: -flickable.orientation
        }

        MouseArea {
            property real startX
            property real lastX
            anchors.fill: parent
            onPressed: {
                _allowParentFlick = false
                startX = lastX = mouse.x
            }
            onCanceled: {
                if (flickable.atXBeginning && lastX > startX) {
                    // Allow to flick left
                    _allowParentFlick = true
                } else if (flickable.atXEnd && lastX < startX) {
                    // Allow to flick right
                    _allowParentFlick = true
                }
            }
            onMouseXChanged: lastX = mouse.x
            onClicked: flickable.clicked()
        }
    }

    Timer {
        id: quickLoadTimer

        interval: 1000
        running: true
    }

    Loader {
        active: opacity > 0
        opacity: (!quickLoadTimer.running && photo.status === Image.Loading) ? 1 : 0
        anchors.centerIn: parent
        sourceComponent: Component {
            BusyIndicator {
                size: BusyIndicatorSize.Large
                running: true
            }
        }
        Behavior on opacity { FadeAnimation {} }
    }

    Component {
        id: errorLabelComponent
        Label {
            //: Thumbnail Image loading failed
            //% "Oops, can't display the thumbnail!"
            text: qsTrId("foilpics-thumbnail_image-loading_failed")
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.paddingMedium
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // Let the states handle switching between menu open and fullscreen states.
    // We need to extend fullscreen state with two different states: portrait and
    // landscape to make it actually reset the fitted scale via state changes when
    // the orientation changes. Ie. state change from "fullscreen" to "fullscreen"
    // doesn't reset the fitted scale.
    states: [
        State {
            name: "menuOpen"
            when: flickable.menuOpen && photo.status === Image.Ready
            PropertyChanges {
                target: flickable
                _scale: flickable._menuOpenScale
                scaled: false
                contentX: fitWidth ? (flickable.implicitWidth  * flickable._menuOpenScale - flickable._viewOpenWidth ) / (flickable._viewOrientation === Orientation.Portrait ? 2 : -2) : 0
                contentY: fitWidth ? 0 : (flickable.implicitHeight  * flickable._menuOpenScale - flickable._viewOpenHeight ) / (flickable._viewOrientation === Orientation.Portrait ? -2 : 2)
            }
        },
        State {
            name: "fullscreen"
            PropertyChanges {
                target: flickable
                _scale: flickable._fittedScale
                scaled: false
                contentX: 0
                contentY: 0
            }
        },
        State {
            when: !flickable.menuOpen && photo.status === Image.Ready && _viewOrientation === Orientation.Portrait
            name: "portrait"
            extend: "fullscreen"
        },
        State {
            when: !flickable.menuOpen && photo.status === Image.Ready && _viewOrientation === Orientation.Landscape
            name: "landscape"
            extend: "fullscreen"
        }
    ]

    transitions: [
        Transition {
            from: '*'
            to: 'menuOpen'
            SequentialAnimation {
                ScriptAction { script: flickable._menuAnimating = true }
                PropertyAnimation {
                    target: flickable
                    properties: "_scale,contentX,contentY"
                    duration: 300
                    easing.type: Easing.InOutCubic
                }
                ScriptAction { script: flickable._menuAnimating = false }
            }
        },
        Transition {
            from: 'menuOpen'
            to: '*'
            SequentialAnimation {
                ScriptAction { script: flickable._menuAnimating = true }
                PropertyAnimation {
                    target: flickable
                    properties: "_scale,contentX,contentY"
                    duration: 300
                    easing.type: Easing.InOutCubic
                }
                ScriptAction { script: flickable._menuAnimating = false }
            }
        }
    ]
}
