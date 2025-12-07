import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

T.Dialog {
    id: root

    property string ip
    signal ipsConfirmed(string ip)

    modal: true
    anchors.centerIn: parent
    width: 450
    height: 240

    background: Rectangle {
        color: "#2a2a2a"
        border.color: "#444"
        border.width: 2
        radius: 8
    }

    header: Rectangle {
        id: headerBG
        color: "#2b2b2b"
        height: 50
        radius: 8
        border.color: "#444"
        border.width: 2

        Text {
            text: "Raspberry Pi Configuration"
            color: "white"
            font.pixelSize: 25
            font.bold: true
            anchors.centerIn: parent
        }
    }

    contentItem: Item {
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15

            Text {
                text: "Car IP address:"
                color: "#aaa"
                font.pixelSize: 16
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                color: "#1a1a1a"
                border.color: ipInput.activeFocus ? "lime" : "#333"
                border.width: 2
                radius: 5

                TextInput {
                    id: ipInput
                    anchors.fill: parent
                    anchors.margins: 12
                    color: "white"
                    font.pixelSize: 16
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    text: root.ip

                    validator: RegularExpressionValidator {
                        regularExpression: /^(?:[0-9]{1,3}\.){0,3}[0-9]{0,3}$/
                    }

                    onTextChanged: root.ip = text

                    Keys.onReturnPressed: confirmButton.clicked()
                    Keys.onEnterPressed: confirmButton.clicked()
                }

                Text {
                    visible: ipInput.text.length === 0
                    text: "e.g., 192.168.1.100"
                    color: "#555"
                    font.pixelSize: 16
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                }
            }

            Item {
                Layout.fillHeight: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Layout.alignment: Qt.AlignHCenter
                Button {
                    id: cancelButton
                    text: "Cancel"
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 35

                    contentItem: Text {
                        text: cancelButton.text
                        color: cancelButton.hovered ? "white" : "#aaa"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: cancelButton.pressed ? "#1a1a1a" : (cancelButton.hovered ? "#333" : "#2a2a2a")
                        border.color: "#444"
                        border.width: 1
                        radius: 5
                    }

                    onClicked: root.reject()
                }

                Button {
                    id: confirmButton
                    text: "Confirm"
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 35
                    enabled: ipInput.text.length > 0

                    contentItem: Text {
                        text: confirmButton.text
                        color: confirmButton.enabled ? (confirmButton.hovered ? "white" : "lime") : "#555"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: confirmButton.enabled ?
                               (confirmButton.pressed ? "#1a1a1a" : (confirmButton.hovered ? "#2a3d1a" : "#1a2a1a")) :
                               "#1a1a1a"
                        border.color: confirmButton.enabled ? "lime" : "#333"
                        border.width: 2
                        radius: 5
                    }

                    onClicked: {
                        root.ipsConfirmed(root.ip)
                        root.accept()
                    }
                }
            }
        }
    }
}
