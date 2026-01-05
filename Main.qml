import QtQuick
import QtQuick.Controls
import video
import wheel
import utils

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
    IpPopUp {
        id: popUp

        onIpsConfirmed: function(ip) {
            console.log("IP:", ip)

            if (ip.length>0) {
                steeringControllerService.connectToServer("ws://" + ip + ":8765")
            }
        }

        onRejected: {
            console.warn("User cancelled IP input")
        }
    }

    // Settings button to reopen IP dialog
    Button {
        id: settingsButton
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        width: 50
        height: 50

        contentItem: Text {
            text: "⚙"
            color: settingsButton.hovered ? "lime" : "white"
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: settingsButton.pressed ? "#1a1a1a" : (settingsButton.hovered ? "#333" : "#2a2a2a")
            border.color: settingsButton.hovered ? "lime" : "#444"
            border.width: 2
            radius: 25
        }

        onClicked: popUp.open()
    }

    Component.onCompleted: {
        popUp.open()
    }
}
