import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

Page {
    id: page
    property var hints
    property var foilModel
    property var foilTransferMethodsModel

    readonly property var pages: [ encryptedViewComponent, galleryViewComponent ]
    property real contentOffsetX // x coordinate of the first delegate

    SystemState {
        id: systemState
        onLockModeChanged: {
            if (locked) {
                page.foilModel.lock(false);
            }
        }
    }

    Component {
        id: encryptedViewComponent
        EncryptedView {
            hints: page.hints
            foilModel: page.foilModel
            mainPage: page
            Connections {
                target: parent
                onXChanged: contentOffsetX = target.x
            }
        }
    }

    Component {
        id: galleryViewComponent
        GalleryView {
            hints: page.hints
            foilModel: page.foilModel
            transferMethodsModel: foilTransferMethodsModel
        }
    }

    SilicaListView {
        id: scroller
        spacing: Theme.paddingMedium
        cacheBuffer: Math.max(width + spacing, 1)
        anchors.fill: parent
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        interactive: !scrollAnimation.running
        highlightRangeMode: ListView.StrictlyEnforceRange
        model: pages
        delegate: Loader {
            readonly property bool isCurrentItem: index == scroller.currentIndex
            enabled: isCurrentItem
            width: scroller.width
            height: scroller.height
            sourceComponent: modelData
        }
        onCurrentIndexChanged: {
            if (currentIndex !== 0 && hints.leftSwipeToGallery < MaximumHintCount) {
                // No need for this hint
                hints.leftSwipeToGallery = MaximumHintCount
            }
        }
    }

    GlassItem {
        x: scroller.x + contentOffsetX- scroller.contentX  + scroller.width + Math.round((scroller.spacing - width)/2)
        y: scroller.y + Math.round(((page.isLandscape ? Theme.itemSizeSmall : Theme.itemSizeLarge) - height)/2)
        color: Theme.primaryColor
        radius: 0.22
        falloffRadius: 0.18
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (scroller.currentItem.height < Screen.height) {
                    // Need to hide the keyboard first, otherwise
                    // it's not going to look good
                    forceActiveFocus()
                } else {
                    scrollAnimation.from = scroller.contentX
                    scrollAnimation.to = scroller.currentIndex > 0 ? contentOffsetX :
                        contentOffsetX + scroller.width + scroller.spacing
                    scrollAnimation.start()
                }
            }
            NumberAnimation {
                id: scrollAnimation
                target: scroller
                property: "contentX"
                duration: 500
                easing.type: Easing.InOutQuad
            }
        }
    }

    Loader {
        id: leftSwipeToGalleryHintLoader
        anchors.fill: parent
        active: opacity > 0
        opacity: (hints.leftSwipeToGallery < MaximumHintCount | running) ? 1 : 0
        property bool running
        sourceComponent: Component {
            LeftRightSwipeHint {
                //: Left swipe hint text
                //% "Swipe left to access the picture gallery"
                text: qsTrId("foilpics-hint-swipe_left_to_gallery")
                property bool hintCanBeEnabled: scroller.currentIndex === 0 &&
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
