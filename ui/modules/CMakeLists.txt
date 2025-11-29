qt_add_library(modules STATIC)
qt_add_qml_module(modules
    URI "modules"
    VERSION 1.0
    QML_FILES
        Wheel.qml
        Pedal.qml
    RESOURCES
        resources/steeringWheel.svg
        resources/pedal.svg
)
