// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1 as LP
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

    PanelToolTip {
        id: toolTip
        text: Applet.weather.tooltipText || ""
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }

    Timer {
        id: toolTipShowTimer
        interval: 50
        onTriggered: {
            const point = root.mapToItem(null, root.width / 2, root.height / 2)
            toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
            toolTip.open()
        }
    }

    HoverHandler {
        onHoveredChanged: {
            if (hovered && toolTip.text.length > 0) {
                toolTipShowTimer.start()
            } else {
                if (toolTipShowTimer.running) {
                    toolTipShowTimer.stop()
                }
                toolTip.close()
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        preventStealing: true
        onClicked: function(mouse) {
            toolTip.close()
            locationMenuLoader.active = true
            locationMenuLoader.item.open()
            mouse.accepted = true
        }
        onPressed: function(mouse) {
            mouse.accepted = true
        }
    }
    
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
    
    // Main content: use explicit anchors so the icon/text gap stays stable
    // after the icon is visually trimmed inside WeatherIcon.
    Item {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        visible: !Applet.weather.isLoading && !Applet.weather.hasError
        
        // Left: Weather icon
        Item {
            id: iconSlot
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(24, dockSize - 22)
            height: parent.height

            WeatherIcon {
                id: weatherIcon
                anchors.centerIn: parent
                anchors.verticalCenterOffset: 1
                width: Math.min(parent.height - 2, parent.width - 2)
                height: width
                weatherCode: Applet.weather.weatherCode
                iconColor: themeColors.icon
            }
        }
        
        // Right: Two rows
        ColumnLayout {
            anchors.left: iconSlot.right
            // Keep a fixed visual gap from the icon's visible edge instead of
            // relying on RowLayout spacing against the icon canvas width.
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
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
                    text: Applet.weather.formattedTemperatureRange
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
                    text: Applet.weather.formattedTemperature
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

    Loader {
        id: locationMenuLoader
        active: false
        sourceComponent: LP.Menu {
            LP.MenuItem {
                text: qsTr("Location Settings")
                onTriggered: {
                    Applet.showSettings()
                }
            }
        }
    }
}
