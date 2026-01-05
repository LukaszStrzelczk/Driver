import QtQuick
import QtQuick.Layouts

Item {
    id: root

    implicitWidth: 1400
    implicitHeight: 720

    Rectangle{
        anchors.fill: parent
        color: "black"
    }

    /* ========================================================================
     * GSTREAMER IMPLEMENTATION (ACTIVE)
     * ========================================================================
     * Uses GStreamer for low-latency RTP/MJPEG video streaming.
     * Receives video over UDP port 5000.
     */

    // GStreamer RTP/MJPEG Stream Display
    Image {
        id: videoImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        cache: false
        asynchronous: true
        // Update source when frame changes - the frame ID changes trigger QML to request new image
        source: videoReceiver ? "image://videostream/" + videoReceiver.currentFrame : ""

        // Auto-start stream on component load
        // IMPORTANT: Use a timer to delay startup until after event loop is running
        // Starting immediately in Component.onCompleted causes QTimer issues
        Component.onCompleted: {
            if (videoReceiver) {
                startupTimer.start()
            }
        }
    }

    // Monitor streaming state and frame updates
    Connections {
        target: videoReceiver

        // Update UI when streaming state changes
        function onStreamingChanged() {
            root.updateConnectionUI()
        }

        // Update UI when active stream state changes (frames being received or not)
        function onHasActiveStreamChanged() {
            root.updateConnectionUI()
        }

        // Handle errors
        function onErrorOccurred(message) {
            console.log("GStreamer Error:", message)
            errorText.text = message
            errorText.visible = true
            // Hide error after 5 seconds
            errorHideTimer.restart()
        }
    }

    // Helper function to update connection UI based on streaming and active stream state
    function updateConnectionUI() {
        if (videoReceiver.isStreaming && videoReceiver.hasActiveStream) {
            // Pipeline is running AND frames are being received
            noVideoText.visible = false
        } else if (videoReceiver.isStreaming && !videoReceiver.hasActiveStream) {
            // Pipeline is running but NO frames are being received
            noVideoText.visible = true
        } else {
            // Pipeline is not running
            noVideoText.visible = true
        }
    }

    // Timer to auto-hide errors
    Timer {
        id: errorHideTimer
        interval: 5000
        onTriggered: errorText.visible = false
    }

    // Timer to delay stream startup until event loop is running
    // This prevents crashes from QTimer creation before event loop starts
    Timer {
        id: startupTimer
        interval: 100  // Wait 100ms for event loop to be ready
        onTriggered: {
            console.log("Starting video stream on port 5000...")
            videoReceiver.startStream(5000)
        }
    }

    // Connection status indicator
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        width: 140
        height: 30
        color: "#2a2a2a"
        border.color: "#444"
        border.width: 1
        radius: 5
        opacity: 0.8

        RowLayout {
            anchors.centerIn: parent
            spacing: 10

            Rectangle {
                width: 24
                height: 24
                radius: 12
                color: {
                    if (videoReceiver && videoReceiver.isStreaming && videoReceiver.hasActiveStream) {
                        return "lime"
                    } else if (videoReceiver && videoReceiver.isStreaming && !videoReceiver.hasActiveStream) {
                        return "red"
                    } else {
                        return "gray"
                    }
                }
            }

            Text {
                text: {
                    if (videoReceiver && videoReceiver.isStreaming && videoReceiver.hasActiveStream) {
                        return "Connected"
                    } else if (videoReceiver && videoReceiver.isStreaming && !videoReceiver.hasActiveStream) {
                        return "Disconnected"
                    } else {
                        return "Not streaming"
                    }
                }
                color: {
                    if (videoReceiver && videoReceiver.isStreaming && videoReceiver.hasActiveStream) {
                        return "lime"
                    } else if (videoReceiver && videoReceiver.isStreaming && !videoReceiver.hasActiveStream) {
                        return "orange"
                    } else {
                        return "#aaa"
                    }
                }
                font.pixelSize: 11
            }
        }
    }

    // No connection message
    Text {
        id: noVideoText
        text: "No Video Stream\n\nWaiting for video on UDP port 5000..."
        color: "white"
        font.pixelSize: 32
        horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: parent
        visible: videoReceiver ? !videoReceiver.isStreaming : true
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

/* ========================================================================
 * VIDEO SENDER COMMAND REFERENCE
 * ========================================================================
 * Use these commands on the sending device (e.g., Raspberry Pi) to stream
 * video to this application.
 *
 * OPTION 1: Using ffmpeg (from webcam to RTP/MJPEG)
 * Replace 192.168.18.18 with the IP address of the computer running this app
 */
// sudo ffmpeg -f v4l2 -input_format mjpeg -video_size 1280x720 -framerate 30 -i /dev/video0 -c:v copy -f rtp rtp://192.168.18.18:5000

/* OPTION 2: Using GStreamer (test pattern)
 * gst-launch-1.0 videotestsrc ! jpegenc ! rtpjpegpay ! udpsink host=192.168.18.18 port=5000
 *
 * OPTION 3: Using GStreamer (from webcam)
 * gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,width=1280,height=720 ! jpegenc ! rtpjpegpay ! udpsink host=192.168.18.18 port=5000
 *
 * OPTION 4: Using GStreamer (from file)
 * gst-launch-1.0 filesrc location=video.mp4 ! decodebin ! videoconvert ! jpegenc ! rtpjpegpay ! udpsink host=192.168.18.18 port=5000
 *
 * OPTION 5: FFmpeg with buffering (RECOMMENDED for smooth playback, reduces flickering)
 * Adds buffering and packet timing parameters to smooth out frame delivery
 * Replace 192.168.18.18 with the IP address of the computer running this app
 */
// sudo ffmpeg -f v4l2 -input_format mjpeg -video_size 1280x720 -framerate 30 -i /dev/video0 \
//   -c:v copy \
//   -flush_packets 0 \
//   -max_delay 200000 \
//   -buffer_size 2000000 \
//   -f rtp rtp://192.168.18.18:5000
//  sudo ffmpeg -f v4l2 -input_format mjpeg -video_size 1280x720 -framerate 30 -i /dev/video0 -c:v copy -flush_packets 0 -max_delay 200000 -buffer_size 2000000 -f rtp rtp://192.168.18.18:5000

/* OPTION 6: FFmpeg with re-encoding and optimal quality (best for eliminating flickering)
 * Re-encodes video with consistent quality and frame timing
 * Uses more CPU than OPTION 5 but provides the smoothest playback
 * Best choice if you have CPU power and want zero flickering
 */
// sudo ffmpeg -f v4l2 -input_format mjpeg -video_size 1280x720 -framerate 30 -i /dev/video0 \
//   -c:v mjpeg \
//   -q:v 5 \
//   -huffman optimal \
//   -pkt_size 1400 \
//   -buffer_size 2000000 \
//   -f rtp rtp://192.168.18.18:5000
// sudo ffmpeg -f v4l2 -input_format mjpeg -video_size 1280x720 -framerate 30 -i /dev/video0 -c:v mjpeg -q:v 5 -huffman optimal -pkt_size 1400 -buffer_size 2000000 -f rtp rtp://192.168.18.18:5000
