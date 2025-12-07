cmake_minimum_required(VERSION 3.16)

# Toolchain file is now set via CMakeUserPresets.json for machine-specific paths

project(Driver VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Network WebSockets)
find_package(SDL2 CONFIG REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(src)
add_subdirectory(apps)
add_subdirectory(ui)
add_subdirectory(utils)
add_subdirectory(net)

qt_add_executable(appDriver
    src/main.cpp
)

qt_add_qml_module(appDriver
    URI Driver
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        resources/bg.jpg
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appDriver PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appDriver
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appDriver
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Multimedia
        SDL2::SDL2
        videoplugin
        wheelplugin
        modulesplugin
        utilsplugin
        driversrcplugin
        net
        netplugin
)

include(GNUInstallDirs)
install(TARGETS appDriver
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
