// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "weatherapplet.h"
#include "locationsettingsdialog.h"
#include "pluginfactory.h"
#include <QDebug>
#include <QHash>
#include <QSettings>
#include <QVariantMap>

namespace
{
constexpr auto kAutoLocationEnabledKey = "autoLocationEnabled";
constexpr auto kManualCityKey = "manualCity";
constexpr auto kManualLatitudeKey = "manualLatitude";
constexpr auto kManualLongitudeKey = "manualLongitude";
constexpr auto kSettingsOrganization = "deepin";
constexpr auto kSettingsApplication = "dde-shell-weather-plugin";
}

WeatherApplet::WeatherApplet (QObject *parent)
    : DApplet (parent), m_weatherProvider (nullptr)
{
  qDebug () << "WeatherApplet created";
}

WeatherApplet::~WeatherApplet () { qDebug () << "WeatherApplet destroyed"; }

bool
WeatherApplet::init ()
{
  m_weatherProvider = new WeatherProvider (this);

  bool autoLocationEnabled = true;
  QString manualCity;
  double manualLatitude = 0;
  double manualLongitude = 0;
  loadLocationPreference (&autoLocationEnabled, &manualCity, &manualLatitude,
                          &manualLongitude);
  m_weatherProvider->restoreLocationPreference (
      autoLocationEnabled, manualCity, manualLatitude, manualLongitude);
  m_weatherProvider->initialize ();

  return DApplet::init ();
}

void
WeatherApplet::showSettings ()
{
  if (!m_weatherProvider)
    {
      return;
    }

  LocationSettingsDialog dialog (m_weatherProvider);
  if (dialog.exec () != QDialog::Accepted)
    {
      return;
    }

  saveLocationPreference (dialog.autoLocationEnabled (), dialog.manualCity (),
                          dialog.manualLatitude (), dialog.manualLongitude ());
}

QString
WeatherApplet::settingsGroupKey () const
{
  const QString resolvedPluginId
      = pluginId ().trimmed ().isEmpty ()
            ? QStringLiteral ("org.deepin.ds.weather")
            : pluginId ().trimmed ();
  return resolvedPluginId;
}

void
WeatherApplet::loadLocationPreference (bool *autoLocationEnabled,
                                       QString *manualCity,
                                       double *manualLatitude,
                                       double *manualLongitude) const
{
  QSettings settings (QString::fromLatin1 (kSettingsOrganization),
                      QString::fromLatin1 (kSettingsApplication));
  settings.beginGroup (settingsGroupKey ());

  const bool hasStoredSettings = settings.contains (kAutoLocationEnabledKey)
                                 || settings.contains (kManualCityKey)
                                 || settings.contains (kManualLatitudeKey)
                                 || settings.contains (kManualLongitudeKey);

  if (hasStoredSettings)
    {
      if (autoLocationEnabled)
        {
          *autoLocationEnabled
              = settings.value (kAutoLocationEnabledKey, true).toBool ();
        }

      if (manualCity)
        {
          *manualCity = settings.value (kManualCityKey).toString ().trimmed ();
        }

      if (manualLatitude)
        {
          *manualLatitude
              = settings.value (kManualLatitudeKey, 0.0).toDouble ();
        }

      if (manualLongitude)
        {
          *manualLongitude
              = settings.value (kManualLongitudeKey, 0.0).toDouble ();
        }

      settings.endGroup ();
      return;
    }

  const QStringList childKeys = settings.childKeys ();
  QHash<QString, QVariantMap> legacyGroups;
  for (const QString &key : childKeys)
    {
      const int separatorIndex = key.indexOf (QLatin1Char ('/'));
      if (separatorIndex <= 0 || separatorIndex >= key.size () - 1)
        {
          continue;
        }

      const QString prefix = key.left (separatorIndex);
      const QString subKey = key.mid (separatorIndex + 1);
      if (subKey != QLatin1String (kAutoLocationEnabledKey)
          && subKey != QLatin1String (kManualCityKey)
          && subKey != QLatin1String (kManualLatitudeKey)
          && subKey != QLatin1String (kManualLongitudeKey))
        {
          continue;
        }

      legacyGroups[prefix].insert (subKey, settings.value (key));
    }

  QVariantMap migratedValues;
  for (auto it = legacyGroups.cbegin (); it != legacyGroups.cend (); ++it)
    {
      const QVariantMap candidate = it.value ();
      if (!candidate.contains (kAutoLocationEnabledKey))
        {
          continue;
        }

      migratedValues = candidate;
      if (candidate.contains (kManualCityKey)
          && !candidate.value (kManualCityKey)
                  .toString ()
                  .trimmed ()
                  .isEmpty ())
        {
          break;
        }
    }

  if (!migratedValues.isEmpty ())
    {
      if (autoLocationEnabled)
        {
          *autoLocationEnabled
              = migratedValues.value (kAutoLocationEnabledKey, true).toBool ();
        }

      if (manualCity)
        {
          *manualCity
              = migratedValues.value (kManualCityKey).toString ().trimmed ();
        }

      if (manualLatitude)
        {
          *manualLatitude
              = migratedValues.value (kManualLatitudeKey, 0.0).toDouble ();
        }

      if (manualLongitude)
        {
          *manualLongitude
              = migratedValues.value (kManualLongitudeKey, 0.0).toDouble ();
        }

      settings.setValue (kAutoLocationEnabledKey,
                         migratedValues.value (kAutoLocationEnabledKey, true));
      settings.setValue (
          kManualCityKey,
          migratedValues.value (kManualCityKey).toString ().trimmed ());
      settings.setValue (kManualLatitudeKey,
                         migratedValues.value (kManualLatitudeKey, 0.0));
      settings.setValue (kManualLongitudeKey,
                         migratedValues.value (kManualLongitudeKey, 0.0));
      settings.endGroup ();
      settings.sync ();
      return;
    }

  settings.endGroup ();

  const DAppletData data = appletData ();

  if (autoLocationEnabled)
    {
      *autoLocationEnabled
          = data.value (kAutoLocationEnabledKey, true).toBool ();
    }

  if (manualCity)
    {
      *manualCity = data.value (kManualCityKey).toString ().trimmed ();
    }

  if (manualLatitude)
    {
      *manualLatitude = data.value (kManualLatitudeKey, 0.0).toDouble ();
    }

  if (manualLongitude)
    {
      *manualLongitude = data.value (kManualLongitudeKey, 0.0).toDouble ();
    }
}

void
WeatherApplet::saveLocationPreference (bool autoLocationEnabled,
                                       const QString &manualCity,
                                       double manualLatitude,
                                       double manualLongitude)
{
  QSettings settings (QString::fromLatin1 (kSettingsOrganization),
                      QString::fromLatin1 (kSettingsApplication));
  settings.beginGroup (settingsGroupKey ());
  settings.setValue (kAutoLocationEnabledKey, autoLocationEnabled);
  settings.setValue (kManualCityKey, manualCity.trimmed ());
  settings.setValue (kManualLatitudeKey, manualLatitude);
  settings.setValue (kManualLongitudeKey, manualLongitude);
  settings.endGroup ();
  settings.sync ();

  QVariantMap dataMap = appletData ().toMap ();
  dataMap.insert (kAutoLocationEnabledKey, autoLocationEnabled);
  dataMap.insert (kManualCityKey, manualCity.trimmed ());
  dataMap.insert (kManualLatitudeKey, manualLatitude);
  dataMap.insert (kManualLongitudeKey, manualLongitude);
  setAppletData (DAppletData (dataMap));

  if (m_weatherProvider)
    {
      m_weatherProvider->setLocationPreference (
          autoLocationEnabled, manualCity, manualLatitude, manualLongitude);
    }
}

// Register plugin
D_APPLET_CLASS (WeatherApplet)

#include "weatherapplet.moc"
