// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "weatherprovider.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrlQuery>

#ifdef QT_POSITIONING_LIB
#include <QGeoPositionInfoSource>
#endif

namespace
{
qint64
nowMs ()
{
  return QDateTime::currentMSecsSinceEpoch ();
}
} // namespace

WeatherProvider::WeatherProvider (QObject *parent)
    : QObject (parent), m_networkManager (new QNetworkAccessManager (this)),
      m_positionSource (nullptr), m_refreshTimer (new QTimer (this)),
      m_latitude (0), m_longitude (0), m_isLoading (false), m_hasError (false),
      m_providerName (tr ("MET Norway")),
      m_candidateServices (buildCandidateServices ())
{
  // Setup refresh timer (update every 30 minutes)
  connect (m_refreshTimer, &QTimer::timeout, this, &WeatherProvider::refresh);
  m_refreshTimer->start (30 * 60 * 1000);

  // Initialize location source
  initLocationSource ();
}

WeatherProvider::~WeatherProvider ()
{
#ifdef QT_POSITIONING_LIB
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

      // Request position update
      m_positionSource->requestUpdate (10000); // 10 seconds timeout
      return;
    }
#endif

  qWarning () << "Position source not available, using default location";
  // Use Beijing as default
  setLocation (39.9042, 116.4074);
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
      m_positionSource->requestUpdate (10000);
#endif
    }
}

void
WeatherProvider::setLocation (double latitude, double longitude)
{
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
      setLocation (coord.latitude (), coord.longitude ());
    }
}

void
WeatherProvider::onPositionError (QGeoPositionInfoSource::Error error)
{
  qWarning () << "Position error:" << error;
  m_hasError = true;
  m_errorMessage = tr ("Failed to get location");
  emit errorChanged ();

  // Use default location
  setLocation (39.9042, 116.4074);
}
#endif

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
  QUrl url ("https://api.open-meteo.com/v1/forecast");
  QUrlQuery query;
  query.addQueryItem ("latitude", QString::number (latitude));
  query.addQueryItem ("longitude", QString::number (longitude));
  query.addQueryItem (
      "current",
      "temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m");
  query.addQueryItem ("daily", "temperature_2m_max,temperature_2m_min");
  query.addQueryItem ("timezone", "auto");
  query.addQueryItem ("wind_speed_unit", "ms");
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
  query.addQueryItem ("localityLanguage", "en");
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
  m_weatherData.windSpeed = current["wind_speed_10m"].toDouble ();

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
WeatherProvider::buildCandidateServices () const
{
  return {
    QVariantMap{
        { "priority", 1 },
        { "key", "met-no" },
        { "name", "MET Norway (api.met.no)" },
        { "status", "implemented" },
        { "coverage", "global" },
        { "auth", "User-Agent" },
        { "reason", tr ("Best default choice: global coverage, no API key, "
                        "forecast quality is high.") },
    },
    QVariantMap{
        { "priority", 2 },
        { "key", "nws" },
        { "name", "US National Weather Service (api.weather.gov)" },
        { "status", "candidate" },
        { "coverage", "United States" },
        { "auth", "User-Agent" },
        { "reason", tr ("High-quality official data, but only suitable for US "
                        "locations.") },
    },
    QVariantMap{
        { "priority", 3 },
        { "key", "openweathermap" },
        { "name", "OpenWeatherMap" },
        { "status", "candidate" },
        { "coverage", "global" },
        { "auth", "API key" },
        { "reason", tr ("Global and mature, but setup cost is higher because "
                        "an API key is required.") },
    },
    QVariantMap{
        { "priority", 4 },
        { "key", "metar" },
        { "name", "METAR / AviationWeather" },
        { "status", "candidate" },
        { "coverage", "station-based" },
        { "auth", "none" },
        { "reason", tr ("Useful for current observations near airports, but "
                        "weak for general forecast UX.") },
    },
    QVariantMap{
        { "priority", 5 },
        { "key", "iwin" },
        { "name", "IWIN (legacy)" },
        { "status", "legacy" },
        { "coverage", "legacy US feeds" },
        { "auth", "unknown" },
        { "reason", tr ("Legacy source kept for compatibility in libgweather, "
                        "not a good new integration target.") },
    },
  };
}
