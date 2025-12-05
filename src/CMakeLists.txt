qt_add_library(driversrc STATIC)

qt_add_qml_module(driversrc
    URI "driversrc"
    VERSION 1.0
    SOURCES
        sources/steeringcontroller.cpp
        includes/steeringcontroller.hpp
)

# Make headers directory available for includes
target_include_directories(driversrc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(driversrc
    PRIVATE
        Qt6::Quick
        Qt6::Multimedia
        SDL2::SDL2
)
