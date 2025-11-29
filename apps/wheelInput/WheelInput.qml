import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import modules
import driversrc

Item{
    id: root

    width: 900
    height: 280

    // Create a SteeringController instance
    SteeringController {
        id: steeringController

        Component.onCompleted: {
            // Auto-connect to first available device
            if (availableDevices.length > 0) {
                connectDevice(0)
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

        RowLayout{
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            Wheel{
                id: wheel
                Layout.alignment: Qt.AlignVCenter
                steeringValue: steeringController.steering
            }

            Pedal{
                id: throttlePedal
                Layout.alignment: Qt.AlignVCenter
                label: "Throttle"
                value: steeringController.throttle
            }

            Pedal{
                id: brakePedal
                Layout.alignment: Qt.AlignVCenter
                label: "Brake"
                value: steeringController.brake
            }

            // Connection status indicator
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 150
                Layout.preferredHeight: 120
                color: "#2a2a2a"
                border.color: "#444"
                border.width: 1
                radius: 5

                Column {
                    anchors.centerIn: parent
                    spacing: 8

                    Text {
                        text: "Steering Wheel"
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Rectangle {
                        width: 24
                        height: 24
                        radius: 12
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: steeringController.connected ? "lime" : "red"
                    }

                    Text {
                        text: steeringController.connected ? "Connected" : "Disconnected"
                        color: steeringController.connected ? "lime" : "orange"
                        font.pixelSize: 11
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: steeringController.connected ? steeringController.deviceName : "No device"
                        color: "#aaa"
                        font.pixelSize: 9
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 140
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        // Device connection panel
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 500
            height: 40
            color: "#1a1a1a"
            border.color: "#333"
            border.width: 1
            radius: 3

            Row {
                anchors.centerIn: parent
                spacing: 10

                Text {
                    text: steeringController.connected ?
                          ("Device: " + steeringController.deviceName) :
                          (steeringController.availableDevices.length > 0 ?
                           "Click 'Refresh' to connect" :
                           "No steering devices found")
                    color: steeringController.connected ? "lime" : "orange"
                    font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }

                Button {
                    text: "Refresh Devices"
                    onClicked: {
                        steeringController.refreshDevices()
                        if (steeringController.availableDevices.length > 0) {
                            steeringController.connectDevice(0)
                        }
                    }
                    height: 28
                }
            }
        }
    }
}
