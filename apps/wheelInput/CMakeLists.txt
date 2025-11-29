qt_add_library(wheel STATIC)
qt_add_qml_module(wheel
    URI "wheel"
    VERSION 1.0
    QML_FILES
        WheelInput.qml
)