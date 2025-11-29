import QtQuick
import QtMultimedia

Item{
    id: root

    implicitWidth: 1400
    implicitHeight: 720

    Rectangle{
        id: bg

        anchors.fill: root
        color: "black"
    }

    Video {
        id: cameraFeed
        anchors.fill: parent
        source: "rtsp://127.0.0.1:8554/cam?rtsp_transport=udp"
        autoPlay: true
        muted: true
        fillMode: VideoOutput.PreserveAspectFit
        // playbackRate: 0
        onErrorOccurred: (error, errorString) => {
            console.log("Error:", errorString)
            noVideoText.visible = true
        }

        onPlaying: {
            noVideoText.visible = false
        }
    }

    Text {
        id: noVideoText
        text: "No Connection"
        color: "white"
        font.pixelSize: 48
        anchors.centerIn: parent
        visible: true
    }
}
