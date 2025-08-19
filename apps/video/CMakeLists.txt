qt_add_library(video STATIC)
qt_add_qml_module(video
    URI "video"
    VERSION 1.0
    QML_FILES
        VideoScreen.qml
)