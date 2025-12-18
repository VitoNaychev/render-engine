cmake_minimum_required(VERSION 3.14)

project(Engine LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(PROJECT_SOURCES
        engine/engine.cpp
        app/app.cpp
        app/window.cpp
        app/main.cpp
        app/viewport.cpp
)

qt_add_executable(EngineApp ${PROJECT_SOURCES})

target_link_libraries(EngineApp PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
