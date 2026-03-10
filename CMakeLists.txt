# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

project(dde-shell-weather-plugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Network Quick QuickControls2)
find_package(Qt6 OPTIONAL_COMPONENTS Positioning Location)
find_package(Dtk6 REQUIRED COMPONENTS Core Widget)
find_package(DDEShell REQUIRED)

# Create plugin library
add_library(ds-weather-applet SHARED
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
    Qt6::Network
    Qt6::Quick
    Qt6::QuickControls2
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

# Build the dde-shell package tree, but exclude VCS metadata from installation.
ds_build_package(
    PACKAGE ${DS_WEATHER_PACKAGE_ID}
    TARGET ds-weather-applet
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
