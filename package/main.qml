// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

AppletItem {
    id: root
    objectName: "weather applet"
    
    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 10
    
    property int dockSize: Panel.rootObject.dockSize || 48
    
    implicitWidth: useColumnLayout ? dockSize : 180
    implicitHeight: dockSize
    
    // Loading indicator
    BusyIndicator {
        anchors.centerIn: parent
        running: Applet.weather.isLoading
        visible: running
        width: 20
        height: 20
    }
    
    // Error message
    Text {
        anchors.centerIn: parent
        text: Applet.weather.hasError ? Applet.weather.errorMessage : ""
        color: themeColors.text
        font.pixelSize: 10
        visible: Applet.weather.hasError
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        width: parent.width - 10
    }
    
    // Main content: Left icon + Right info (two rows)
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        spacing: 2
        visible: !Applet.weather.isLoading && !Applet.weather.hasError
        
        // Left: Weather icon
        WeatherIcon {
            id: weatherIcon
            Layout.preferredWidth: dockSize - 8
            Layout.preferredHeight: dockSize - 8
            weatherCode: Applet.weather.weatherCode
            iconColor: themeColors.icon
            Layout.alignment: Qt.AlignVCenter
        }
        
        // Right: Two rows
        ColumnLayout {
            spacing: 0
            
            // Row 1: Location + High/Low temp
            RowLayout {
                spacing: 4
                
                Text {
                    id: cityLabel
                    text: Applet.weather.city
                    font.pixelSize: 10
                    color: themeColors.text
                    elide: Text.ElideRight
                }
                
                Text {
                    id: tempRangeLabel
                    text: qsTr("%1°/%2°").arg(Math.round(Applet.weather.temperatureMin)).arg(Math.round(Applet.weather.temperatureMax))
                    font.pixelSize: 10
                    color: themeColors.text
                    opacity: 0.8
                }
            }
            
            // Row 2: Current temp + Weather description
            RowLayout {
                spacing: 4
                
                Text {
                    id: tempLabel
                    text: qsTr("%1°C").arg(Math.round(Applet.weather.temperature))
                    font.pixelSize: 14
                    font.bold: true
                    color: themeColors.text
                }
                
                Text {
                    id: descLabel
                    text: Applet.weather.weatherDescription
                    font.pixelSize: 10
                    color: themeColors.text
                    opacity: 0.8
                    elide: Text.ElideRight
                }
            }
        }
    }
    
    // Theme colors
    QtObject {
        id: themeColors
        readonly property color text: Qt.rgba(1, 1, 1, 0.9)
        readonly property color icon: Qt.rgba(1, 1, 1, 0.95)
    }
}
