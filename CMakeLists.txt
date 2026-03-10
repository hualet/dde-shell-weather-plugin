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
    weatherapplet.h
    weatherapplet.cpp
    weatherprovider.h
    weatherprovider.cpp
)

target_include_directories(ds-weather-applet PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
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

# Install plugin package using dde-shell macro
ds_install_package(
    PACKAGE org.deepin.ds.weather
    TARGET ds-weather-applet
)
