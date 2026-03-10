// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "weatherapplet.h"
#include "pluginfactory.h"
#include <QDebug>

WeatherApplet::WeatherApplet(QObject *parent)
    : DApplet(parent)
    , m_weatherProvider(nullptr)
{
    qDebug() << "WeatherApplet created";
}

WeatherApplet::~WeatherApplet()
{
    qDebug() << "WeatherApplet destroyed";
}

bool WeatherApplet::init()
{
    // Create weather provider
    m_weatherProvider = new WeatherProvider(this);
    
    // Call parent init
    return DApplet::init();
}

// Register plugin
D_APPLET_CLASS(WeatherApplet)

#include "weatherapplet.moc"
