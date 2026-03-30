// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds.weather 1.0

Item {
    id: root

    property string weatherCode: "0"
    property string iconNameOverride: ""
    property color iconColor: "white"
    property bool loading: false
    property alias animated: weatherIcon.animated
    readonly property string iconName: iconNameOverride.length > 0 ? iconNameOverride : getIconName(weatherCode)

    implicitWidth: 48
    implicitHeight: 48
    clip: true

    // Map WMO weather code to icon name
    function getIconName(wmoCode) {
        var code = parseInt(wmoCode) || 0

        if (code === 0) return "clear-day"
        if (code === 1) return "cloudy-1-day"
        if (code === 2) return "cloudy-2-day"
        if (code === 3) return "cloudy"
        if (code >= 45 && code <= 48) return "fog"
        if (code >= 51 && code <= 53) return "rainy-1"
        if (code >= 55 && code <= 57) return "rainy-2"
        if (code >= 61 && code <= 63) return "rainy-3"
        if (code >= 64 && code <= 67) return "rainy-3"
        if (code >= 71 && code <= 73) return "snowy-1"
        if (code >= 74 && code <= 77) return "snowy-2"
        if (code >= 80 && code <= 81) return "rainy-1"
        if (code >= 82) return "rainy-2"
        if (code >= 85 && code <= 86) return "snowy-1"
        if (code >= 95 && code <= 99) return "thunderstorms"

        return "cloudy"
    }

    AnimatedSvgItem {
        id: weatherIcon
        anchors.fill: parent
        source: Qt.resolvedUrl("icons/" + root.iconName + ".svg")
        opacity: root.loading ? 0 : 1
        visible: opacity > 0

        Behavior on opacity {
            NumberAnimation {
                duration: 160
                easing.type: Easing.OutCubic
            }
        }
    }

    BusyIndicator {
        id: refreshIndicator
        anchors.centerIn: parent
        width: Math.max(18, Math.min(root.width, root.height) * 0.72)
        height: width
        running: root.loading
        opacity: root.loading ? 1 : 0
        visible: running || opacity > 0

        Behavior on opacity {
            NumberAnimation {
                duration: 160
                easing.type: Easing.OutCubic
            }
        }
    }
}
