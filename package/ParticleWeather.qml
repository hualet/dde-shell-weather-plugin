// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Particles 2.15

Item {
    id: root
    
    property string weatherCode: "0"
    property bool active: visible
    
    // Particle system
    ParticleSystem {
        id: particleSystem
        anchors.fill: parent
        running: root.active
        
        // Image particle for rain
        ImageParticle {
            id: rainParticle
            groups: ["rain"]
            source: "qrc:/qtquick/particles/circle.png"
            color: "#87CEEB"
            colorVariation: 0.2
            opacity: 0.6
        }
        
        // Rain emitter
        Emitter {
            id: rainEmitter
            width: parent.width
            height: 1
            anchors.top: parent.top
            anchors.topMargin: -20
            emitRate: getRainRate()
            lifeSpan: 2000
            lifeSpanVariation: 500
            size: 10
            sizeVariation: 5
            enabled: getWeatherType() === "rain" || getWeatherType() === "thunder"
            group: "rain"
            
            velocity: AngleDirection {
                angle: 90
                angleVariation: 10
                magnitude: 200
                magnitudeVariation: 50
            }
        }
        
        // Image particle for snow
        ImageParticle {
            id: snowParticle
            groups: ["snow"]
            source: "qrc:/qtquick/particles/star.png"
            color: "white"
            opacity: 0.8
            rotation: 0
            rotationVariation: 360
            autoRotation: true
        }
        
        // Snow emitter
        Emitter {
            id: snowEmitter
            width: parent.width
            height: 1
            anchors.top: parent.top
            anchors.topMargin: -20
            emitRate: getSnowRate()
            lifeSpan: 5000
            lifeSpanVariation: 2000
            size: 20
            sizeVariation: 10
            enabled: getWeatherType() === "snow"
            group: "snow"
            
            velocity: AngleDirection {
                angle: 90
                angleVariation: 30
                magnitude: 40
                magnitudeVariation: 20
            }
            
            acceleration: PointDirection {
                x: 0
                y: 10
            }
        }
        
        // Image particle for fog
        ImageParticle {
            id: fogParticle
            groups: ["fog"]
            source: "qrc:/qtquick/particles/fuzzyball.png"
            color: "white"
            opacity: 0.15
        }
        
        // Fog emitter
        Emitter {
            id: fogEmitter
            width: parent.width
            height: parent.height
            emitRate: 2
            lifeSpan: 10000
            lifeSpanVariation: 5000
            size: 60
            sizeVariation: 30
            enabled: getWeatherType() === "fog"
            group: "fog"
            
            velocity: PointDirection {
                x: 20
                xVariation: 10
                y: 0
            }
        }
        
        // Image particle for sparkle
        ImageParticle {
            id: sparkleParticle
            groups: ["sparkle"]
            source: "qrc:/qtquick/particles/glow.png"
            color: "#FFD700"
            opacity: 0.8
        }
        
        // Sparkle for clear sky (sun rays)
        Emitter {
            id: sparkleEmitter
            x: parent.width * 0.2
            y: parent.height * 0.2
            width: 40
            height: 40
            emitRate: 3
            lifeSpan: 2000
            lifeSpanVariation: 1000
            size: 10
            sizeVariation: 5
            enabled: getWeatherType() === "clear"
            group: "sparkle"
            
            velocity: AngleDirection {
                angle: 0
                angleVariation: 360
                magnitude: 30
                magnitudeVariation: 20
            }
        }
        
        // Image particle for lightning
        ImageParticle {
            id: lightningParticle
            groups: ["lightning"]
            source: "qrc:/qtquick/particles/circle.png"
            color: "#FFD700"
            opacity: 0.9
        }
        
        // Lightning for thunderstorm
        Emitter {
            id: lightningEmitter
            width: parent.width
            height: 1
            anchors.top: parent.top
            emitRate: 0.2
            lifeSpan: 200
            lifeSpanVariation: 100
            size: 15
            sizeVariation: 10
            enabled: getWeatherType() === "thunder"
            group: "lightning"
            
            velocity: AngleDirection {
                angle: 90
                magnitude: 500
            }
        }
        
        // Turbulence for natural movement
        Turbulence {
            anchors.fill: parent
            strength: 50
            enabled: getWeatherType() === "rain" || getWeatherType() === "snow"
            groups: ["rain", "snow"]
        }
    }
    
    // Helper functions
    function getWeatherType() {
        var code = parseInt(weatherCode) || 0
        
        if (code === 0) return "clear"
        if (code >= 1 && code <= 3) return "cloudy"
        if (code >= 45 && code <= 48) return "fog"
        if ((code >= 51 && code <= 57) || (code >= 61 && code <= 67) || (code >= 80 && code <= 82)) return "rain"
        if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) return "snow"
        if (code >= 95 && code <= 99) return "thunder"
        
        return "cloudy"
    }
    
    function getRainRate() {
        var code = parseInt(weatherCode) || 0
        
        // Light rain
        if (code >= 51 && code <= 53) return 20
        // Moderate rain
        if (code >= 55 && code <= 57) return 40
        // Light rain showers
        if (code >= 80 && code <= 81) return 30
        // Heavy rain
        if (code >= 61 && code <= 63) return 50
        // Heavy rain showers
        if (code >= 82) return 60
        
        return 30
    }
    
    function getSnowRate() {
        var code = parseInt(weatherCode) || 0
        
        // Light snow
        if (code >= 71 && code <= 73) return 10
        // Heavy snow
        if (code >= 74 && code <= 77) return 20
        // Snow showers
        if (code >= 85 && code <= 86) return 15
        
        return 10
    }
}
