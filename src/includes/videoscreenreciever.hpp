/**
 * @file videoscreenreciever.hpp
 * @brief Header file for video streaming receiver using GStreamer
 *
 * This file defines classes for receiving and displaying video streams over RTP/UDP.
 * It uses GStreamer for video decoding and Qt for integration with QML UI.
 */

#ifndef VIDEOSTREAMRECEIVER_H
#define VIDEOSTREAMRECEIVER_H

// Qt includes for core functionality, images, and QML integration
#include <QObject>              // Base class for Qt objects with signal/slot support
#include <QImage>               // Qt image container for frame storage
#include <QQuickImageProvider>  // Interface for providing images to QML Image elements
#include <QTimer>               // Timer for polling GStreamer bus messages

// GStreamer includes for video pipeline management
#include <gst/gst.h>            // Core GStreamer functionality
#include <gst/app/gstappsink.h> // AppSink element for extracting frames from pipeline

// Forward declaration to allow VideoImageProvider to reference VideoStreamReceiver
// before its full definition (solves circular dependency)
class VideoStreamReceiver;

/**
 * @class VideoImageProvider
 * @brief QML image provider that serves video frames to QML Image components
 *
 * This class acts as a bridge between the video receiver and QML's Image element.
 * It implements QQuickImageProvider to supply frames on demand when QML requests them.
 * Uses composition pattern to avoid multiple inheritance diamond problem with QObject.
 */
class VideoImageProvider : public QQuickImageProvider
{
public:
    /**
     * @brief Constructs the image provider with a reference to the video receiver
     * @param receiver Pointer to the VideoStreamReceiver that holds the actual frames
     */
    explicit VideoImageProvider(VideoStreamReceiver *receiver);

    /**
     * @brief Called by QML when it needs to render an image
     * @param id Image identifier (unused in this implementation)
     * @param size Output parameter filled with the image dimensions
     * @param requestedSize Desired size (unused, returns original size)
     * @return QImage containing the current video frame
     */
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    VideoStreamReceiver *m_receiver; ///< Pointer to the receiver that owns the frame data
};

/**
 * @class VideoStreamReceiver
 * @brief Main class for receiving and decoding RTP video streams using GStreamer
 *
 * This class manages a GStreamer pipeline that receives MJPEG video over RTP/UDP,
 * decodes it, and makes frames available to QML. It exposes properties and methods
 * to QML for controlling the stream and monitoring its status.
 *
 * Pipeline structure:
 * udpsrc -> rtpjpegdepay -> jpegdec -> videoconvert -> appsink
 *
 * Features:
 * - Low-latency configuration (drops frames if processing is too slow)
 * - Automatic format conversion to RGB for Qt compatibility
 * - Error handling and status reporting
 * - Thread-safe callback mechanisms
 */
class VideoStreamReceiver : public QObject
{
    Q_OBJECT // Macro enabling Qt's meta-object system (signals/slots, properties, etc.)

    // QML-accessible properties with getters and change notification signals
    Q_PROPERTY(QString currentFrame READ currentFrame NOTIFY frameChanged)    ///< Frame ID for QML Image source updates
    Q_PROPERTY(bool isStreaming READ isStreaming NOTIFY streamingChanged)     ///< Streaming active status
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)               ///< Human-readable status message
    Q_PROPERTY(bool hasActiveStream READ hasActiveStream NOTIFY hasActiveStreamChanged)  ///< Whether frames are actively being received

public:
    /**
     * @brief Constructs the video receiver and initializes GStreamer
     * @param parent Parent QObject for memory management (follows Qt parent-child pattern)
     */
    explicit VideoStreamReceiver(QObject *parent = nullptr);

    /**
     * @brief Destructor - ensures stream is stopped and resources are cleaned up
     */
    ~VideoStreamReceiver();

    // Public getters for QML property access

    /**
     * @brief Returns the most recent decoded frame as a QImage
     * @return QImage containing the current video frame (used by VideoImageProvider)
     */
    QImage currentImage() const { return m_currentImage; }

    /**
     * @brief Returns a unique frame identifier that changes with each new frame
     * @return QString like "frame_42" that QML monitors to trigger image reloads
     *
     * QML uses this to detect when the image source has changed and needs re-rendering
     */
    QString currentFrame() const { return m_frameId; }

    /**
     * @brief Returns whether the stream is currently active
     * @return true if pipeline is playing, false otherwise
     */
    bool isStreaming() const { return m_isStreaming; }

    /**
     * @brief Returns whether frames are actively being received
     * @return true if frames have been received recently, false otherwise
     */
    bool hasActiveStream() const { return m_hasActiveStream; }

    /**
     * @brief Returns the current status message
     * @return Human-readable status string (e.g., "Ready", "Streaming on port 5000", "Error: ...")
     */
    QString status() const { return m_status; }

    // QML-invokable methods (callable from JavaScript in QML)

    /**
     * @brief Starts receiving and decoding video from the specified UDP port
     * @param port UDP port number to listen on (e.g., 5000)
     *
     * Creates a GStreamer pipeline configured for low-latency MJPEG/RTP reception.
     * Stops any existing stream before starting a new one.
     */
    Q_INVOKABLE void startStream(int port);

    /**
     * @brief Stops the video stream and releases all GStreamer resources
     *
     * Safe to call even if no stream is running.
     */
    Q_INVOKABLE void stopStream();

signals:
    // Qt signals emitted when properties change (for QML property bindings)

    /**
     * @brief Emitted when a new video frame has been decoded and is ready
     *
     * QML listens to this to know when to re-render the Image element
     */
    void frameChanged();

    /**
     * @brief Emitted when the streaming state changes (started/stopped)
     */
    void streamingChanged();

    /**
     * @brief Emitted when the status message changes
     */
    void statusChanged();

    /**
     * @brief Emitted when the active stream state changes (receiving frames or not)
     */
    void hasActiveStreamChanged();

    /**
     * @brief Emitted when an error occurs during streaming
     * @param message Descriptive error message
     */
    void errorOccurred(const QString &message);

private:
    // Static callback functions for GStreamer (must be static to match C API)

    /**
     * @brief GStreamer callback invoked when a new decoded frame is available
     * @param appsink The appsink element that received the frame
     * @param user_data Pointer to the VideoStreamReceiver instance (set during initialization)
     * @return GST_FLOW_OK to continue processing, or error code to stop
     *
     * This is called from GStreamer's internal thread, so it must be thread-safe.
     * It extracts the frame and delegates to processNewSample() for actual processing.
     */
    static GstFlowReturn onNewSample(GstAppSink *appsink, gpointer user_data);

    /**
     * @brief GStreamer bus callback for receiving pipeline messages (errors, warnings, state changes)
     * @param bus The message bus
     * @param message The message to process
     * @param user_data Pointer to the VideoStreamReceiver instance
     * @return TRUE to keep receiving messages, FALSE to stop
     *
     * Handles asynchronous events from the pipeline and delegates to handleBusMessage()
     */
    static gboolean busCallback(GstBus *bus, GstMessage *message, gpointer user_data);

    // Private helper methods for internal processing

    /**
     * @brief Processes a new video sample from GStreamer
     * @param sample GStreamer sample containing the video frame and metadata
     *
     * Extracts the raw image data, converts it to QImage, and updates the frame counter.
     * Called from onNewSample() callback.
     */
    void processNewSample(GstSample *sample);

    /**
     * @brief Handles messages from the GStreamer bus (errors, warnings, state changes)
     * @param message The GStreamer message to process
     *
     * Called from busCallback() to handle pipeline events like errors and state transitions.
     */
    void handleBusMessage(GstMessage *message);

    /**
     * @brief Qt slot to check for GStreamer bus messages
     *
     * Called periodically by QTimer to poll the bus for messages.
     * This avoids using GLib's main loop which conflicts with Qt.
     */
    void checkBusMessages();

    /**
     * @brief Updates the status message and emits statusChanged signal if changed
     * @param status New status message
     */
    void setStatus(const QString &status);

    /**
     * @brief Updates the streaming state and emits streamingChanged signal if changed
     * @param streaming New streaming state
     */
    void setStreaming(bool streaming);

    /**
     * @brief Updates the active stream state and emits hasActiveStreamChanged signal if changed
     * @param hasActiveStream New active stream state
     */
    void setHasActiveStream(bool hasActiveStream);

    /**
     * @brief Qt slot to check if frames are still being received
     *
     * Called periodically by QTimer to detect stream timeout (no frames received).
     */
    void checkFrameTimeout();

    // Member variables (all prefixed with m_ following Qt convention)

    GstElement *m_pipeline;  ///< GStreamer pipeline - the complete processing chain from UDP to decoded frames
    GstElement *m_appsink;   ///< AppSink element - extracts frames from pipeline for application use
    GstBus *m_bus;           ///< Message bus - receives asynchronous events from the pipeline
    QImage m_currentImage;   ///< Most recently decoded frame stored as QImage (RGB format)
    QString m_frameId;       ///< Unique identifier for current frame (e.g., "frame_123")
    QString m_status;        ///< Current status message displayed to user
    int m_frameCounter;      ///< Incrementing counter used to generate unique frame IDs
    bool m_isStreaming;      ///< Flag indicating whether pipeline is currently playing
    bool m_hasActiveStream;  ///< Flag indicating whether frames are actively being received
    QTimer *m_busTimer;      ///< Timer to poll GStreamer bus for messages (avoids GLib main loop)
    QTimer *m_frameTimeoutTimer;  ///< Timer to detect when no frames are being received
    qint64 m_lastFrameTime;  ///< Timestamp of last received frame (milliseconds since epoch)
};

#endif // VIDEOSTREAMRECEIVER_H