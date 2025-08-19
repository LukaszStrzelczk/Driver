import QtQuick
import video

Window {
    width: 1080
    height: 1920
    visible: true
    title: qsTr("Driver App")

    VideoScreen{
        id: video
        anchors.centerIn: parent

        Component.onCompleted: console.log("done")
    }

}
