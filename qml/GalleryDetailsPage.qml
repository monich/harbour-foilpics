import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    property alias item: view.item

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            PageHeader {
                //: Page header
                //% "Details"
                title: qsTrId("foilpics-details-header")
            }

            GalleryDetailsView {
                id: view

                width: parent.width
            }
        }

        VerticalScrollDecorator { }
    }
}
