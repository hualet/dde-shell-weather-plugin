// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "weatherprovider.h"
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkRequest>
#include <QUrlQuery>

#ifdef QT_POSITIONING_LIB
#include <QGeoPositionInfoSource>
#endif

namespace
{
constexpr double kMetersPerSecondToKilometersPerHour = 3.6;
constexpr double kMetersPerSecondToMilesPerHour = 2.2369362920544;
constexpr double kDefaultLatitude = 39.9042;
constexpr double kDefaultLongitude = 116.4074;

#ifdef QT_POSITIONING_LIB
constexpr int kPositionRequestTimeoutMs = 10000;
constexpr int kGeoclueAgentReadyDelayMs = 300;
constexpr int kGeoclueAgentStartFallbackMs = 1500;
constexpr int kGeoclueAgentStopGraceMs = 1000;
#endif

qint64
nowMs ()
{
  return QDateTime::currentMSecsSinceEpoch ();
}

QLocale
formatLocale ()
{
  const QLocale locale;
  if (locale.language () != QLocale::C)
    {
      return locale;
    }

  const QLocale systemLocale = QLocale::system ();
  if (systemLocale.language () != QLocale::C)
    {
      return systemLocale;
    }

  return QLocale (QLocale::English);
}

bool
usesFahrenheit (const QLocale &locale)
{
  return locale.measurementSystem () == QLocale::ImperialUSSystem;
}

bool
usesMilesPerHour (const QLocale &locale)
{
  return locale.measurementSystem () == QLocale::ImperialUSSystem
         || locale.measurementSystem () == QLocale::ImperialUKSystem;
}

double
celsiusToDisplayValue (double celsius, const QLocale &locale)
{
  if (usesFahrenheit (locale))
    {
      return celsius * 9.0 / 5.0 + 32.0;
    }

  return celsius;
}

double
metersPerSecondToDisplayValue (double metersPerSecond, const QLocale &locale)
{
  if (usesMilesPerHour (locale))
    {
      return metersPerSecond * kMetersPerSecondToMilesPerHour;
    }

  return metersPerSecond * kMetersPerSecondToKilometersPerHour;
}

double
windSpeedToMetersPerSecond (double windSpeed, const QString &unit)
{
  if (unit == QStringLiteral ("km/h"))
    {
      return windSpeed / kMetersPerSecondToKilometersPerHour;
    }

  if (unit == QStringLiteral ("mph"))
    {
      return windSpeed / kMetersPerSecondToMilesPerHour;
    }

  return windSpeed;
}

QString
temperatureUnitForLocale (const QLocale &locale)
{
  if (usesFahrenheit (locale))
    {
      return QStringLiteral ("°F");
    }

  return QStringLiteral ("°C");
}

QString
windSpeedUnitForLocale (const QLocale &locale)
{
  if (usesMilesPerHour (locale))
    {
      return QStringLiteral ("mph");
    }

  return QStringLiteral ("km/h");
}

QString
openMeteoWindSpeedQueryUnit (const QLocale &locale)
{
  if (usesMilesPerHour (locale))
    {
      return QStringLiteral ("mph");
    }

  return QStringLiteral ("kmh");
}

QString
uiLanguageCode ()
{
  const QStringList uiLanguages = QLocale::system ().uiLanguages ();

  for (QString uiLanguage : uiLanguages)
    {
      const int encodingSeparator = uiLanguage.indexOf (QLatin1Char ('.'));
      if (encodingSeparator >= 0)
        {
          uiLanguage.truncate (encodingSeparator);
        }

      uiLanguage.replace (QLatin1Char ('-'), QLatin1Char ('_'));
      const QString languageCode
          = uiLanguage.section (QLatin1Char ('_'), 0, 0).toLower ();
      if (!languageCode.isEmpty () && languageCode != QStringLiteral ("c"))
        {
          return languageCode;
        }
    }

  const QString localeName
      = formatLocale ().name ().section ('_', 0, 0).toLower ();

  if (localeName.isEmpty () || localeName == QStringLiteral ("c"))
    {
      return QStringLiteral ("en");
    }

  return localeName;
}

#ifdef QT_POSITIONING_LIB
void
logGeoclueAgentOutput (QProcess *process)
{
  if (!process)
    {
      return;
    }

  const QString output
      = QString::fromLocal8Bit (process->readAllStandardOutput ()).trimmed ();
  if (!output.isEmpty ())
    {
      qInfo ().noquote () << "Geoclue agent output:" << output;
    }
}
#endif
} // namespace

WeatherProvider::WeatherProvider (QObject *parent)
    : QObject (parent), m_networkManager (new QNetworkAccessManager (this)),
      m_refreshTimer (new QTimer (this)), m_latitude (0), m_longitude (0),
      m_isLoading (false), m_hasError (false),
      m_providerName (tr ("MET Norway")), m_positionSource (nullptr),
      m_geoclueAgentProcess (nullptr), m_positionRequestPending (false)
{
#ifdef QT_POSITIONING_LIB
  m_geoclueAgentProcess = new QProcess (this);
  m_geoclueAgentProcess->setProcessChannelMode (QProcess::MergedChannels);
  connect (m_geoclueAgentProcess, &QProcess::started, this,
           &WeatherProvider::onGeoclueAgentStarted);
  connect (m_geoclueAgentProcess, &QProcess::errorOccurred, this,
           &WeatherProvider::onGeoclueAgentErrorOccurred);
  connect (m_geoclueAgentProcess,
           qOverload<int, QProcess::ExitStatus> (&QProcess::finished), this,
           &WeatherProvider::onGeoclueAgentFinished);
#endif

  // Setup refresh timer (update every 30 minutes)
  connect (m_refreshTimer, &QTimer::timeout, this, &WeatherProvider::refresh);
  m_refreshTimer->start (30 * 60 * 1000);

  // Initialize location source
  initLocationSource ();
}

WeatherProvider::~WeatherProvider ()
{
#ifdef QT_POSITIONING_LIB
  stopGeoclueAgent ();
  if (m_positionSource)
    {
      m_positionSource->stopUpdates ();
    }
#endif
}

void
WeatherProvider::initLocationSource ()
{
#ifdef QT_POSITIONING_LIB
  m_positionSource = QGeoPositionInfoSource::createDefaultSource (this);

  if (m_positionSource)
    {
      connect (m_positionSource, &QGeoPositionInfoSource::positionUpdated,
               this, &WeatherProvider::onPositionUpdated);
      connect (m_positionSource, &QGeoPositionInfoSource::errorOccurred, this,
               &WeatherProvider::onPositionError);

      requestLocationUpdate ();
      return;
    }
#endif

  qWarning () << "Position source not available, using default location";
  // Use Beijing as default
  setLocation (kDefaultLatitude, kDefaultLongitude);
}

void
WeatherProvider::refresh ()
{
  qInfo () << "Weather refresh requested"
           << "latitude=" << m_latitude << "longitude=" << m_longitude
           << "loading=" << m_isLoading;

  if (m_latitude != 0 && m_longitude != 0)
    {
      fetchWeather (m_latitude, m_longitude);
#ifdef QT_POSITIONING_LIB
    }
  else if (m_positionSource)
    {
      requestLocationUpdate ();
#endif
    }
}

void
WeatherProvider::setLocation (double latitude, double longitude)
{
#ifdef QT_POSITIONING_LIB
  stopGeoclueAgent ();
#endif

  qInfo () << "Weather location updated"
           << "latitude=" << latitude << "longitude=" << longitude;

  m_latitude = latitude;
  m_longitude = longitude;
  fetchWeather (latitude, longitude);
  fetchCityName (latitude, longitude);
}

#ifdef QT_POSITIONING_LIB
void
WeatherProvider::onPositionUpdated (const QGeoPositionInfo &info)
{
  QGeoCoordinate coord = info.coordinate ();
  if (coord.isValid ())
    {
      stopGeoclueAgent ();
      setLocation (coord.latitude (), coord.longitude ());
    }
}

void
WeatherProvider::onPositionError (QGeoPositionInfoSource::Error error)
{
  stopGeoclueAgent ();
  qWarning () << "Position error:" << error;
  m_hasError = true;
  m_errorMessage = tr ("Failed to get location");
  emit errorChanged ();

  // Use default location
  setLocation (kDefaultLatitude, kDefaultLongitude);
}

void
WeatherProvider::onGeoclueAgentStarted ()
{
  qInfo () << "Geoclue agent started"
           << "program=" << m_geoclueAgentProcess->program ();
  QTimer::singleShot (kGeoclueAgentReadyDelayMs, this,
                      &WeatherProvider::requestPositionUpdate);
}

void
WeatherProvider::onGeoclueAgentErrorOccurred (QProcess::ProcessError error)
{
  logGeoclueAgentOutput (m_geoclueAgentProcess);
  qWarning () << "Failed to start Geoclue agent"
              << "error=" << error
              << "message=" << m_geoclueAgentProcess->errorString ();
  requestPositionUpdate ();
}

void
WeatherProvider::onGeoclueAgentFinished (int exitCode,
                                         QProcess::ExitStatus exitStatus)
{
  logGeoclueAgentOutput (m_geoclueAgentProcess);
  qInfo () << "Geoclue agent finished"
           << "exitCode=" << exitCode << "exitStatus=" << exitStatus;
  requestPositionUpdate ();
}
#endif

void
WeatherProvider::requestLocationUpdate ()
{
#ifdef QT_POSITIONING_LIB
  if (!m_positionSource)
    {
      return;
    }

  if (m_positionRequestPending)
    {
      qInfo () << "Position request already pending";
      return;
    }

  m_positionRequestPending = true;
  ensureGeoclueAgentForLocation ();
#endif
}

void
WeatherProvider::fetchWeather (double latitude, double longitude)
{
  if (!m_isLoading)
    {
      m_isLoading = true;
      emit loadingChanged ();
    }

  if (m_hasError)
    {
      m_hasError = false;
      m_errorMessage.clear ();
      emit errorChanged ();
    }

  fetchWeatherFromBackend (WeatherBackend::MetNo, latitude, longitude);
}

void
WeatherProvider::fetchWeatherFromBackend (WeatherBackend backend,
                                          double latitude, double longitude)
{
  switch (backend)
    {
    case WeatherBackend::MetNo:
      fetchMetNoWeather (latitude, longitude);
      return;
    case WeatherBackend::OpenMeteo:
      fetchOpenMeteoWeather (latitude, longitude);
      return;
    }
}

void
WeatherProvider::fetchMetNoWeather (double latitude, double longitude)
{
  QUrl url ("https://api.met.no/weatherapi/locationforecast/2.0/compact");
  QUrlQuery query;
  query.addQueryItem ("lat", QString::number (latitude, 'f', 4));
  query.addQueryItem ("lon", QString::number (longitude, 'f', 4));
  url.setQuery (query);

  QNetworkRequest request (url);
  request.setRawHeader (
      "User-Agent",
      "dde-shell-weather-plugin/1.0 "
      "(https://github.com/linuxdeepin/dde-shell-weather-plugin)");
  request.setRawHeader ("Accept", "application/json");

  qInfo () << "Weather request start"
           << "backend=" << backendName (WeatherBackend::MetNo)
           << "latitude=" << latitude << "longitude=" << longitude
           << "url=" << url.toString ();

  QNetworkReply *reply = m_networkManager->get (request);
  reply->setProperty ("backend", static_cast<int> (WeatherBackend::MetNo));
  reply->setProperty ("latitude", latitude);
  reply->setProperty ("longitude", longitude);
  reply->setProperty ("requestStartedAt", nowMs ());

  connect (reply, &QNetworkReply::finished, this,
           [this, reply] () { onWeatherReplyFinished (reply); });
}

void
WeatherProvider::fetchOpenMeteoWeather (double latitude, double longitude)
{
  const QLocale locale = formatLocale ();
  QUrl url ("https://api.open-meteo.com/v1/forecast");
  QUrlQuery query;
  query.addQueryItem ("latitude", QString::number (latitude));
  query.addQueryItem ("longitude", QString::number (longitude));
  query.addQueryItem (
      "current",
      "temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m");
  query.addQueryItem ("daily", "temperature_2m_max,temperature_2m_min");
  query.addQueryItem ("timezone", "auto");
  query.addQueryItem ("wind_speed_unit", openMeteoWindSpeedQueryUnit (locale));
  url.setQuery (query);

  qInfo () << "Weather request start"
           << "backend=" << backendName (WeatherBackend::OpenMeteo)
           << "latitude=" << latitude << "longitude=" << longitude
           << "url=" << url.toString ();

  QNetworkRequest request (url);
  QNetworkReply *reply = m_networkManager->get (request);
  reply->setProperty ("backend", static_cast<int> (WeatherBackend::OpenMeteo));
  reply->setProperty ("latitude", latitude);
  reply->setProperty ("longitude", longitude);
  reply->setProperty ("requestStartedAt", nowMs ());

  connect (reply, &QNetworkReply::finished, this,
           [this, reply] () { onWeatherReplyFinished (reply); });
}

void
WeatherProvider::fetchCityName (double latitude, double longitude)
{
  // Use BigData Cloud free reverse geocoding API (no auth required)
  QUrl url ("https://api.bigdatacloud.net/data/reverse-geocode-client");
  QUrlQuery query;
  query.addQueryItem ("latitude", QString::number (latitude));
  query.addQueryItem ("longitude", QString::number (longitude));
  query.addQueryItem ("localityLanguage", uiLanguageCode ());
  url.setQuery (query);

  qInfo () << "City request start"
           << "latitude=" << latitude << "longitude=" << longitude
           << "url=" << url.toString ();

  QNetworkRequest request (url);
  QNetworkReply *reply = m_networkManager->get (request);
  reply->setProperty ("requestStartedAt", nowMs ());

  connect (reply, &QNetworkReply::finished, this,
           [this, reply] () { onCityReplyFinished (reply); });
}

void
WeatherProvider::onWeatherReplyFinished (QNetworkReply *reply)
{
  const WeatherBackend backend
      = static_cast<WeatherBackend> (reply->property ("backend").toInt ());
  const double latitude = reply->property ("latitude").toDouble ();
  const double longitude = reply->property ("longitude").toDouble ();
  const qint64 startedAt = reply->property ("requestStartedAt").toLongLong ();
  const qint64 elapsedMs = startedAt > 0 ? nowMs () - startedAt : -1;
  const QVariant httpStatus
      = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute);

  qInfo () << "Weather request finished"
           << "backend=" << backendName (backend) << "elapsedMs=" << elapsedMs
           << "httpStatus=" << httpStatus
           << "networkError=" << reply->error ();

  reply->deleteLater ();

  if (reply->error () != QNetworkReply::NoError)
    {
      qWarning () << "Weather API error"
                  << "backend=" << backendName (backend)
                  << "elapsedMs=" << elapsedMs
                  << "message=" << reply->errorString ();
      if (fallbackToNextBackend (backend, latitude, longitude))
        {
          return;
        }

      finishWeatherRequest ();
      m_hasError = true;
      m_errorMessage = reply->errorString ();
      emit errorChanged ();
      return;
    }

  const QJsonDocument doc = QJsonDocument::fromJson (reply->readAll ());
  QJsonObject root = doc.object ();

  bool parsed = false;
  switch (backend)
    {
    case WeatherBackend::MetNo:
      parsed = parseMetNoWeather (root);
      if (parsed)
        {
          m_providerName = tr ("MET Norway");
        }
      break;
    case WeatherBackend::OpenMeteo:
      parsed = parseOpenMeteoWeather (root);
      if (parsed)
        {
          m_providerName = tr ("Open-Meteo");
        }
      break;
    }

  if (!parsed)
    {
      qWarning () << "Unexpected weather payload"
                  << "backend=" << backendName (backend)
                  << "elapsedMs=" << elapsedMs;
      if (fallbackToNextBackend (backend, latitude, longitude))
        {
          return;
        }

      finishWeatherRequest ();
      m_hasError = true;
      m_errorMessage = tr ("Unexpected weather data response");
      emit errorChanged ();
      return;
    }

  qInfo () << "Weather parse success"
           << "backend=" << backendName (backend)
           << "providerName=" << m_providerName
           << "temperature=" << m_weatherData.temperature
           << "humidity=" << m_weatherData.humidity
           << "windSpeed=" << m_weatherData.windSpeed
           << "weatherCode=" << m_weatherData.weatherCode
           << "elapsedMs=" << elapsedMs;

  finishWeatherRequest ();
  m_weatherData.isValid = true;
  emit weatherChanged ();
}

void
WeatherProvider::onCityReplyFinished (QNetworkReply *reply)
{
  const qint64 startedAt = reply->property ("requestStartedAt").toLongLong ();
  const qint64 elapsedMs = startedAt > 0 ? nowMs () - startedAt : -1;
  const QVariant httpStatus
      = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute);

  qInfo () << "City request finished"
           << "elapsedMs=" << elapsedMs << "httpStatus=" << httpStatus
           << "networkError=" << reply->error ();

  reply->deleteLater ();

  if (reply->error () != QNetworkReply::NoError)
    {
      qWarning () << "City API error"
                  << "elapsedMs=" << elapsedMs
                  << "message=" << reply->errorString ();
      m_weatherData.city = tr ("Unknown");
      emit weatherChanged ();
      return;
    }

  QByteArray data = reply->readAll ();
  QJsonDocument doc = QJsonDocument::fromJson (data);
  QJsonObject root = doc.object ();

  // BigData Cloud returns city directly
  QString city = root["city"].toString ();
  if (city.isEmpty ())
    {
      city = root["locality"].toString ();
    }
  if (city.isEmpty ())
    {
      city = root["principalSubdivision"].toString ();
    }

  m_weatherData.city = city.isEmpty () ? tr ("Unknown") : city;
  qInfo () << "City parse success"
           << "city=" << m_weatherData.city << "elapsedMs=" << elapsedMs;
  emit weatherChanged ();
}

bool
WeatherProvider::parseMetNoWeather (const QJsonObject &root)
{
  const QJsonObject properties = root["properties"].toObject ();
  const QJsonArray timeseries = properties["timeseries"].toArray ();

  if (timeseries.isEmpty ())
    {
      return false;
    }

  const QJsonObject firstPoint = timeseries[0].toObject ();
  const QJsonObject firstData = firstPoint["data"].toObject ();
  const QJsonObject instantDetails
      = firstData["instant"].toObject ()["details"].toObject ();

  if (instantDetails.isEmpty ()
      || !instantDetails.contains ("air_temperature"))
    {
      return false;
    }

  m_weatherData.temperature = instantDetails["air_temperature"].toDouble ();
  m_weatherData.humidity = instantDetails["relative_humidity"].toDouble ();
  m_weatherData.windSpeed = instantDetails["wind_speed"].toDouble ();

  QString symbolCode = firstData["next_1_hours"]
                           .toObject ()["summary"]
                           .toObject ()["symbol_code"]
                           .toString ();
  if (symbolCode.isEmpty ())
    {
      symbolCode = firstData["next_6_hours"]
                       .toObject ()["summary"]
                       .toObject ()["symbol_code"]
                       .toString ();
    }
  if (symbolCode.isEmpty ())
    {
      symbolCode = firstData["next_12_hours"]
                       .toObject ()["summary"]
                       .toObject ()["symbol_code"]
                       .toString ();
    }

  m_weatherData.weatherCode = symbolCode;
  m_weatherData.weatherDescription = parseMetNoSymbolCode (symbolCode);

  const QDateTime firstTime
      = QDateTime::fromString (firstPoint["time"].toString (), Qt::ISODate);

  double minTemp = m_weatherData.temperature;
  double maxTemp = m_weatherData.temperature;
  bool hasRange = false;

  for (const QJsonValue &entryValue : timeseries)
    {
      const QJsonObject entry = entryValue.toObject ();
      const QDateTime timestamp
          = QDateTime::fromString (entry["time"].toString (), Qt::ISODate);
      if (firstTime.isValid () && timestamp.isValid ()
          && firstTime.secsTo (timestamp) >= 24 * 60 * 60)
        {
          break;
        }

      const QJsonObject details = entry["data"]
                                      .toObject ()["instant"]
                                      .toObject ()["details"]
                                      .toObject ();
      if (!details.contains ("air_temperature"))
        {
          continue;
        }

      const double airTemperature = details["air_temperature"].toDouble ();
      if (!hasRange)
        {
          minTemp = airTemperature;
          maxTemp = airTemperature;
          hasRange = true;
          continue;
        }

      if (airTemperature < minTemp)
        {
          minTemp = airTemperature;
        }
      if (airTemperature > maxTemp)
        {
          maxTemp = airTemperature;
        }
    }

  m_weatherData.temperatureMin
      = hasRange ? minTemp : m_weatherData.temperature;
  m_weatherData.temperatureMax
      = hasRange ? maxTemp : m_weatherData.temperature;

  return true;
}

bool
WeatherProvider::parseOpenMeteoWeather (const QJsonObject &root)
{
  if (!root.contains ("current"))
    {
      return false;
    }

  const QJsonObject current = root["current"].toObject ();

  if (!current.contains ("temperature_2m"))
    {
      return false;
    }

  m_weatherData.temperature = current["temperature_2m"].toDouble ();
  m_weatherData.humidity = current["relative_humidity_2m"].toDouble ();
  const QString windSpeedUnit
      = root["current_units"].toObject ()["wind_speed_10m"].toString ();
  m_weatherData.windSpeed = windSpeedToMetersPerSecond (
      current["wind_speed_10m"].toDouble (), windSpeedUnit);

  const int weatherCode = current["weather_code"].toInt ();
  m_weatherData.weatherCode = QString::number (weatherCode);
  m_weatherData.weatherDescription = parseOpenMeteoWeatherCode (weatherCode);

  if (root.contains ("daily"))
    {
      const QJsonObject daily = root["daily"].toObject ();
      const QJsonArray maxTemps = daily["temperature_2m_max"].toArray ();
      const QJsonArray minTemps = daily["temperature_2m_min"].toArray ();
      if (!maxTemps.isEmpty ())
        {
          m_weatherData.temperatureMax = maxTemps[0].toDouble ();
        }
      else
        {
          m_weatherData.temperatureMax = m_weatherData.temperature;
        }
      if (!minTemps.isEmpty ())
        {
          m_weatherData.temperatureMin = minTemps[0].toDouble ();
        }
      else
        {
          m_weatherData.temperatureMin = m_weatherData.temperature;
        }
    }
  else
    {
      m_weatherData.temperatureMax = m_weatherData.temperature;
      m_weatherData.temperatureMin = m_weatherData.temperature;
    }

  return true;
}

void
WeatherProvider::finishWeatherRequest ()
{
  if (m_isLoading)
    {
      m_isLoading = false;
      emit loadingChanged ();
    }
}

bool
WeatherProvider::fallbackToNextBackend (WeatherBackend backend,
                                        double latitude, double longitude)
{
  if (backend == WeatherBackend::MetNo)
    {
      qWarning () << "Weather backend fallback"
                  << "from=" << backendName (backend)
                  << "to=" << backendName (WeatherBackend::OpenMeteo)
                  << "latitude=" << latitude << "longitude=" << longitude;
      fetchWeatherFromBackend (WeatherBackend::OpenMeteo, latitude, longitude);
      return true;
    }

  return false;
}

QString
WeatherProvider::backendName (WeatherBackend backend)
{
  switch (backend)
    {
    case WeatherBackend::MetNo:
      return QStringLiteral ("MET Norway");
    case WeatherBackend::OpenMeteo:
      return QStringLiteral ("Open-Meteo");
    }

  return QStringLiteral ("Unknown");
}

QString
WeatherProvider::parseOpenMeteoWeatherCode (int code)
{
  // WMO Weather interpretation codes (WW)
  // https://open-meteo.com/en/docs

  if (code == 0)
    return tr ("Clear sky");
  if (code >= 1 && code <= 3)
    return tr ("Partly cloudy");
  if (code >= 45 && code <= 48)
    return tr ("Fog");
  if (code >= 51 && code <= 57)
    return tr ("Drizzle");
  if (code >= 61 && code <= 67)
    return tr ("Rain");
  if (code >= 71 && code <= 77)
    return tr ("Snow");
  if (code >= 80 && code <= 82)
    return tr ("Rain showers");
  if (code >= 85 && code <= 86)
    return tr ("Snow showers");
  if (code >= 95 && code <= 99)
    return tr ("Thunderstorm");

  return tr ("Unknown");
}

QString
WeatherProvider::parseMetNoSymbolCode (const QString &symbolCode)
{
  const QString normalized = symbolCode.toLower ();

  if (normalized.contains ("thunder"))
    return tr ("Thunderstorm");
  if (normalized.contains ("fog"))
    return tr ("Fog");
  if (normalized.contains ("partlycloudy") || normalized.contains ("fair"))
    {
      return tr ("Partly cloudy");
    }
  if (normalized.contains ("snowshowers"))
    return tr ("Snow showers");
  if (normalized.contains ("rainshowers"))
    return tr ("Rain showers");
  if (normalized.contains ("snow"))
    return tr ("Snow");
  if (normalized.contains ("sleet"))
    return tr ("Sleet");
  if (normalized.contains ("drizzle"))
    return tr ("Drizzle");
  if (normalized.contains ("rain"))
    return tr ("Rain");
  if (normalized.contains ("cloudy"))
    return tr ("Cloudy");
  if (normalized.contains ("clearsky"))
    return tr ("Clear sky");

  return tr ("Unknown");
}

QVariantList
WeatherProvider::candidateServices () const
{
  return buildCandidateServices ();
}

QString
WeatherProvider::formattedTemperature () const
{
  const QLocale locale = formatLocale ();
  const double displayValue
      = celsiusToDisplayValue (m_weatherData.temperature, locale);

  return tr ("%1%2")
      .arg (locale.toString (qRound (displayValue)))
      .arg (temperatureUnitForLocale (locale));
}

QString
WeatherProvider::formattedTemperatureRange () const
{
  const QLocale locale = formatLocale ();
  const QString minValue = locale.toString (qRound (celsiusToDisplayValue (
                               m_weatherData.temperatureMin, locale)))
                           + QStringLiteral ("°");
  const QString maxValue = locale.toString (qRound (celsiusToDisplayValue (
                               m_weatherData.temperatureMax, locale)))
                           + QStringLiteral ("°");

  return tr ("%1/%2").arg (minValue).arg (maxValue);
}

QString
WeatherProvider::formattedWindSpeed () const
{
  const QLocale locale = formatLocale ();
  const double displayValue
      = metersPerSecondToDisplayValue (m_weatherData.windSpeed, locale);

  return tr ("%1 %2")
      .arg (locale.toString (qRound (displayValue)))
      .arg (windSpeedUnitForLocale (locale));
}

QString
WeatherProvider::temperatureUnit () const
{
  return temperatureUnitForLocale (formatLocale ());
}

QString
WeatherProvider::windSpeedUnit () const
{
  return windSpeedUnitForLocale (formatLocale ());
}

QVariantList
WeatherProvider::buildCandidateServices () const
{
  return {
    QVariantMap{
        { "priority", 1 },
        { "key", "met-no" },
        { "name", "MET Norway (api.met.no)" },
        { "status", "implemented" },
        { "statusLabel", tr ("Implemented") },
        { "coverage", "global" },
        { "coverageLabel", tr ("Global") },
        { "auth", "user-agent" },
        { "authLabel", tr ("User-Agent") },
        { "reason", tr ("Best default choice: global coverage, no API key, "
                        "forecast quality is high.") },
    },
    QVariantMap{
        { "priority", 2 },
        { "key", "nws" },
        { "name", "US National Weather Service (api.weather.gov)" },
        { "status", "candidate" },
        { "statusLabel", tr ("Candidate") },
        { "coverage", "united-states" },
        { "coverageLabel", tr ("United States") },
        { "auth", "user-agent" },
        { "authLabel", tr ("User-Agent") },
        { "reason", tr ("High-quality official data, but only suitable for US "
                        "locations.") },
    },
    QVariantMap{
        { "priority", 3 },
        { "key", "openweathermap" },
        { "name", "OpenWeatherMap" },
        { "status", "candidate" },
        { "statusLabel", tr ("Candidate") },
        { "coverage", "global" },
        { "coverageLabel", tr ("Global") },
        { "auth", "api-key" },
        { "authLabel", tr ("API key") },
        { "reason", tr ("Global and mature, but setup cost is higher because "
                        "an API key is required.") },
    },
    QVariantMap{
        { "priority", 4 },
        { "key", "metar" },
        { "name", "METAR / AviationWeather" },
        { "status", "candidate" },
        { "statusLabel", tr ("Candidate") },
        { "coverage", "station-based" },
        { "coverageLabel", tr ("Station-based") },
        { "auth", "none" },
        { "authLabel", tr ("None") },
        { "reason", tr ("Useful for current observations near airports, but "
                        "weak for general forecast UX.") },
    },
    QVariantMap{
        { "priority", 5 },
        { "key", "iwin" },
        { "name", "IWIN (legacy)" },
        { "status", "legacy" },
        { "statusLabel", tr ("Legacy") },
        { "coverage", "legacy-us-feeds" },
        { "coverageLabel", tr ("Legacy US feeds") },
        { "auth", "unknown" },
        { "authLabel", tr ("Unknown") },
        { "reason", tr ("Legacy source kept for compatibility in libgweather, "
                        "not a good new integration target.") },
    },
  };
}

#ifdef QT_POSITIONING_LIB
void
WeatherProvider::ensureGeoclueAgentForLocation ()
{
  if (!m_geoclueAgentProcess)
    {
      requestPositionUpdate ();
      return;
    }

  const QString program = geoclueAgentProgram ();
  if (program.isEmpty ())
    {
      qWarning () << "Geoclue agent not found, requesting location without "
                     "pre-started authorization agent";
      requestPositionUpdate ();
      return;
    }

  if (m_geoclueAgentProcess->state () != QProcess::NotRunning)
    {
      qInfo () << "Geoclue agent already running"
               << "program=" << m_geoclueAgentProcess->program ();
      QTimer::singleShot (kGeoclueAgentReadyDelayMs, this,
                          &WeatherProvider::requestPositionUpdate);
      return;
    }

  qInfo () << "Starting Geoclue agent"
           << "program=" << program;
  m_geoclueAgentProcess->setProgram (program);
  m_geoclueAgentProcess->setArguments ({});
  m_geoclueAgentProcess->start ();

  QTimer::singleShot (kGeoclueAgentStartFallbackMs, this, [this] () {
    if (!m_positionRequestPending || !m_geoclueAgentProcess
        || m_geoclueAgentProcess->state () == QProcess::Running)
      {
        return;
      }

    qWarning () << "Geoclue agent start timed out, requesting position anyway";
    requestPositionUpdate ();
  });
}

void
WeatherProvider::requestPositionUpdate ()
{
  if (!m_positionSource || !m_positionRequestPending)
    {
      return;
    }

  m_positionRequestPending = false;
  qInfo () << "Requesting position update"
           << "timeoutMs=" << kPositionRequestTimeoutMs;
  m_positionSource->requestUpdate (kPositionRequestTimeoutMs);
}

void
WeatherProvider::stopGeoclueAgent ()
{
  if (!m_geoclueAgentProcess)
    {
      return;
    }

  m_positionRequestPending = false;

  if (m_geoclueAgentProcess->state () == QProcess::NotRunning)
    {
      return;
    }

  qInfo () << "Stopping Geoclue agent"
           << "program=" << m_geoclueAgentProcess->program ();
  m_geoclueAgentProcess->terminate ();
  QTimer::singleShot (kGeoclueAgentStopGraceMs, this, [this] () {
    if (!m_geoclueAgentProcess
        || m_geoclueAgentProcess->state () == QProcess::NotRunning)
      {
        return;
      }

    qWarning () << "Geoclue agent did not exit in time, killing process";
    m_geoclueAgentProcess->kill ();
  });
}

QString
WeatherProvider::geoclueAgentProgram () const
{
  const QString configuredAgentPath
      = qEnvironmentVariable ("DS_WEATHER_GEOCLUE_AGENT_PATH");
  const QStringList candidates = {
    configuredAgentPath,
    QStringLiteral ("/usr/libexec/geoclue-2.0/demos/agent"),
    QStringLiteral ("/usr/lib/geoclue-2.0/demos/agent"),
  };

  for (const QString &candidate : candidates)
    {
      if (candidate.isEmpty ())
        {
          continue;
        }

      const QFileInfo fileInfo (candidate);
      if (fileInfo.isFile () && fileInfo.isExecutable ())
        {
          return fileInfo.absoluteFilePath ();
        }
    }

  return QString ();
}
#endif
