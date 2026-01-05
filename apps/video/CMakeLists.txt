qt_add_library(video STATIC)
qt_add_qml_module(video
    URI "video"
    VERSION 1.0
    QML_FILES
        VideoScreen.qml
)

target_link_libraries(video
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Multimedia
)