import QtQuick
import video
import wheel

Window {
    width: 1920
    height: 1080
    visible: true
    title: qsTr("Driver App")

    Image {
        id: bg
        source: "resources/bg.jpg"
        anchors.fill: parent
    }

    VideoScreen{
        id: video
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter

        Component.onCompleted: console.log("done")
    }

    WheelInput{
        id: wheelInputs
        anchors.top: video.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
    }

}
