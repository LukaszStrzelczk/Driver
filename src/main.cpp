/**
 * @file main.cpp
 * @brief Application entry point and component initialization
 *
 * Sets up the Qt application, registers QML types and context properties,
 * and initializes video streaming components.
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include "includes/steeringcontroller.hpp"
#include "includes/mjpegdecoder.hpp"              // Old HTTP MJPEG implementation (kept for reference)
#include "includes/videoscreenreciever.hpp"       // New GStreamer RTP implementation
#include "net/includes/steeringcontrollerservice.hpp"

int qMain(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Set Qt Quick Controls style to Basic for full customization support
    QQuickStyle::setStyle("Basic");

    // Create SteeringController instance
    SteeringController steeringController(&app);

    // Auto-connect to first available device
    if (steeringController.availableDevices().length() > 0) {
        steeringController.connectDevice(0);
    }

    // Create WebSocket service
    SteeringControllerService steeringControllerService(&steeringController, &app);

    /* ========================================================================
     * VIDEO STREAMING SETUP
     * ========================================================================
     */

    // GStreamer RTP/MJPEG Video Receiver (NEW - ACTIVE)
    // Receives video over UDP on port 5000
    VideoStreamReceiver videoReceiver(&app);
    VideoImageProvider *videoImageProvider = new VideoImageProvider(&videoReceiver);

    // Old MJPEG HTTP Decoder (KEPT FOR REFERENCE - CURRENTLY UNUSED)
    // This implementation is commented out in VideoScreen.qml but kept here for easy rollback
    //MjpegDecoder mjpegDecoder(&app);

    /* ========================================================================
     * QML ENGINE SETUP
     * ========================================================================
     */

    QQmlApplicationEngine engine;

    // Register image providers for QML Image elements
    engine.addImageProvider("videostream", videoImageProvider);  // GStreamer provider (active)
    //engine.addImageProvider("mjpeg", mjpegDecoder.imageProvider());  // Old MJPEG provider (inactive)

    // Register C++ objects as QML context properties
    // These are accessible globally in QML as singleton-like objects
    engine.rootContext()->setContextProperty("steeringController", &steeringController);
    engine.rootContext()->setContextProperty("steeringControllerService", &steeringControllerService);
    engine.rootContext()->setContextProperty("videoReceiver", &videoReceiver);  // GStreamer receiver (active)
    //engine.rootContext()->setContextProperty("mjpegDecoder", &mjpegDecoder);    // Old decoder (inactive)

    // Handle QML loading failures
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // Load the main QML module
    engine.loadFromModule("Driver", "Main");

    return app.exec();
}
