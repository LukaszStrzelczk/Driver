#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include "includes/steeringcontroller.hpp"
#include "includes/mjpegdecoder.hpp"
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

    // Create MJPEG decoder
    MjpegDecoder mjpegDecoder(&app);

    QQmlApplicationEngine engine;

    // Register MJPEG image provider
    engine.addImageProvider("mjpeg", mjpegDecoder.imageProvider());

    // Register instances as QML context properties
    engine.rootContext()->setContextProperty("steeringController", &steeringController);
    engine.rootContext()->setContextProperty("steeringControllerService", &steeringControllerService);
    engine.rootContext()->setContextProperty("mjpegDecoder", &mjpegDecoder);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("Driver", "Main");

    return app.exec();
}
