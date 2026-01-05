qt_add_library(driversrc STATIC)

qt_add_qml_module(driversrc
    URI "driversrc"
    VERSION 1.0
    SOURCES
        sources/steeringcontroller.cpp
        includes/steeringcontroller.hpp
        sources/videoscreenreciever.cpp
        includes/videoscreenreciever.hpp
)

# Make headers directory available for includes
target_include_directories(driversrc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${GSTREAMER_INCLUDE_DIRS}
)

target_link_libraries(driversrc
    PRIVATE
        Qt6::Quick
        Qt6::Multimedia
        Qt6::Network
        SDL2::SDL2
        gstreamer-1.0
        gstbase-1.0
        gstapp-1.0
        gstvideo-1.0
        gobject-2.0
        glib-2.0
)
