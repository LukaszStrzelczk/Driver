import QtQuick
import utils

Item {
    id: root

    // External properties
    property string label: "Pedal"
    property real value: 0.0  // 0.0 to 1.0

    width: pedal.width + pB.width + 10
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
        value: root.value
        orientation: Qt.Vertical
        anchors.left: pedal.right
        anchors.leftMargin: 5
        anchors.top: pedal.top
        implicitHeight: 100
        implicitWidth: 30
    }

    // Label
    Text {
        anchors.top: pedal.bottom
        anchors.topMargin: 5
        anchors.horizontalCenter: pedal.horizontalCenter
        text: root.label + ": " + (root.value * 100).toFixed(0) + "%"
        color: "white"
        font.pixelSize: 12
    }
}
