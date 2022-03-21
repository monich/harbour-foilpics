import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilpics 1.0

ThumbnailBase {
    id: thumbnailBase

    Thumbnail {
        id: thumbnail

        readonly property bool gridMoving: thumbnailBase.grid ? thumbnailBase.grid.moving : false

        source: thumbnailBase.source
        mimeType: thumbnailBase.mimeType
        width:  size
        height: size
        y: contentYOffset
        x: contentXOffset

        onGridMovingChanged: {
            if (!gridMoving) {
                var visibleIndex = Math.floor(thumbnailBase.grid.contentY / size) * thumbnailBase.grid.columnCount
                if (visibleIndex <= index && index <= visibleIndex + 18) {
                    thumbnail.setHighPriority()
                } else {
                    thumbnail.setLowPriority()
                }
            }
        }

        onThumbnailError: errorLabelComponent.createObject(thumbnail)
    }

    Component {
        id: errorLabelComponent
        Label {
            //: Thumbnail Image loading failed
            //% "Oops, can't display the thumbnail!"
            text: qsTrId("foilpics-thumbnail_image-loading_failed")
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.paddingMedium
            height: parent.height - 2 * Theme.paddingSmall
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: Theme.fontSizeSmall
            fontSizeMode: Text.Fit
        }
    }
}
