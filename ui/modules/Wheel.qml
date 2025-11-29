import QtQuick

Item{
    id: root
    property real steeringValue: 0.0  // -1.0 to 1.0 from SteeringController
    height: wheel.height
    width: wheel.width

    Image{
        id: wheel
        fillMode: Image.PreserveAspectFit
        anchors.centerIn: parent
        source: "resources/steeringWheel.svg"
        transform: Rotation {
            origin.x: wheel.width / 2
            origin.y: wheel.height / 2
            // Map steering value -1.0 to 1.0 to rotation angle -450 to 450 degrees
            angle: root.steeringValue * 450
        }
        smooth: true
        height: 100
        width: 100
    }

    // Debug text showing percentage
    Text {
        anchors.top: wheel.bottom
        anchors.topMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Steering: " + (root.steeringValue * 100).toFixed(0) + "%"
        color: "white"
        font.pixelSize: 12
    }
}
