// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    
    property string weatherCode: "0"
    property bool animated: true
    
    implicitWidth: 80
    implicitHeight: 80
    
    // Weather icon container
    Item {
        id: iconContainer
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        
        // Sun (clear sky)
        Item {
            id: sunIcon
            anchors.centerIn: parent
            width: 60
            height: 60
            visible: getWeatherType() === "clear"
            
            // Sun circle
            Rectangle {
                id: sunCircle
                anchors.centerIn: parent
                width: 36
                height: 36
                radius: 18
                color: "#FFD700"
                
                // Sun rays
                Repeater {
                    model: 8
                    
                    Rectangle {
                        width: 4
                        height: 14
                        radius: 2
                        color: "#FFD700"
                        x: sunCircle.x + sunCircle.width/2 - width/2
                        y: sunCircle.y + sunCircle.height/2 - height/2
                        transformOrigin: Item.Center
                        rotation: index * 45
                        
                        // Animation for rays
                        SequentialAnimation on opacity {
                            running: root.animated && sunIcon.visible
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.6; to: 1.0; duration: 1000 }
                            NumberAnimation { from: 1.0; to: 0.6; duration: 1000 }
                        }
                    }
                }
                
                // Sun glow animation
                SequentialAnimation on scale {
                    running: root.animated && sunIcon.visible
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 1.1; duration: 2000; easing.type: Easing.InOutSine }
                    NumberAnimation { from: 1.1; to: 1.0; duration: 2000; easing.type: Easing.InOutSine }
                }
            }
        }
        
        // Cloud (partly cloudy / overcast)
        Item {
            id: cloudIcon
            anchors.centerIn: parent
            width: 70
            height: 50
            visible: getWeatherType() === "cloudy"
            
            // Main cloud
            Rectangle {
                id: cloud1
                width: 50
                height: 30
                radius: 15
                color: "white"
                opacity: 0.9
                anchors.centerIn: parent
            }
            
            // Cloud puff 1
            Rectangle {
                width: 25
                height: 25
                radius: 12.5
                color: "white"
                opacity: 0.9
                anchors.left: cloud1.left
                anchors.top: cloud1.top
                anchors.topMargin: -10
            }
            
            // Cloud puff 2
            Rectangle {
                width: 30
                height: 30
                radius: 15
                color: "white"
                opacity: 0.9
                anchors.right: cloud1.right
                anchors.top: cloud1.top
                anchors.topMargin: -15
            }
            
            // Cloud drift animation
            SequentialAnimation on x {
                running: root.animated && cloudIcon.visible
                loops: Animation.Infinite
                NumberAnimation { from: -5; to: 5; duration: 3000; easing.type: Easing.InOutSine }
                NumberAnimation { from: 5; to: -5; duration: 3000; easing.type: Easing.InOutSine }
            }
        }
        
        // Rain (rain / drizzle)
        Item {
            id: rainIcon
            anchors.centerIn: parent
            width: 70
            height: 60
            visible: getWeatherType() === "rain"
            
            // Cloud
            Rectangle {
                id: rainCloud
                width: 50
                height: 25
                radius: 12.5
                color: "#95A5A6"
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            // Rain drops
            Repeater {
                model: 5
                
                Rectangle {
                    width: 3
                    height: 12
                    radius: 1.5
                    color: "#3498DB"
                    x: rainCloud.x + 10 + index * 10
                    y: rainCloud.y + rainCloud.height + 5
                    
                    // Rain drop animation
                    SequentialAnimation on y {
                        running: root.animated && rainIcon.visible
                        loops: Animation.Infinite
                        NumberAnimation { 
                            from: rainCloud.y + rainCloud.height + 5
                            to: rainCloud.y + rainCloud.height + 25
                            duration: 800 + index * 100
                        }
                        PropertyAction { value: rainCloud.y + rainCloud.height + 5 }
                    }
                    
                    // Fade out at bottom
                    SequentialAnimation on opacity {
                        running: root.animated && rainIcon.visible
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 0.0; duration: 800 + index * 100 }
                        PropertyAction { value: 1.0 }
                    }
                }
            }
        }
        
        // Snow (snow)
        Item {
            id: snowIcon
            anchors.centerIn: parent
            width: 70
            height: 60
            visible: getWeatherType() === "snow"
            
            // Cloud
            Rectangle {
                id: snowCloud
                width: 50
                height: 25
                radius: 12.5
                color: "#B0BEC5"
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            // Snowflakes
            Repeater {
                model: 6
                
                Text {
                    text: "❄"
                    font.pixelSize: 12
                    color: "white"
                    x: snowCloud.x + 5 + index * 10
                    y: snowCloud.y + snowCloud.height + 5
                    
                    // Snowflake animation
                    SequentialAnimation on y {
                        running: root.animated && snowIcon.visible
                        loops: Animation.Infinite
                        NumberAnimation { 
                            from: snowCloud.y + snowCloud.height + 5
                            to: snowCloud.y + snowCloud.height + 30
                            duration: 1500 + index * 200
                        }
                        PropertyAction { value: snowCloud.y + snowCloud.height + 5 }
                    }
                    
                    // Rotation animation
                    SequentialAnimation on rotation {
                        running: root.animated && snowIcon.visible
                        loops: Animation.Infinite
                        NumberAnimation { from: 0; to: 360; duration: 3000 }
                    }
                }
            }
        }
        
        // Thunderstorm
        Item {
            id: thunderIcon
            anchors.centerIn: parent
            width: 70
            height: 60
            visible: getWeatherType() === "thunder"
            
            // Cloud
            Rectangle {
                id: thunderCloud
                width: 50
                height: 25
                radius: 12.5
                color: "#546E7A"
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            // Lightning bolt
            Text {
                text: "⚡"
                font.pixelSize: 24
                color: "#FFD700"
                anchors.top: thunderCloud.bottom
                anchors.topMargin: 5
                anchors.horizontalCenter: parent.horizontalCenter
                
                // Flash animation
                SequentialAnimation on opacity {
                    running: root.animated && thunderIcon.visible
                    loops: Animation.Infinite
                    PropertyAction { value: 1.0 }
                    PauseAnimation { duration: 200 }
                    PropertyAction { value: 0.0 }
                    PauseAnimation { duration: 800 }
                    PropertyAction { value: 1.0 }
                    PauseAnimation { duration: 100 }
                    PropertyAction { value: 0.0 }
                    PauseAnimation { duration: 1500 }
                }
                
                // Scale animation
                SequentialAnimation on scale {
                    running: root.animated && thunderIcon.visible
                    loops: Animation.Infinite
                    PropertyAction { value: 1.0 }
                    PauseAnimation { duration: 200 }
                    NumberAnimation { from: 0.8; to: 1.2; duration: 100 }
                    NumberAnimation { from: 1.2; to: 1.0; duration: 100 }
                    PauseAnimation { duration: 2200 }
                }
            }
        }
        
        // Fog
        Item {
            id: fogIcon
            anchors.centerIn: parent
            width: 70
            height: 50
            visible: getWeatherType() === "fog"
            
            // Fog lines
            Repeater {
                model: 4
                
                Rectangle {
                    width: 60
                    height: 3
                    radius: 1.5
                    color: "white"
                    opacity: 0.6 - index * 0.1
                    y: 10 + index * 12
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    // Fog drift animation
                    SequentialAnimation on x {
                        running: root.animated && fogIcon.visible
                        loops: Animation.Infinite
                        NumberAnimation { from: -10; to: 10; duration: 4000 + index * 500 }
                        NumberAnimation { from: 10; to: -10; duration: 4000 + index * 500 }
                    }
                }
            }
        }
    }
    
    // Helper function to determine weather type
    function getWeatherType() {
        var code = parseInt(weatherCode) || 0
        
        // Clear sky
        if (code === 0) return "clear"
        
        // Partly cloudy / overcast
        if (code >= 1 && code <= 3) return "cloudy"
        
        // Fog
        if (code >= 45 && code <= 48) return "fog"
        
        // Drizzle / Rain / Rain showers
        if ((code >= 51 && code <= 57) || (code >= 61 && code <= 67) || (code >= 80 && code <= 82)) return "rain"
        
        // Snow / Snow showers
        if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) return "snow"
        
        // Thunderstorm
        if (code >= 95 && code <= 99) return "thunder"
        
        // Default to cloudy
        return "cloudy"
    }
}
