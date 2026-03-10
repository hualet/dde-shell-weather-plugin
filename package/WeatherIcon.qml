// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15

Item {
    id: root
    
    property string weatherCode: "0"
    property color iconColor: "white"
    
    implicitWidth: 48
    implicitHeight: 48
    
    // Map WMO weather code to Makin-Things icon name
    function getIconName(wmoCode) {
        var code = parseInt(wmoCode) || 0
        
        // Clear sky
        if (code === 0) return "clear-day"
        // Mainly clear
        if (code === 1) return "cloudy-1-day"
        // Partly cloudy
        if (code === 2) return "cloudy-2-day"
        // Overcast
        if (code === 3) return "cloudy"
        // Fog
        if (code >= 45 && code <= 48) return "fog"
        // Drizzle / Light rain
        if (code >= 51 && code <= 53) return "rainy-1"
        if (code >= 55 && code <= 57) return "rainy-2"
        // Rain
        if (code >= 61 && code <= 63) return "rainy-3"
        if (code >= 64 && code <= 67) return "rainy-3"
        // Snow
        if (code >= 71 && code <= 73) return "snowy-1"
        if (code >= 74 && code <= 77) return "snowy-2"
        // Rain showers
        if (code >= 80 && code <= 81) return "rainy-1"
        if (code >= 82) return "rainy-2"
        // Snow showers
        if (code >= 85 && code <= 86) return "snowy-1"
        // Thunderstorm
        if (code >= 95 && code <= 99) return "thunderstorms"
        
        return "cloudy"
    }
    
    Image {
        id: weatherIcon
        anchors.fill: parent
        source: Qt.resolvedUrl("icons/" + getIconName(weatherCode) + ".svg")
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
        asynchronous: true
        sourceSize.width: 64
        sourceSize.height: 64
        
        onStatusChanged: {
            if (status === Image.Error) {
                source = Qt.resolvedUrl("icons/cloudy.svg")
            }
        }
    }
}
