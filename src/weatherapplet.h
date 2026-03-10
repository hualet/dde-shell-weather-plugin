// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "weatherprovider.h"

DS_USE_NAMESPACE

class WeatherApplet : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(WeatherProvider* weather READ weather CONSTANT)

public:
    explicit WeatherApplet(QObject *parent = nullptr);
    virtual ~WeatherApplet();
    
    virtual bool init() override;
    
    WeatherProvider* weather() const { return m_weatherProvider; }

private:
    WeatherProvider *m_weatherProvider;
};
