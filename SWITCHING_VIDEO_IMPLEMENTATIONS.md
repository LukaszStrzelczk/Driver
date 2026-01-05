# Quick Guide: Switching Video Implementations

This guide shows how to quickly switch between the GStreamer and HTTP MJPEG implementations.

## Current Setup (GStreamer RTP/UDP)

**Active:** ✅ GStreamer implementation
**Inactive:** 📦 HTTP MJPEG implementation (commented out)

---

## Option 1: Use GStreamer (Current - Default)

**What you get:**
- Low latency (~50-100ms)
- RTP/UDP streaming on port 5000
- Hardware-accelerated decoding

**No changes needed** - this is already active!

**Test sender:**
```bash
gst-launch-1.0 videotestsrc ! jpegenc ! rtpjpegpay ! udpsink host=127.0.0.1 port=5000
```

---

## Option 2: Switch to HTTP MJPEG

**What you get:**
- HTTP-based streaming
- TCP reliability
- Works with existing mjpg-streamer setups

### Steps to Switch:

#### 1. Edit `apps/video/VideoScreen.qml`

**Comment out** GStreamer implementation (lines 14-76):
```qml
/*
// GStreamer RTP/MJPEG Stream Display
Image {
    id: videoImage
    ...
}

Timer {
    id: errorHideTimer
    ...
}
*/
```

**Uncomment** HTTP MJPEG implementation (lines 85-129):
```qml
// Remove the /* and */ around this block:

// MJPEG Stream Display
Image {
    id: mjpegImage
    anchors.fill: parent
    fillMode: Image.PreserveAspectFit
    cache: false
    asynchronous: true
    source: "image://mjpeg/stream"

    Connections {
        target: mjpegDecoder
        ...
    }
}
```

#### 2. Update status indicator bindings

Change the connection status Rectangle to use mjpegDecoder:
```qml
Text {
    id: connectionStatus
    anchors.centerIn: parent
    // OLD: text: videoReceiver ? videoReceiver.status : "Not initialized"
    // OLD: color: videoReceiver && videoReceiver.isStreaming ? "lime" : "gray"

    // NEW:
    text: "Disconnected"
    color: "red"
    font.pixelSize: 12
    font.bold: true
}
```

And update noVideoText:
```qml
Text {
    id: noVideoText
    // OLD: visible: videoReceiver ? !videoReceiver.isStreaming : true

    // NEW:
    visible: true
    text: "No Video Connection\n\nConfigure IP in settings"
    ...
}
```

#### 3. Add URL configuration (if needed)

In your settings or initialization code:
```qml
Component.onCompleted: {
    mjpegDecoder.setUrl("http://192.168.1.100:8080/?action=stream")
    mjpegDecoder.start()
}
```

#### 4. Rebuild
```bash
cmake --build build
```

### Test HTTP MJPEG:

Start an HTTP MJPEG server:
```bash
# Using mjpg-streamer
mjpg_streamer -i "input_uvc.so" -o "output_http.so -w ./www -p 8080"

# Or using ffmpeg
ffmpeg -f lavfi -i testsrc -f mpjpeg http://localhost:8080/stream.mjpg
```

---

## Option 3: Run Both in Parallel (Advanced)

You can keep both implementations active and switch via a settings toggle:

```qml
property bool useGStreamer: true  // Toggle this

Image {
    visible: useGStreamer
    source: "image://videostream/" + videoReceiver.currentFrame
}

Image {
    visible: !useGStreamer
    source: "image://mjpeg/stream?t=" + timestamp
}
```

Both are already registered in `main.cpp`, so no C++ changes needed!

---

## Comparison Table

| Feature | GStreamer (Current) | HTTP MJPEG (Old) |
|---------|-------------------|------------------|
| **Latency** | ~50-100ms | ~200-500ms |
| **Protocol** | RTP/UDP | HTTP/TCP |
| **Port** | 5000 (UDP) | 8080 (HTTP) |
| **Setup** | Direct streaming | Needs HTTP server |
| **Active File** | `VideoScreen.qml` lines 14-76 | Lines 85-129 (commented) |
| **QML Property** | `videoReceiver` | `mjpegDecoder` |
| **Image Provider** | `image://videostream/` | `image://mjpeg/` |

---

## Troubleshooting After Switch

### Switched to HTTP but see "No Video"
- Check HTTP server is running: `curl http://localhost:8080/?action=stream`
- Verify URL is correct in `mjpegDecoder.setUrl()`
- Check network connectivity

### Switched to GStreamer but see "No Video"
- Verify sender is transmitting to correct IP and port
- Use Wireshark to check UDP packets on port 5000
- Check firewall isn't blocking UDP

### Build errors after editing QML
- QML syntax errors - check you closed all comment blocks properly
- Missing braces or parentheses

---

## Quick Test Commands

### Test GStreamer receiver (port 5000):
```bash
# Sender
gst-launch-1.0 videotestsrc pattern=smpte ! \
    jpegenc ! rtpjpegpay ! \
    udpsink host=127.0.0.1 port=5000

# Check if receiving
netstat -an | grep 5000
```

### Test HTTP MJPEG (port 8080):
```bash
# Start server
mjpg_streamer -i "input_uvc.so" -o "output_http.so -p 8080"

# Check if serving
curl -I http://localhost:8080/?action=stream
```

---

## Rollback Safety

Both implementations are **always present** in the codebase:

- **Code files**: Both `.cpp/.hpp` files are compiled
- **QML**: One active, one commented
- **main.cpp**: Both registered in QML context

This means you can switch back and forth anytime with just QML edits!

---

## Need Help?

See `VIDEO_IMPLEMENTATION.md` for detailed architecture documentation.
