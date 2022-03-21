import QtQuick 2.0
import harbour.foilpics 1.0

Item {
    id: thisItem

    property string mimeType
    property int priority
    property url source

    signal thumbnailError()

    readonly property var thumbnail: Qt.createQmlObject(FoilPics.thumbnailQml, thisItem, "Thumbnail")

    function setLowPriority() {
        thumbnail.priority = thumbnail.lowPriority
    }

    function setHighPriority() {
        thumbnail.priority = thumbnail.highPriority
    }

    Binding { target: thumbnail; property: "sourceSize"; value: Qt.size(width,height) }
    Binding { target: thumbnail; property: "mimeType"; value: mimeType }
    Binding { target: thumbnail; property: "source"; value: source }

    Connections {
        target: thumbnail
        ignoreUnknownSignals: true
        onStatusChanged: {
            if (thumbnail.status === thumbnail.errorStatus) {
                thisItem.thumbnailError()
            }
        }
    }
}
