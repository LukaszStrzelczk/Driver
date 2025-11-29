import QtQuick
import QtQuick.Controls

Item{
    id: root
    property int steeringAngle: 0
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
            angle: steeringAngle
        }
        smooth: true
        height: 100
        width: 100
    }

}
