# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

project(dde-shell-weather-plugin VERSION 1.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core DBus LinguistTools Network Quick QuickControls2 Widgets)
find_package(Qt6 OPTIONAL_COMPONENTS Positioning Location)
find_package(Dtk6 REQUIRED COMPONENTS Core Widget)
find_package(DDEShell REQUIRED)

# Create plugin library
add_library(ds-weather-applet SHARED
    src/locationsettingsdialog.h
    src/locationsettingsdialog.cpp
    src/weatherapplet.h
    src/weatherapplet.cpp
    src/weatherprovider.h
    src/weatherprovider.cpp
)

target_include_directories(ds-weather-applet PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ds-weather-applet PRIVATE
    Dde::Shell
    Qt6::Core
    Qt6::DBus
    Qt6::Network
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Dtk6::Core
    Dtk6::Widget
)

if(Qt6Positioning_FOUND)
    target_compile_definitions(ds-weather-applet PRIVATE QT_POSITIONING_LIB)
    target_link_libraries(ds-weather-applet PRIVATE Qt6::Positioning)
endif()

if(Qt6Location_FOUND)
    target_link_libraries(ds-weather-applet PRIVATE Qt6::Location)
endif()

set(DS_WEATHER_PACKAGE_ID org.deepin.ds.weather)
set(DS_WEATHER_QML_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/package/main.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/package/WeatherIcon.qml
)
set(DS_WEATHER_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/weatherapplet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/weatherprovider.cpp
)

# Build the dde-shell package tree, but exclude VCS metadata from installation.
ds_build_package(
    PACKAGE ${DS_WEATHER_PACKAGE_ID}
    TARGET ds-weather-applet
)

ds_handle_package_translation(
    PACKAGE ${DS_WEATHER_PACKAGE_ID}
    QML_FILES ${DS_WEATHER_QML_FILES}
    SOURCE_FILES ${DS_WEATHER_SOURCE_FILES}
)

install(
    DIRECTORY ${PROJECT_BINARY_DIR}/packages/${DS_WEATHER_PACKAGE_ID}/
    DESTINATION ${DDE_SHELL_PACKAGE_INSTALL_DIR}/${DS_WEATHER_PACKAGE_ID}
    PATTERN ".git" EXCLUDE
    PATTERN ".gitignore" EXCLUDE
)

install(
    TARGETS ds-weather-applet
    DESTINATION ${DDE_SHELL_PLUGIN_INSTALL_DIR}/
)
