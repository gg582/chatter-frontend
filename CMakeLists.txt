cmake_minimum_required(VERSION 3.16)
project(chatter_frontend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(QT_PACKAGE Qt6)
set(QT_VERSION_MAJOR 6)
find_package(Qt6 COMPONENTS Widgets QUIET)

if (NOT Qt6_FOUND)
    set(QT_PACKAGE Qt5)
    set(QT_VERSION_MAJOR 5)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
else()
    message(STATUS "Using Qt6 for chatter-frontend")
endif()

add_subdirectory(src)
