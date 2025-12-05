#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include "includes/steeringcontroller.hpp"
#include "net/includes/steeringcontrollerservice.hpp"

int qMain(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Create SteeringController instance
    SteeringController steeringController(&app);

    // Auto-connect to first available device
    if (steeringController.availableDevices().length() > 0) {
        steeringController.connectDevice(0);
    }

    // Create WebSocket service
    SteeringControllerService steeringControllerService(&steeringController, &app);

    // Connect to WebSocket server (uncomment and update URL to connect)
    // steeringControllerService.connectToServer("ws://localhost:8080");

    QQmlApplicationEngine engine;

    // Register instances as QML context properties
    engine.rootContext()->setContextProperty("steeringController", &steeringController);
    engine.rootContext()->setContextProperty("steeringControllerService", &steeringControllerService);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("Driver", "Main");

    return app.exec();
}
