// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1 as LP
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0

AppletItem {
    id: root
    objectName: "weather applet"
    
    property bool useColumnLayout: Panel.position % 2
    readonly property bool useClassicTaskbarLayout: Panel.itemAlignment === Dock.LeftAlignment
    property int dockOrder: useClassicTaskbarLayout ? 21 : 10
    
    property int dockSize: Panel.rootObject.dockSize || 48
    readonly property int horizontalAppletMaxWidth: 180
    readonly property var hourlyForecastEntries: Applet.weather.hourlyForecast || []
    readonly property int forecastCellWidth: 60
    readonly property int forecastCellSpacing: 14
    readonly property int forecastWheelStep: forecastCellWidth + forecastCellSpacing
    readonly property int forecastEdgeInset: 20
    readonly property int temperatureChartHeight: 82
    readonly property int hourlyPopupViewportWidth: useColumnLayout ? 320 : 520
    readonly property bool hasHourlyForecast: hourlyForecastEntries.length > 0
    readonly property real forecastTrackWidth: hasHourlyForecast
                                              ? forecastCellWidth * hourlyForecastEntries.length
                                                + forecastCellSpacing * (hourlyForecastEntries.length - 1)
                                              : 0
    readonly property real hourlyPopupContentWidth: forecastTrackWidth + forecastEdgeInset * 2
    property Palette textPalette: DockPalette.iconTextPalette
    property Palette chartLinePalette: Palette {
        normal {
            common: Qt.rgba(36 / 255, 118 / 255, 220 / 255, 0.78)
        }
        normalDark {
            common: Qt.rgba(186 / 255, 231 / 255, 1, 0.92)
        }
    }
    property Palette chartPointPalette: Palette {
        normal {
            common: Qt.rgba(24 / 255, 88 / 255, 182 / 255, 0.92)
        }
        normalDark {
            common: Qt.rgba(1, 1, 1, 0.95)
        }
    }
    readonly property real hourlyTemperatureMin: {
        if (!hasHourlyForecast) {
            return 0
        }

        let minTemperature = hourlyForecastEntries[0].temperature
        for (let index = 1; index < hourlyForecastEntries.length; ++index) {
            minTemperature = Math.min(minTemperature, hourlyForecastEntries[index].temperature)
        }
        return minTemperature
    }
    readonly property real hourlyTemperatureMax: {
        if (!hasHourlyForecast) {
            return 0
        }

        let maxTemperature = hourlyForecastEntries[0].temperature
        for (let index = 1; index < hourlyForecastEntries.length; ++index) {
            maxTemperature = Math.max(maxTemperature, hourlyForecastEntries[index].temperature)
        }
        return maxTemperature
    }
    
    implicitWidth: useColumnLayout ? dockSize : Math.min(horizontalAppletMaxWidth,
                                                         weatherSummary.implicitWidth)
    implicitHeight: dockSize

    function forecastCenterX(index) {
        return forecastCellWidth / 2 + index * (forecastCellWidth + forecastCellSpacing)
    }

    function forecastPointY(temperature) {
        const chartTopPadding = 26
        const chartBottomPadding = 16
        const usableHeight = Math.max(0, temperatureChartHeight - chartTopPadding - chartBottomPadding)
        const range = hourlyTemperatureMax - hourlyTemperatureMin
        if (range <= 0.01) {
            return chartTopPadding + usableHeight / 2
        }

        return chartTopPadding + (hourlyTemperatureMax - temperature) / range * usableHeight
    }

    function temperatureLabelY(index, labelHeight) {
        const pointY = forecastPointY(hourlyForecastEntries[index].temperature)
        const preferredTop = pointY - labelHeight - 8
        if (preferredTop >= 0) {
            return preferredTop
        }

        return Math.min(temperatureChartHeight - labelHeight,
                        pointY + 8)
    }

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
            if (hovered && !hourlyPopup.popupVisible && toolTip.text.length > 0) {
                toolTipShowTimer.start()
            } else {
                if (toolTipShowTimer.running) {
                    toolTipShowTimer.stop()
                }
                toolTip.close()
            }
        }
    }

    PanelPopup {
        id: hourlyPopup
        width: hourlyPopupContainer.implicitWidth
        height: hourlyPopupContainer.implicitHeight
        popupX: DockPanelPositioner.x
        popupY: DockPanelPositioner.y

        onPopupVisibleChanged: {
            if (popupVisible) {
                toolTip.close()
            }
        }

        Control {
            id: hourlyPopupContainer
            padding: 14
            implicitWidth: Math.min(root.hourlyPopupContentWidth, root.hourlyPopupViewportWidth) + leftPadding + rightPadding
            implicitHeight: hourlyForecastFlick.implicitHeight + topPadding + bottomPadding

            contentItem: Flickable {
                id: hourlyForecastFlick
                implicitWidth: Math.min(root.hourlyPopupContentWidth,
                                        root.hourlyPopupViewportWidth)
                implicitHeight: forecastPopupContent.height
                contentWidth: Math.max(width, root.hourlyPopupContentWidth)
                contentHeight: forecastPopupContent.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick
                interactive: contentWidth > width

                ScrollBar.horizontal: ScrollBar {
                    policy: hourlyForecastFlick.contentWidth > hourlyForecastFlick.width ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton

                    onWheel: function(wheel) {
                        if (hourlyForecastFlick.contentWidth <= hourlyForecastFlick.width) {
                            return
                        }

                        let movement = 0
                        if (wheel.pixelDelta.x !== 0) {
                            movement = wheel.pixelDelta.x
                        } else if (wheel.angleDelta.x !== 0) {
                            movement = wheel.angleDelta.x / 120 * root.forecastWheelStep
                        } else if (wheel.angleDelta.y !== 0) {
                            movement = -wheel.angleDelta.y / 120 * root.forecastWheelStep
                        }

                        if (movement === 0) {
                            return
                        }

                        const maxContentX = Math.max(0, hourlyForecastFlick.contentWidth - hourlyForecastFlick.width)
                        hourlyForecastFlick.contentX = Math.max(0,
                                                                Math.min(maxContentX,
                                                                         hourlyForecastFlick.contentX + movement))
                        wheel.accepted = true
                    }
                }

                Item {
                    id: forecastPopupContent
                    width: hourlyForecastFlick.contentWidth
                    height: forecastColumn.implicitHeight

                    Column {
                        id: forecastColumn
                        x: Math.max(root.forecastEdgeInset,
                                    (forecastPopupContent.width - width) / 2)
                        spacing: 12

                        Item {
                            id: temperatureChart
                            width: root.forecastTrackWidth
                            height: root.temperatureChartHeight

                            Canvas {
                                id: temperatureCurveCanvas
                                anchors.fill: parent

                                onPaint: {
                                    const ctx = getContext("2d")
                                    ctx.clearRect(0, 0, width, height)

                                    if (!root.hasHourlyForecast) {
                                        return
                                    }

                                    ctx.lineWidth = 2
                                    ctx.strokeStyle = themeColors.chartLine
                                    ctx.beginPath()

                                    for (let index = 0; index < root.hourlyForecastEntries.length; ++index) {
                                        const entry = root.hourlyForecastEntries[index]
                                        const x = root.forecastCenterX(index)
                                        const y = root.forecastPointY(entry.temperature)

                                        if (index === 0) {
                                            ctx.moveTo(x, y)
                                        } else {
                                            ctx.lineTo(x, y)
                                        }
                                    }

                                    ctx.stroke()

                                    ctx.fillStyle = themeColors.chartPoint
                                    for (let index = 0; index < root.hourlyForecastEntries.length; ++index) {
                                        const entry = root.hourlyForecastEntries[index]
                                        const x = root.forecastCenterX(index)
                                        const y = root.forecastPointY(entry.temperature)

                                        ctx.beginPath()
                                        ctx.arc(x, y, 3.5, 0, Math.PI * 2)
                                        ctx.fill()
                                    }
                                }

                                Connections {
                                    target: root
                                    function onHourlyForecastEntriesChanged() {
                                        temperatureCurveCanvas.requestPaint()
                                    }
                                    function onHourlyTemperatureMinChanged() {
                                        temperatureCurveCanvas.requestPaint()
                                    }
                                    function onHourlyTemperatureMaxChanged() {
                                        temperatureCurveCanvas.requestPaint()
                                    }
                                }

                                Connections {
                                    target: themeColors
                                    function onChartLineChanged() {
                                        temperatureCurveCanvas.requestPaint()
                                    }
                                    function onChartPointChanged() {
                                        temperatureCurveCanvas.requestPaint()
                                    }
                                }

                                onWidthChanged: requestPaint()
                                onHeightChanged: requestPaint()
                            }

                            Repeater {
                                model: root.hourlyForecastEntries

                                delegate: Text {
                                    required property int index
                                    required property var modelData

                                    x: root.forecastCenterX(index) - width / 2
                                    y: root.temperatureLabelY(index, height)
                                    text: modelData.formattedTemperature || ""
                                    color: themeColors.primaryText
                                    font.pixelSize: 10
                                    font.bold: false
                                    horizontalAlignment: Text.AlignHCenter
                                    renderType: Text.NativeRendering
                                }
                            }
                        }

                        Row {
                            id: hourlyForecastRow
                            spacing: root.forecastCellSpacing

                            Repeater {
                                model: root.hourlyForecastEntries

                                delegate: Column {
                                    required property int index
                                    required property var modelData

                                    width: root.forecastCellWidth
                                    spacing: 8

                                    WeatherIcon {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        width: 34
                                        height: 34
                                        weatherCode: modelData.weatherCode || ""
                                        iconNameOverride: modelData.iconName || ""
                                        iconColor: themeColors.icon
                                    }

                                    Text {
                                        width: parent.width
                                        text: index === 0 ? qsTr("Now") : (modelData.displayHour || "")
                                        color: themeColors.secondaryText
                                        font.pixelSize: 10
                                        horizontalAlignment: Text.AlignHCenter
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Component.onCompleted: {
            DockPanelPositioner.bounding = Qt.binding(function () {
                const point = root.mapToItem(null, root.width / 2, root.height / 2)
                return Qt.rect(point.x, point.y, hourlyPopup.width, hourlyPopup.height)
            })
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        gesturePolicy: TapHandler.ReleaseWithinBounds

        onTapped: {
            if (!root.hasHourlyForecast) {
                return
            }

            if (hourlyPopup.popupVisible) {
                hourlyPopup.close()
            } else {
                Panel.requestClosePopup()
                hourlyPopup.open()
            }

            toolTip.close()
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        preventStealing: true
        onClicked: function(mouse) {
            hourlyPopup.close()
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
        color: themeColors.primaryText
        font.pixelSize: 10
        visible: Applet.weather.hasError
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        width: parent.width - 10
    }
    
    // Main content: use explicit anchors so the icon/text gap stays stable
    // after the icon is visually trimmed inside WeatherIcon.
    Item {
        id: weatherSummary
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        visible: !Applet.weather.isLoading && !Applet.weather.hasError
        readonly property int horizontalPadding: anchors.leftMargin + anchors.rightMargin
        readonly property int contentSpacing: 4
        readonly property int textColumnMaxWidth: Math.max(0,
                                                           root.horizontalAppletMaxWidth
                                                           - horizontalPadding
                                                           - iconSlot.width
                                                           - contentSpacing)
        implicitWidth: horizontalPadding + iconSlot.width + contentSpacing
                       + textColumn.implicitWidth
        
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
                iconNameOverride: Applet.weather.iconName
                iconColor: themeColors.icon
            }
        }
        
        // Right: Two rows
        ColumnLayout {
            id: textColumn
            anchors.left: iconSlot.right
            // Keep a fixed visual gap from the icon's visible edge instead of
            // relying on RowLayout spacing against the icon canvas width.
            anchors.leftMargin: weatherSummary.contentSpacing
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0
            
            // Row 1: Location + High/Low temp
            RowLayout {
                id: topRow
                spacing: 4
                
                Text {
                    id: cityLabel
                    text: Applet.weather.city
                    width: Math.min(implicitWidth,
                                    Math.max(0,
                                             weatherSummary.textColumnMaxWidth
                                             - (tempRangeLabel.visible
                                                    ? topRow.spacing + tempRangeLabel.implicitWidth
                                                    : 0)))
                    font.pixelSize: 10
                    color: themeColors.secondaryText
                    elide: Text.ElideRight
                    visible: text.length > 0
                }
                
                Text {
                    id: tempRangeLabel
                    text: Applet.weather.formattedTemperatureRange
                    font.pixelSize: 10
                    color: themeColors.secondaryText
                }
            }
            
            // Row 2: Current temp + Weather description
            RowLayout {
                id: bottomRow
                spacing: 4
                
                Text {
                    id: tempLabel
                    text: Applet.weather.formattedTemperature
                    font.pixelSize: 14
                    font.bold: false
                    color: themeColors.primaryText
                }
                
                Text {
                    id: descLabel
                    text: Applet.weather.weatherDescription
                    width: Math.min(implicitWidth,
                                    Math.max(0,
                                             weatherSummary.textColumnMaxWidth
                                             - (tempLabel.visible
                                                    ? bottomRow.spacing + tempLabel.implicitWidth
                                                    : 0)))
                    font.pixelSize: 10
                    color: themeColors.secondaryText
                    elide: Text.ElideRight
                    visible: text.length > 0
                }
            }
        }
    }
    
    // Theme colors
    QtObject {
        id: themeColors
        readonly property color text: root.ColorSelector.textPalette
        readonly property color primaryText: Qt.rgba(text.r, text.g, text.b, 0.82)
        readonly property color secondaryText: Qt.rgba(text.r, text.g, text.b, 0.68)
        readonly property color icon: text
        readonly property color chartLine: root.ColorSelector.chartLinePalette
        readonly property color chartPoint: root.ColorSelector.chartPointPalette
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
