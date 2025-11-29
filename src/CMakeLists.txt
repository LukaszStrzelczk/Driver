qt_add_library(driversrc STATIC)

qt_add_qml_module(driversrc
    URI "driversrc"
    VERSION 1.0
    SOURCES
        sources/steeringcontroller.cpp
        headers/steeringcontroller.hpp
)

# Make headers directory available for includes
target_include_directories(driversrc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_link_libraries(driversrc
    PRIVATE
        Qt6::Quick
        SDL2::SDL2
)
