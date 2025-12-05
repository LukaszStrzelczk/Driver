qt_add_library(utils STATIC)
qt_add_qml_module(utils
    URI "utils"
    VERSION 1.0
    QML_FILES
        ProgressBar.qml
        CenteredProgressBar.qml
)