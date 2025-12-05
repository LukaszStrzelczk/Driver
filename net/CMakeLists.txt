qt_add_library(net STATIC)

qt_add_qml_module(net
    URI "net"
    VERSION 1.0
    SOURCES
        sources/steeringcontrollerservice.cpp
        includes/steeringcontrollerservice.hpp
)

# Make headers directory available for includes
target_include_directories(net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(net
    PRIVATE
        Qt6::Quick
        SDL2::SDL2
        Qt6::WebSockets
        driversrc
)