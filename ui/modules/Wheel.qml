import QtQuick
import utils

Item{
    id: root
    property real steeringValue: 0.0  // -1.0 to 1.0 from SteeringController
    height: wheel.height + bar.height + bar.anchors.bottomMargin
    width: wheel.width

    CenteredProgressBar{
        id: bar
        value: root.steeringValue
        anchors.top: root.top
        anchors.bottomMargin: 10
    }

    Image{
        id: wheel
        fillMode: Image.PreserveAspectFit
        anchors.top: bar.bottom 
        anchors.horizontalCenter: parent.horizontalCenter
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
}
