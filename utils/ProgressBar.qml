import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Templates 2.15 as T

T.ProgressBar {
    id: root

    property int orientation: Qt.Vertical
    readonly property bool vertical: orientation == Qt.Vertical

    implicitWidth: bg.implicitWidth
    implicitHeight: bg.implicitHeight

    background: Rectangle{
        id: bg
        radius: 5
        implicitHeight: vertical ? 100 : 30
        implicitWidth: vertical ? 30 : 100
        color: '#626363'
    }

    contentItem: Item {
        Rectangle {
            id: content
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            height: vertical ? parent.height * root.position : parent.height
            width: !vertical ? parent.width * root.position : parent.width
            color: '#803300'
            radius: 5

            Component.onCompleted: console.log("content height:", height)
        }
    }
}
