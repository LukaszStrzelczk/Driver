import QtQuick
import QtQuick.Layouts
import modules

Item{
    id: root


    width: 800
    height: 200
    RowLayout{
        spacing: 10
        anchors.centerIn: parent
        Wheel{
            id: wheel
            Layout.alignment: Qt.AlignVCetnter
        }
        Pedal{
            id: pedal
            Layout.alignment: Qt.AlignVCetnter
        }
    }
}
