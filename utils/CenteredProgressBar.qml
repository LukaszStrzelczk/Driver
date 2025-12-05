import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

T.Control{
    id: root

    implicitWidth: 100
    implicitHeight: 20

    property real value: 0.0
    property real from: -1.0
    property real to: 1.0

    property color bgColor: '#626363'
    property color centerLineColor: '#001e80'
    property color fillColor:'#803300'

    background: Rectangle{
        id: bg
        height: root.height - 4
        width: root.width
        radius: 5
        color: root.bgColor

    }

    contentItem: Item{
        Rectangle{
            id: content
            height: bg.height
            radius: 5
            color: root.fillColor

            readonly property real normalizedValue: Math.abs(root.value - 0) / (root.to - root.from)
            width: normalizedValue * (root.width)
            x: root.value >= 0
                ? root.width / 2
                : (root.width / 2) - width
            
             Behavior on width { 
                NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
            }
            Behavior on x { 
                NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
            }
        }
    }
}
