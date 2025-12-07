import QtQuick
import QtMultimedia

Item {
    id: root

    implicitWidth: 1400
    implicitHeight: 720

    Rectangle {
        id: bg
        anchors.fill: root
        color: "black"
    }

    // MJPEG Stream Display
    Image {
        id: mjpegImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        cache: false
        asynchronous: true
        source: "image://mjpeg/stream"

        // Force refresh when new frame arrives
        Connections {
            target: mjpegDecoder
            function onImageUpdated() {
                // Update the image by changing the source slightly
                var timestamp = Date.now()
                mjpegImage.source = "image://mjpeg/stream?t=" + timestamp
            }

            function onConnectedChanged() {
                if (mjpegDecoder.connected) {
                    noVideoText.visible = false
                    connectionStatus.color = "lime"
                    connectionStatus.text = "Connected (" + mjpegDecoder.fps + " FPS)"
                } else {
                    noVideoText.visible = true
                    connectionStatus.color = "red"
                    connectionStatus.text = "Disconnected"
                }
            }

            function onFpsChanged() {
                if (mjpegDecoder.connected) {
                    connectionStatus.text = "Connected (" + mjpegDecoder.fps + " FPS)"
                }
            }

            function onErrorOccurred(error) {
                console.log("MJPEG Error:", error)
                errorText.text = error
                errorText.visible = true
            }
        }
    }

    // Connection status indicator
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        width: connectionStatus.width + 20
        height: 30
        color: "#2a2a2a"
        border.color: "#444"
        border.width: 1
        radius: 5
        opacity: 0.8

        Text {
            id: connectionStatus
            anchors.centerIn: parent
            text: "Disconnected"
            color: "red"
            font.pixelSize: 12
            font.bold: true
        }
    }

    // No connection message
    Text {
        id: noVideoText
        text: "No Video Connection\n\nConfigure IP in settings"
        color: "white"
        font.pixelSize: 32
        horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: parent
        visible: true
    }

    // Error message
    Text {
        id: errorText
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 10
        text: ""
        color: "orange"
        font.pixelSize: 12
        visible: false
    }
}
