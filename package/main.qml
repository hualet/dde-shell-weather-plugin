// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Particles 2.15
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

AppletItem {
    id: root
    objectName: "weather applet"
    
    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 10

    
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : 280
    implicitHeight: useColumnLayout ? 140 : Panel.rootObject.dockSize
    
    // Background with gradient
    Rectangle {
        id: background
        anchors.fill: parent
        radius: useColumnLayout ? Panel.rootObject.dockSize / 2 : 12
        
        // Dynamic gradient based on weather
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: getTopGradientColor()
            }
            GradientStop {
                position: 1.0
                color: getBottomGradientColor()
            }
        }
        
        // Particle system for weather effects
        ParticleWeather {
            id: particleSystem
            anchors.fill: parent
            weatherCode: Applet.weather.weatherCode
            visible: !Applet.weather.isLoading
        }
        
        // Loading indicator
        BusyIndicator {
            anchors.centerIn: parent
            running: Applet.weather.isLoading
            visible: running
            width: 48
            height: 48
        }
        
        // Error message
        Text {
            anchors.centerIn: parent
            text: Applet.weather.hasError ? Applet.weather.errorMessage : ""
            color: "white"
            font.pixelSize: 14
            visible: Applet.weather.hasError
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: parent.width - 40
        }
        
        // Main content layout
        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 16
            visible: !Applet.weather.isLoading && !Applet.weather.hasError
            
            // Left: Weather icon
            WeatherIcon {
                id: weatherIcon
                Layout.preferredWidth: useColumnLayout ? Panel.rootObject.dockSize * 0.6 : 80
                Layout.preferredHeight: useColumnLayout ? Panel.rootObject.dockSize * 0.6 : 80
                weatherCode: Applet.weather.weatherCode
                animated: true
            }
            
            // Right: Weather information
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8
                
                // City name
                Label {
                    id: cityLabel
                    text: Applet.weather.city
                    font.pixelSize: useColumnLayout ? 12 : 18
                    font.bold: true
                    color: "white"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    visible: !useColumnLayout
                }
                
                // Temperature
                Label {
                    id: tempLabel
                    text: qsTr("%1°C").arg(Math.round(Applet.weather.temperature))
                    font.pixelSize: useColumnLayout ? 24 : 36
                    font.bold: true
                    color: "white"
                }
                
                // Weather description
                Label {
                    id: descLabel
                    text: Applet.weather.weatherDescription
                    font.pixelSize: useColumnLayout ? 10 : 14
                    color: "white"
                    opacity: 0.9
                    visible: !useColumnLayout
                }
                
                // Additional info row
                RowLayout {
                    spacing: 12
                    visible: !useColumnLayout
                    
                    // Humidity
                    Row {
                        spacing: 4
                        Text {
                            text: "💧"
                            font.pixelSize: 12
                        }
                        Text {
                            text: qsTr("%1%").arg(Math.round(Applet.weather.humidity))
                            font.pixelSize: 12
                            color: "white"
                            opacity: 0.8
                        }
                    }
                    
                    // Wind speed
                    Row {
                        spacing: 4
                        Text {
                            text: "💨"
                            font.pixelSize: 12
                        }
                        Text {
                            text: qsTr("%1 km/h").arg(Math.round(Applet.weather.windSpeed))
                            font.pixelSize: 12
                            color: "white"
                            opacity: 0.8
                        }
                    }
                }
            }
        }
        
        // Refresh button
        Rectangle {
            id: refreshButton
            width: 32
            height: 32
            radius: 16
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 8
            color: refreshMouseArea.containsMouse ? "white" : "transparent"
            opacity: refreshMouseArea.containsMouse ? 0.3 : 0.5
            visible: !useColumnLayout
            
            Text {
                anchors.centerIn: parent
                text: "↻"
                font.pixelSize: 18
                color: "white"
            }
            
            MouseArea {
                id: refreshMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                
                onClicked: {
                    Applet.weather.refresh()
                }
            }
        }
    }
    
    // Helper functions for gradient colors
    function getTopGradientColor() {
        var code = parseInt(Applet.weather.weatherCode) || 0
        
        // Clear sky - sunny
        if (code === 0) return "#4A90E2"
        // Partly cloudy
        if (code >= 1 && code <= 3) return "#5DADE2"
        // Fog
        if (code >= 45 && code <= 48) return "#7F8C8D"
        // Drizzle or Rain
        if ((code >= 51 && code <= 57) || (code >= 61 && code <= 67) || (code >= 80 && code <= 82)) return "#546E7A"
        // Snow
        if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) return "#B0BEC5"
        // Thunderstorm
        if (code >= 95 && code <= 99) return "#37474F"
        
        // Default
        return "#4A90E2"
    }
    
    function getBottomGradientColor() {
        var code = parseInt(Applet.weather.weatherCode) || 0
        
        // Clear sky - sunny
        if (code === 0) return "#87CEEB"
        // Partly cloudy
        if (code >= 1 && code <= 3) return "#85C1E9"
        // Fog
        if (code >= 45 && code <= 48) return "#95A5A6"
        // Drizzle or Rain
        if ((code >= 51 && code <= 57) || (code >= 61 && code <= 67) || (code >= 80 && code <= 82)) return "#607D8B"
        // Snow
        if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) return "#CFD8DC"
        // Thunderstorm
        if (code >= 95 && code <= 99) return "#455A64"
        
        // Default
        return "#87CEEB"
    }
}
