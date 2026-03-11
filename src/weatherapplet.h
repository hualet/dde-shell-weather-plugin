// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "weatherprovider.h"

DS_USE_NAMESPACE

class WeatherApplet : public DApplet
{
  Q_OBJECT
  Q_PROPERTY (WeatherProvider *weather READ weather CONSTANT)

public:
  explicit WeatherApplet (QObject *parent = nullptr);
  virtual ~WeatherApplet ();

  virtual bool init () override;

  Q_INVOKABLE void showSettings ();

  WeatherProvider *
  weather () const
  {
    return m_weatherProvider;
  }

private:
  QString settingsGroupKey () const;
  void loadLocationPreference (bool *autoLocationEnabled, QString *manualCity,
                               double *manualLatitude,
                               double *manualLongitude) const;
  void saveLocationPreference (bool autoLocationEnabled,
                               const QString &manualCity,
                               double manualLatitude, double manualLongitude);

  WeatherProvider *m_weatherProvider;
};
