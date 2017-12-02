import QtQuick 2.0
import Sailfish.Silica 1.0

Label {
    property int minFontSize: Theme.fontSizeTiny
    property int maxFontSize: Theme.fontSizeMedium
    property int refitting

    smooth: true
    visible: opacity > 0
    color: Theme.highlightColor
    anchors.margins: Theme.paddingMedium
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter

    Behavior on opacity { FadeAnimation {} }

    Component.onCompleted: refitText()

    onWidthChanged: refitText()
    onHeightChanged: refitText()
    onTextChanged: refitText()
    onMaxFontSizeChanged: refitText()
    onMinFontSizeChanged: refitText()

    function refitText() {
        refitting++
        if (refitting == 1 && paintedHeight > 0 && paintedWidth > 0) {
            if (font.pixelSize % 2) font.pixelSize++
            while (paintedWidth > width || paintedHeight > height) {
                if ((font.pixelSize -= 2) <= minFontSize)
                    break
            }
            while (paintedWidth < width && paintedHeight < height) {
                font.pixelSize += 2
            }
            font.pixelSize -= 2
            if (font.pixelSize >= maxFontSize) {
                font.pixelSize = maxFontSize
                return
            }
        }
        refitting--
    }
}
