// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15

Item {
    id: root
    
    property string weatherCode: "0"
    property string iconNameOverride: ""
    property color iconColor: "white"
    readonly property string iconName: iconNameOverride.length > 0 ? iconNameOverride : getIconName(weatherCode)
    readonly property real sourceCanvasWidth: 56
    readonly property real sourceCanvasHeight: 48
    // Measured from the rendered SVGs. We center the visible content, not the
    // raw 56x48 canvas, because these assets carry asymmetric transparent pads.
    readonly property rect trimRect: getTrimRect(iconName)
    readonly property real scaleFactor: Math.min(width / trimRect.width,
                                                  height / trimRect.height)
    
    implicitWidth: 48
    implicitHeight: 48
    clip: true
    
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

    function makeTrimRect(left, top, rectWidth, rectHeight) {
        // A small padding avoids clipping strokes after scaling.
        var padding = 0.5
        var x = Math.max(0, left - padding)
        var y = Math.max(0, top - padding)
        var right = Math.min(sourceCanvasWidth, left + rectWidth + padding)
        var bottom = Math.min(sourceCanvasHeight, top + rectHeight + padding)
        return Qt.rect(x, y, right - x, bottom - y)
    }

    // These SVGs include significant transparent margins. Center the visible
    // content instead of the raw 56x48 canvas to keep icon/text spacing tight.
    function getTrimRect(name) {
        switch (name) {
        case "clear-day":
            return makeTrimRect(0.5, 1, 31.5, 33)
        case "clear-night":
            return makeTrimRect(31.5, 2.5, 21.5, 23)
        case "cloudy-1-day":
        case "cloudy-2-day":
            return makeTrimRect(0.5, 1, 48.5, 39.5)
        case "cloudy-1-night":
        case "cloudy-2-night":
        case "cloudy-3-night":
            return makeTrimRect(11.5, 1, 41, 40)
        case "fog":
            return makeTrimRect(3, 17, 49, 20.5)
        case "fog-night":
            return makeTrimRect(7, 2.5, 45.5, 35)
        case "rainy-1":
            return makeTrimRect(7, 7.5, 42, 37)
        case "rainy-1-night":
            return makeTrimRect(7, 2.5, 45.5, 42)
        case "rainy-2":
            return makeTrimRect(7, 7.5, 42, 38.5)
        case "rainy-2-night":
            return makeTrimRect(7, 2.5, 45.5, 43)
        case "rainy-3":
            return makeTrimRect(7, 7.5, 42, 40.5)
        case "rainy-3-night":
            return makeTrimRect(7, 2.5, 45.5, 45)
        case "snowy-1":
        case "snowy-2":
            return makeTrimRect(7, 7.5, 42, 38)
        case "snowy-1-night":
        case "snowy-2-night":
            return makeTrimRect(7, 2.5, 45.5, 42.5)
        case "scattered-thunderstorms-night":
            return makeTrimRect(7, 2.5, 45.5, 45.5)
        case "thunderstorms":
            return makeTrimRect(7, 4.5, 42, 43.5)
        case "cloudy":
        default:
            return makeTrimRect(7, 2.5, 42, 38)
        }
    }
    
    Image {
        id: weatherIcon
        width: Math.round(root.sourceCanvasWidth * root.scaleFactor)
        height: Math.round(root.sourceCanvasHeight * root.scaleFactor)
        // Position the full source canvas so the trimmed bounds land in the
        // center of the WeatherIcon item.
        x: Math.round((root.width - root.trimRect.width * root.scaleFactor) / 2
                      - root.trimRect.x * root.scaleFactor)
        y: Math.round((root.height - root.trimRect.height * root.scaleFactor) / 2
                      - root.trimRect.y * root.scaleFactor)
        source: Qt.resolvedUrl("icons/" + root.iconName + ".svg")
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
        asynchronous: true
        
        onStatusChanged: {
            if (status === Image.Error) {
                source = Qt.resolvedUrl("icons/cloudy.svg")
            }
        }
    }
}
