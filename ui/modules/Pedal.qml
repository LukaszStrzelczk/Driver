import QtQuick
import utils

Item {
    id: root
    width: pedal.width
    height: pedal.height
    Image {
        id: pedal
        source: "resources/pedal.svg"
        fillMode: Image.PreserveAspectFit
        height: 100
        width: 100
    }
    ProgressBar{
        id: pB
        value: 0.5
        anchors.left: pedal.right
        anchors.leftMargin: 5
    }
}
