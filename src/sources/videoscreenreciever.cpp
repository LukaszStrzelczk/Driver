/**
 * @file videoscreenreciever.cpp
 * @brief Implementation of video streaming receiver using GStreamer
 *
 * This file implements the video reception and decoding functionality defined in
 * videoscreenreciever.hpp. It handles GStreamer pipeline creation, frame extraction,
 * and integration with Qt's signal/slot system.
 */

#include "includes/videoscreenreciever.hpp"
#include <QDebug>              // Qt logging for debugging output
#include <QDateTime>           // Qt date/time utilities for frame timeout tracking
#include <gst/video/video.h>   // GStreamer video utilities for format info and conversions

/* ============================================================================
 * VideoImageProvider Implementation
 * ============================================================================
 * This class serves as a simple adapter between VideoStreamReceiver and QML's
 * Image element. QML calls requestImage() whenever it needs to display a frame.
 */

/**
 * @brief Constructor - stores reference to the receiver that owns the frame data
 * @param receiver Pointer to VideoStreamReceiver (must outlive this provider)
 */
VideoImageProvider::VideoImageProvider(VideoStreamReceiver *receiver)
    : QQuickImageProvider(QQuickImageProvider::Image)  // Initialize base class for Image type (not Pixmap or Texture)
    , m_receiver(receiver)  // Store pointer to access currentImage() later
{
    // No additional initialization needed - provider is stateless and just delegates
}

/**
 * @brief Provides the current video frame to QML when Image element requests it
 * @param id Image source identifier from QML (e.g., "image://video/frame_42")
 * @param size Output parameter - filled with actual image dimensions if non-null
 * @param requestedSize Desired size from QML (ignored - we return original size)
 * @return QImage containing the current frame (may be empty if no frames received yet)
 *
 * This is called from QML's rendering thread whenever the Image source changes.
 * QML typically requests with a URL like "image://video/" + frameId
 */
QImage VideoImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id)             // We don't use the ID - always return the latest frame
    Q_UNUSED(requestedSize)  // We don't scale - return original resolution

    // Get the current frame from the receiver
    QImage image = m_receiver->currentImage();

    // If caller wants to know the size, fill it in
    if (size) {
        *size = image.size();  // Returns QSize(0,0) if image is null/empty
    }

    return image;  // QImage is implicitly shared (copy-on-write), so this is efficient
}

/* ============================================================================
 * VideoStreamReceiver Implementation
 * ============================================================================
 * Main class that manages the GStreamer pipeline and frame processing.
 */

/**
 * @brief Constructor - initializes GStreamer and sets up initial state
 * @param parent Parent QObject for Qt's memory management system
 *
 * Qt's parent-child system ensures this object is deleted when parent is deleted.
 * GStreamer is initialized here once for the lifetime of the receiver.
 */
VideoStreamReceiver::VideoStreamReceiver(QObject *parent)
    : QObject(parent)           // Initialize QObject base class with parent
    , m_pipeline(nullptr)       // No pipeline yet - will be created in startStream()
    , m_appsink(nullptr)        // No appsink element yet
    , m_bus(nullptr)            // No message bus yet
    , m_frameCounter(0)         // Start frame numbering at 0
    , m_isStreaming(false)      // Not streaming initially
    , m_hasActiveStream(false)  // No active stream initially
    , m_busTimer(nullptr)       // Bus polling timer - created when pipeline starts
    , m_frameTimeoutTimer(nullptr)  // Frame timeout timer - created when pipeline starts
    , m_lastFrameTime(0)        // No frames received yet
{
    qDebug() << "[VideoStreamReceiver] Constructor started";

    // Initialize GStreamer library (safe to call multiple times - only initializes once)
    // nullptr arguments mean no command-line args to parse
    qDebug() << "[VideoStreamReceiver] Initializing GStreamer...";
    gst_init(nullptr, nullptr);
    qDebug() << "[VideoStreamReceiver] GStreamer initialized successfully";

    // Initialize with a placeholder frame ID to avoid QML warnings
    // QML will try to load this immediately but won't find a valid image
    m_frameId = "placeholder";

    // Create a placeholder black image to avoid errors when QML first requests an image
    m_currentImage = QImage(640, 480, QImage::Format_RGB888);
    m_currentImage.fill(Qt::black);
    qDebug() << "[VideoStreamReceiver] Placeholder image created";

    // Set initial status message for UI
    setStatus("Ready");

    // Log GStreamer version for debugging/verification
    qDebug() << "[VideoStreamReceiver] GStreamer version:" << gst_version_string();
    qDebug() << "[VideoStreamReceiver] Constructor completed successfully";
}

/**
 * @brief Destructor - ensures stream is stopped and resources are released
 *
 * Called automatically when object is deleted (either explicitly or by Qt parent system)
 */
VideoStreamReceiver::~VideoStreamReceiver()
{
    qDebug() << "[VideoStreamReceiver] Destructor called";
    // Stop the stream and clean up all GStreamer resources
    // Safe to call even if stream isn't running
    stopStream();
    qDebug() << "[VideoStreamReceiver] Destructor completed";
}

/**
 * @brief Starts receiving and decoding video stream from the specified UDP port
 * @param port UDP port number to listen on (typically 5000 or similar)
 *
 * This method creates a complete GStreamer pipeline for low-latency video reception.
 * If a stream is already running, it stops it first before starting the new one.
 *
 * Pipeline explanation:
 * 1. udpsrc - Receives UDP packets on specified port
 * 2. application/x-rtp,encoding-name=JPEG - Caps filter for RTP/JPEG
 * 3. rtpjpegdepay - Extracts JPEG frames from RTP packets
 * 4. jpegdec - Decodes JPEG to raw video frames
 * 5. videoconvert - Converts pixel format if needed
 * 6. video/x-raw,format=RGB - Forces RGB format for Qt compatibility
 * 7. appsink - Extracts frames for application use
 */
void VideoStreamReceiver::startStream(int port)
{
    qDebug() << "[VideoStreamReceiver::startStream] Called with port:" << port;

    // If a pipeline already exists, stop it first to avoid resource conflicts
    if (m_pipeline) {
        qDebug() << "[VideoStreamReceiver::startStream] Existing pipeline found, stopping it first";
        stopStream();
    }

    // Update UI status to show stream is initializing
    qDebug() << "[VideoStreamReceiver::startStream] Setting status to 'Starting stream...'";
    setStatus("Starting stream...");

    // Build the GStreamer pipeline description string
    // This uses GStreamer's launch syntax to create and link elements
    QString pipelineStr = QString(
        "udpsrc port=%1 buffer-size=200000 ! "           // UDP source with 200KB buffer (port substituted below)
        "application/x-rtp,encoding-name=JPEG ! "        // RTP caps filter for JPEG payload
        "rtpjpegdepay ! "                                // RTP depayloader - extracts JPEG from RTP packets
        "queue max-size-buffers=100 leaky=downstream ! "   // Queue with 2-frame buffer (smooth transitions)
        //                        ^ drop oldest frames if queue fills up
        "jpegdec ! "                                     // JPEG decoder - converts JPEG to raw video
        "videoconvert ! "                                // Format converter - ensures compatible pixel format
        "video/x-raw,format=RGB ! "                      // Caps filter - force RGB format (Qt uses RGB)
        "appsink name=sink sync=false max-buffers=100 drop=false"  // App sink with 2-frame buffering
        //        ^ give it a name for later retrieval
        //                     ^ don't sync to clock (low latency)
        //                                ^ keep 2 buffers (retain previous frame)
        //                                                ^ let queue handle drops (smoother)
    ).arg(port);  // Substitute %1 with actual port number

    qDebug() << "[VideoStreamReceiver::startStream] Creating pipeline:" << pipelineStr;

    // Parse and create the pipeline from the description string
    qDebug() << "[VideoStreamReceiver::startStream] Calling gst_parse_launch...";
    GError *error = nullptr;  // GStreamer uses this for error reporting
    m_pipeline = gst_parse_launch(pipelineStr.toUtf8().constData(), &error);

    // Check if pipeline creation failed
    if (error) {
        QString errorMsg = QString("Failed to create pipeline: %1").arg(error->message);
        qWarning() << "[VideoStreamReceiver::startStream] ERROR:" << errorMsg;
        emit this->errorOccurred(errorMsg);  // Notify UI of error
        setStatus("Error: " + QString(error->message));
        g_error_free(error);  // Free GLib error object
        return;  // Abort startup
    }
    qDebug() << "[VideoStreamReceiver::startStream] Pipeline created successfully";

    // Extract the appsink element by name so we can configure it
    // The pipeline is a bin (container) of elements
    qDebug() << "[VideoStreamReceiver::startStream] Getting appsink element...";
    m_appsink = gst_bin_get_by_name(GST_BIN(m_pipeline), "sink");
    if (!m_appsink) {
        qWarning() << "[VideoStreamReceiver::startStream] ERROR: Failed to get appsink element";
        setStatus("Error: Failed to initialize");
        gst_object_unref(m_pipeline);  // Clean up the pipeline
        m_pipeline = nullptr;
        return;
    }
    qDebug() << "[VideoStreamReceiver::startStream] Appsink element retrieved successfully";

    // Configure callbacks for the appsink element
    // These callbacks are invoked when specific events occur
    qDebug() << "[VideoStreamReceiver::startStream] Configuring appsink callbacks...";
    GstAppSinkCallbacks callbacks;
    callbacks.eos = nullptr;              // End-of-stream callback (not needed)
    callbacks.new_preroll = nullptr;      // Preroll callback (not needed)
    callbacks.new_sample = onNewSample;   // Called when a new frame arrives (IMPORTANT!)
    #if GST_VERSION_MAJOR >= 1 && GST_VERSION_MINOR >= 20
    callbacks.propose_allocation = nullptr;  // Memory allocation callback (newer GStreamer versions)
    #endif

    // Register the callbacks with the appsink
    // 'this' is passed as user_data and will be available in the callbacks
    gst_app_sink_set_callbacks(GST_APP_SINK(m_appsink), &callbacks, this, nullptr);
    //                                                                 ^      ^
    //                                                                 |      destroy notify function (none)
    //                                                                 user_data pointer (our instance)
    qDebug() << "[VideoStreamReceiver::startStream] Appsink callbacks registered";

    // Get the message bus for the pipeline
    // The bus is used for asynchronous communication (errors, state changes, etc.)
    qDebug() << "[VideoStreamReceiver::startStream] Getting message bus...";
    m_bus = gst_element_get_bus(m_pipeline);
    qDebug() << "[VideoStreamReceiver::startStream] Message bus retrieved";

    // Use Qt timer to poll bus instead of GLib's event loop (which conflicts with Qt)
    // Create and start timer to check for messages every 50ms
    qDebug() << "[VideoStreamReceiver::startStream] Creating bus polling timer...";
    m_busTimer = new QTimer(this);
    connect(m_busTimer, &QTimer::timeout, this, &VideoStreamReceiver::checkBusMessages);
    m_busTimer->start(50);  // Check bus 20 times per second
    qDebug() << "[VideoStreamReceiver::startStream] Bus polling timer started (50ms interval)";

    // Create frame timeout timer to detect when no frames are being received
    // Check every 1 second if we've received frames recently
    qDebug() << "[VideoStreamReceiver::startStream] Creating frame timeout timer...";
    m_frameTimeoutTimer = new QTimer(this);
    connect(m_frameTimeoutTimer, &QTimer::timeout, this, &VideoStreamReceiver::checkFrameTimeout);
    m_frameTimeoutTimer->start(1000);  // Check every 1 second
    qDebug() << "[VideoStreamReceiver::startStream] Frame timeout timer started (1000ms interval)";

    // Initialize frame timeout tracking
    m_lastFrameTime = 0;  // No frames received yet
    setHasActiveStream(false);  // Start with no active stream until we receive frames

    // Transition the pipeline to PLAYING state
    // GStreamer uses state machine: NULL -> READY -> PAUSED -> PLAYING
    qDebug() << "[VideoStreamReceiver::startStream] Setting pipeline state to PLAYING...";
    GstStateChangeReturn ret = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    qDebug() << "[VideoStreamReceiver::startStream] gst_element_set_state returned:" << ret;

    if (ret == GST_STATE_CHANGE_FAILURE) {
        // Pipeline couldn't start (maybe port already in use, wrong format, etc.)
        qWarning() << "[VideoStreamReceiver::startStream] ERROR: Failed to start pipeline";
        setStatus("Error: Failed to start");
        emit errorOccurred("Failed to start stream");
        stopStream();  // Clean up
        return;
    }

    // Success! Update state and notify UI
    qDebug() << "[VideoStreamReceiver::startStream] Pipeline state change successful";
    qDebug() << "[VideoStreamReceiver::startStream] Updating streaming state...";
    setStreaming(true);  // Update property and emit signal
    setStatus("Streaming on port " + QString::number(port));
    qDebug() << "[VideoStreamReceiver::startStream] Stream started successfully on port:" << port;
    qDebug() << "[VideoStreamReceiver::startStream] startStream() completed successfully";
}

/**
 * @brief Stops the video stream and releases all GStreamer resources
 *
 * This method safely shuts down the pipeline and cleans up all allocated resources.
 * It's safe to call multiple times or when no stream is running.
 *
 * Cleanup order is important:
 * 1. Set pipeline to NULL state (stops all processing)
 * 2. Remove bus watch and unref bus
 * 3. Unref pipeline (which also unrefs all elements including appsink)
 * 4. Update state flags
 */
void VideoStreamReceiver::stopStream()
{
    qDebug() << "[VideoStreamReceiver::stopStream] Called";

    // Only proceed if we have an active pipeline
    if (m_pipeline) {
        qDebug() << "[VideoStreamReceiver::stopStream] Active pipeline found, stopping...";

        // CRITICAL: Stop timers FIRST to prevent callbacks during cleanup
        // This prevents race conditions where callbacks try to access pipeline being destroyed
        if (m_busTimer) {
            qDebug() << "[VideoStreamReceiver::stopStream] Stopping bus polling timer...";
            m_busTimer->stop();
            qDebug() << "[VideoStreamReceiver::stopStream] Disconnecting bus timer...";
            m_busTimer->disconnect();  // Disconnect all signals to prevent callbacks during deletion
            qDebug() << "[VideoStreamReceiver::stopStream] Deleting bus timer...";
            m_busTimer->deleteLater();  // Use deleteLater instead of delete for safety
            m_busTimer = nullptr;
            qDebug() << "[VideoStreamReceiver::stopStream] Bus timer marked for deletion";
        }

        if (m_frameTimeoutTimer) {
            qDebug() << "[VideoStreamReceiver::stopStream] Stopping frame timeout timer...";
            m_frameTimeoutTimer->stop();
            qDebug() << "[VideoStreamReceiver::stopStream] Disconnecting frame timeout timer...";
            m_frameTimeoutTimer->disconnect();  // Disconnect all signals
            qDebug() << "[VideoStreamReceiver::stopStream] Deleting frame timeout timer...";
            m_frameTimeoutTimer->deleteLater();  // Use deleteLater for safety
            m_frameTimeoutTimer = nullptr;
            qDebug() << "[VideoStreamReceiver::stopStream] Frame timeout timer marked for deletion";
        }

        // Flush any remaining messages from the bus before shutting down
        // This prevents warnings about unhandled messages
        if (m_bus) {
            qDebug() << "[VideoStreamReceiver::stopStream] Flushing remaining bus messages...";
            GstMessage *msg;
            while ((msg = gst_bus_pop(m_bus)) != nullptr) {
                gst_message_unref(msg);
            }
            qDebug() << "[VideoStreamReceiver::stopStream] Bus flushed";
        }

        // Transition pipeline to NULL state
        // This stops all processing and releases hardware resources
        // State transitions: PLAYING -> PAUSED -> READY -> NULL
        qDebug() << "[VideoStreamReceiver::stopStream] Setting pipeline state to NULL...";
        GstStateChangeReturn ret = gst_element_set_state(m_pipeline, GST_STATE_NULL);

        if (ret == GST_STATE_CHANGE_FAILURE) {
            qWarning() << "[VideoStreamReceiver::stopStream] Failed to set pipeline to NULL state";
        } else {
            qDebug() << "[VideoStreamReceiver::stopStream] Pipeline state change initiated";
        }

        // Don't wait for state change during shutdown - it can hang if event loop is stopped
        // The pipeline will finish transitioning asynchronously before being unreferenced

        // Unreference the appsink
        // gst_bin_get_by_name() adds a reference, so we must unref it
        // Do this BEFORE unreferencing the pipeline
        if (m_appsink) {
            qDebug() << "[VideoStreamReceiver::stopStream] Unreferencing appsink...";
            gst_object_unref(m_appsink);
            m_appsink = nullptr;
            qDebug() << "[VideoStreamReceiver::stopStream] Appsink unreferenced";
        }

        // Clean up the message bus
        if (m_bus) {
            qDebug() << "[VideoStreamReceiver::stopStream] Unreferencing message bus...";
            gst_object_unref(m_bus);      // Decrease reference count (will free if count reaches 0)
            m_bus = nullptr;              // Clear our pointer
            qDebug() << "[VideoStreamReceiver::stopStream] Message bus unreferenced";
        }

        // Unreference the pipeline last
        // This decreases the reference count and will automatically free the pipeline
        // and all its elements when the count reaches 0
        qDebug() << "[VideoStreamReceiver::stopStream] Unreferencing pipeline...";
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;   // Clear our pointer
        qDebug() << "[VideoStreamReceiver::stopStream] Pipeline unreferenced";

        // Update state and notify UI
        qDebug() << "[VideoStreamReceiver::stopStream] Updating streaming state to false...";
        setStreaming(false);    // Update property and emit streamingChanged signal
        setHasActiveStream(false);  // No active stream when stopped
        setStatus("Stopped");   // Update status and emit statusChanged signal
        qDebug() << "[VideoStreamReceiver::stopStream] Stream stopped successfully";
    } else {
        qDebug() << "[VideoStreamReceiver::stopStream] No active pipeline, nothing to stop";
    }
    qDebug() << "[VideoStreamReceiver::stopStream] stopStream() completed";
}

/**
 * @brief Static callback invoked by GStreamer when a new decoded frame is available
 * @param appsink The appsink element that has a new sample ready
 * @param user_data Pointer to VideoStreamReceiver instance (set in startStream)
 * @return GST_FLOW_OK to continue processing, or error code to stop pipeline
 *
 * IMPORTANT: This is called from GStreamer's streaming thread, NOT the main Qt thread!
 * Must be static to match GStreamer's C-style callback signature.
 * Thread safety is handled by Qt's signal/slot system in processNewSample().
 */
GstFlowReturn VideoStreamReceiver::onNewSample(GstAppSink *appsink, gpointer user_data)
{
    // Cast the generic pointer back to our VideoStreamReceiver instance
    // This pointer was passed to gst_app_sink_set_callbacks() in startStream()
    VideoStreamReceiver *receiver = static_cast<VideoStreamReceiver*>(user_data);

    // Pull the sample (frame + metadata) from the appsink
    // This removes it from the appsink's internal queue
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (sample) {
        // Process the sample (extract image data, update QImage, emit signal)
        receiver->processNewSample(sample);

        // Decrease reference count on the sample
        // GStreamer will free it when ref count reaches 0
        gst_sample_unref(sample);
    } else {
        qWarning() << "[VideoStreamReceiver::onNewSample] WARNING: Sample is null";
    }

    // Return GST_FLOW_OK to tell GStreamer everything is fine and to continue
    // Could return GST_FLOW_ERROR or GST_FLOW_EOS to stop the pipeline
    return GST_FLOW_OK;
}

/**
 * @brief Processes a new video sample from GStreamer and converts it to QImage
 * @param sample GStreamer sample containing the video frame buffer and metadata (caps)
 *
 * This method extracts the raw RGB data from the GStreamer buffer and creates a QImage.
 * It's called from onNewSample() which runs in GStreamer's thread.
 *
 * IMPORTANT: This runs in GStreamer's streaming thread, NOT the main Qt thread!
 * We must use QMetaObject::invokeMethod to safely update Qt objects on the main thread.
 *
 * Steps:
 * 1. Extract buffer (pixel data) and caps (format info) from sample
 * 2. Parse caps to get video format details (width, height, stride)
 * 3. Map buffer to access raw memory
 * 4. Create QImage from the raw data (deep copy so it's safe across threads)
 * 5. Unmap buffer to release memory lock
 * 6. Marshal the QImage to main thread using QMetaObject::invokeMethod
 */
void VideoStreamReceiver::processNewSample(GstSample *sample)
{
    // Extract the buffer (contains actual pixel data) from the sample
    GstBuffer *buffer = gst_sample_get_buffer(sample);

    // Extract the caps (capabilities - format information) from the sample
    // Caps describe the format: resolution, pixel format, framerate, etc.
    GstCaps *caps = gst_sample_get_caps(sample);

    // Sanity check - both must exist for us to proceed
    if (!buffer || !caps) {
        qWarning() << "[VideoStreamReceiver::processNewSample] ERROR: Buffer or caps is null, skipping frame";
        return;  // Silently skip this frame
    }

    // Parse the caps into a GstVideoInfo structure for easy access to video parameters
    GstVideoInfo videoInfo;
    if (!gst_video_info_from_caps(&videoInfo, caps)) {
        qWarning() << "[VideoStreamReceiver::processNewSample] ERROR: Failed to parse video info from caps";
        return;  // Can't proceed without knowing the format
    }

    // Map the buffer to access the raw pixel data
    // Mapping locks the buffer and gives us a pointer to its memory
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        qWarning() << "[VideoStreamReceiver::processNewSample] ERROR: Failed to map buffer";
        return;  // Can't access the data
    }

    // Extract video dimensions and row stride from the video info
    int width = GST_VIDEO_INFO_WIDTH(&videoInfo);      // Frame width in pixels
    int height = GST_VIDEO_INFO_HEIGHT(&videoInfo);    // Frame height in pixels
    int stride = GST_VIDEO_INFO_PLANE_STRIDE(&videoInfo, 0);  // Bytes per row (may include padding)
    //                                                   ^
    //                                                   plane 0 (RGB has only one plane)

    // Create QImage from the raw buffer data
    // We MUST call .copy() because:
    // 1. map.data will be invalid after we unmap the buffer
    // 2. QImage needs to own its own data for thread safety
    // 3. This QImage will be passed to the main thread
    QImage newImage = QImage(map.data, width, height, stride,
                             QImage::Format_RGB888).copy();
    //                       ^ RGB format (8 bits per channel, 3 channels)
    //                                             ^ deep copy the data

    // Flip the image horizontally and vertically to correct orientation
    newImage = newImage.mirrored(true, true);
    //                           ^     ^
    //                           |     vertical flip
    //                           horizontal flip

    // Unmap the buffer to release the memory lock
    // After this, map.data is invalid and should not be accessed
    gst_buffer_unmap(buffer, &map);

    // CRITICAL: We're in GStreamer's thread, but Qt objects must be updated on main thread!
    // Use QMetaObject::invokeMethod to safely marshal the image to the main thread
    // Qt::QueuedConnection ensures the lambda runs on the main thread
    QMetaObject::invokeMethod(this, [this, newImage]() {
        // This lambda runs on the main Qt thread, so it's safe to update Qt objects

        // Update the current image (thread-safe now)
        m_currentImage = newImage;

        // Generate a new unique frame ID for QML
        // QML monitors the currentFrame property; changing it triggers Image reload
        m_frameCounter++;  // Increment to next frame number
        m_frameId = QString("frame_%1").arg(m_frameCounter);  // e.g., "frame_42"

        // Update last frame time for timeout detection
        m_lastFrameTime = QDateTime::currentMSecsSinceEpoch();

        // Mark stream as active if this is the first frame or if it was previously inactive
        if (!m_hasActiveStream) {
            setHasActiveStream(true);
            setStatus("Streaming (receiving frames)");
        }

        // Emit signal to notify QML that a new frame is available
        // QML will call currentFrame() getter, see it changed, and call requestImage()
        // Safe to emit from main thread
        emit frameChanged();
    }, Qt::QueuedConnection);
    //     ^ IMPORTANT: Queued connection ensures lambda executes on object's thread (main thread)
}

/**
 * @brief Static callback for GStreamer bus messages (errors, warnings, state changes)
 * @param bus The message bus (unused)
 * @param message The message to process
 * @param user_data Pointer to VideoStreamReceiver instance (set in startStream)
 * @return TRUE to continue receiving messages, FALSE to stop
 *
 * The GStreamer bus is used for asynchronous event notification.
 * This callback is invoked from GLib's main loop whenever a message arrives.
 * Must be static to match GStreamer's C-style callback signature.
 */
gboolean VideoStreamReceiver::busCallback(GstBus *bus, GstMessage *message, gpointer user_data)
{
    Q_UNUSED(bus)  // We don't need the bus pointer - we just process the message

    // Cast the generic pointer back to our VideoStreamReceiver instance
    VideoStreamReceiver *receiver = static_cast<VideoStreamReceiver*>(user_data);

    // Delegate to member function for actual message handling
    receiver->handleBusMessage(message);

    // Return TRUE to keep receiving messages
    // Returning FALSE would stop the bus watch
    return TRUE;
}

/**
 * @brief Handles different types of messages from the GStreamer pipeline bus
 * @param message The GStreamer message to process
 *
 * The bus carries various types of messages:
 * - ERROR: Fatal errors that stop the pipeline
 * - WARNING: Non-fatal issues that don't stop playback
 * - EOS: End of stream (shouldn't happen with live UDP stream)
 * - STATE_CHANGED: Pipeline state transitions
 * - And many others (INFO, TAG, etc.) that we ignore
 *
 * This method is called from busCallback() when messages arrive.
 */
void VideoStreamReceiver::handleBusMessage(GstMessage *message)
{
    // Process different message types using a switch statement
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            // Fatal error - pipeline will stop
            GError *err;      // GLib error structure
            gchar *debug;     // Debug string with detailed error info

            // Parse the error message into separate error object and debug string
            gst_message_parse_error(message, &err, &debug);

            // Convert to Qt string for logging and UI display
            QString errorMsg = QString("GStreamer error: %1").arg(err->message);
            qWarning() << errorMsg;
            qWarning() << "Debug info:" << debug;  // Usually contains element names, file:line, etc.

            // Update status and notify UI
            setStatus("Error: " + QString(err->message));
            emit errorOccurred(errorMsg);  // QML can connect to this signal to show error dialogs

            // Free GLib-allocated memory
            g_error_free(err);   // Free the error structure
            g_free(debug);       // Free the debug string
            break;
        }

        case GST_MESSAGE_WARNING: {
            // Non-fatal warning - pipeline continues running
            GError *warn;     // Warning structure (same type as error)
            gchar *debug;     // Debug info

            // Parse warning message
            gst_message_parse_warning(message, &warn, &debug);

            // Log to console - we don't update UI for warnings
            qWarning() << "GStreamer warning:" << warn->message;

            // Free GLib-allocated memory
            g_error_free(warn);
            g_free(debug);
            break;
        }

        case GST_MESSAGE_EOS:
            // End-of-stream message
            // This shouldn't normally happen with a live UDP stream (which has no "end")
            // But could occur if sender disconnects gracefully
            qDebug() << "End of stream";
            setStatus("Stream ended");
            break;

        case GST_MESSAGE_STATE_CHANGED: {
            // State change notification
            // Many elements send these, but we only care about the pipeline's state
            if (GST_MESSAGE_SRC(message) == GST_OBJECT(m_pipeline)) {
                GstState oldState, newState, pendingState;

                // Parse state change details
                gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);

                // Log state transition for debugging
                // e.g., "Pipeline state: READY -> PAUSED"
                qDebug() << "Pipeline state:"
                         << gst_element_state_get_name(oldState)  // e.g., "READY"
                         << "->" << gst_element_state_get_name(newState);  // e.g., "PAUSED"
            }
            // Ignore state changes from individual elements (udpsrc, jpegdec, etc.)
            break;
        }

        default:
            // Ignore all other message types (INFO, TAG, BUFFERING, etc.)
            // Not logging these to avoid cluttering console
            break;
    }
}

/**
 * @brief Updates the status message and emits signal if it changed
 * @param status New status message to display
 *
 * This is a helper method that updates the status property and notifies QML.
 * Only emits the signal if the status actually changed to avoid unnecessary updates.
 *
 * Typical status messages:
 * - "Ready" - Initial state
 * - "Starting stream..." - During initialization
 * - "Streaming on port 5000" - Active streaming
 * - "Error: <message>" - Error occurred
 * - "Stopped" - Stream stopped
 */
void VideoStreamReceiver::setStatus(const QString &status)
{
    // Only update and emit signal if status actually changed
    // This optimization prevents unnecessary QML property updates
    if (m_status != status) {
        m_status = status;          // Update internal state
        emit statusChanged();       // Notify QML that status property changed
                                    // QML can bind to this to update UI text
    }
}

/**
 * @brief Updates the streaming state flag and emits signal if it changed
 * @param streaming New streaming state (true = active, false = stopped)
 *
 * This is a helper method that updates the isStreaming property and notifies QML.
 * Only emits the signal if the state actually changed.
 *
 * QML can use this property to:
 * - Enable/disable UI controls (e.g., disable "Start" button while streaming)
 * - Show/hide status indicators
 * - Update button text ("Start" vs "Stop")
 */
void VideoStreamReceiver::setStreaming(bool streaming)
{
    // Only update and emit signal if streaming state actually changed
    if (m_isStreaming != streaming) {
        m_isStreaming = streaming;      // Update internal flag
        emit streamingChanged();        // Notify QML that isStreaming property changed
    }
}

/**
 * @brief Updates the active stream state flag and emits signal if it changed
 * @param hasActiveStream New active stream state (true = receiving frames, false = no frames)
 *
 * This is a helper method that updates the hasActiveStream property and notifies QML.
 * Only emits the signal if the state actually changed.
 *
 * QML can use this property to:
 * - Show red/green status indicator based on actual frame reception
 * - Display "No Video Stream" message when no frames are coming in
 */
void VideoStreamReceiver::setHasActiveStream(bool hasActiveStream)
{
    // Only update and emit signal if active stream state actually changed
    if (m_hasActiveStream != hasActiveStream) {
        m_hasActiveStream = hasActiveStream;    // Update internal flag
        emit hasActiveStreamChanged();          // Notify QML that hasActiveStream property changed
    }
}

/**
 * @brief Qt slot to check if frames are still being received
 *
 * Called periodically by QTimer (every 1 second) to detect stream timeout.
 * If no frames have been received for 3 seconds, marks the stream as inactive.
 * This allows the UI to show "No connection" even when the pipeline is running.
 */
void VideoStreamReceiver::checkFrameTimeout()
{
    // Only check if pipeline is running and timer still exists
    if (!m_isStreaming || !m_pipeline || !m_frameTimeoutTimer) {
        return;
    }

    // If we've never received a frame, check if we should mark it as timeout
    if (m_lastFrameTime == 0) {
        // Pipeline is running but no frames received yet
        if (m_hasActiveStream) {
            setHasActiveStream(false);
            setStatus("Waiting for video stream...");
            qDebug() << "[VideoStreamReceiver::checkFrameTimeout] No frames received yet";
        }
        return;
    }

    // Calculate time since last frame
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeSinceLastFrame = currentTime - m_lastFrameTime;

    // If more than 3 seconds since last frame, mark stream as inactive
    if (timeSinceLastFrame > 3000) {
        if (m_hasActiveStream) {
            setHasActiveStream(false);
            setStatus("No video stream (timeout)");
            qDebug() << "[VideoStreamReceiver::checkFrameTimeout] Stream timeout - last frame received"
                     << timeSinceLastFrame << "ms ago";
        }
    }
}

/**
 * @brief Qt slot to check for GStreamer bus messages
 *
 * Called periodically by QTimer to poll the bus for messages.
 * This avoids using GLib's main loop which conflicts with Qt's event loop.
 * We poll the bus using gst_bus_pop() which doesn't require GLib integration.
 */
void VideoStreamReceiver::checkBusMessages()
{
    // Check if bus and timer still exist (prevents access during cleanup)
    if (!m_bus || !m_busTimer || !m_pipeline) {
        return;
    }

    // Poll for messages without blocking
    // gst_bus_pop() returns nullptr if no messages are available
    GstMessage *message;
    int messageCount = 0;
    while ((message = gst_bus_pop(m_bus)) != nullptr) {
        messageCount++;
        qDebug() << "[VideoStreamReceiver::checkBusMessages] Message" << messageCount << "received, type:" << GST_MESSAGE_TYPE_NAME(message);

        // Process the message
        handleBusMessage(message);

        // Unref the message after processing
        gst_message_unref(message);
    }

    if (messageCount == 0) {
        // Only log occasionally to avoid spam
        static int noMessageCounter = 0;
        if (++noMessageCounter % 100 == 0) {  // Log every 100 polls (5 seconds at 50ms interval)
            qDebug() << "[VideoStreamReceiver::checkBusMessages] No messages on bus (polled" << noMessageCounter << "times)";
        }
    }
}