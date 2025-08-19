import QtQuick
import QtMultimedia

Item{
    id: root

    implicitWidth: 1400
    implicitHeight: 840

    Rectangle{
        id: bg

        anchors.fill: root
        color: "black"
    }

    MediaPlayer {
        id: mediaplayer
        source: // TO DO
        videoOutput: videooutput
        audioOutput: AudioOutput{}
    }

    VideoOutput{
        id: videooutput
        anchors.fill: parent
    }

}
