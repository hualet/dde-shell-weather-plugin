// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "weatherprovider.h"
#include <QDBusConnection>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <cmath>

#ifdef QT_POSITIONING_LIB
#include <QGeoPositionInfoSource>
#endif

namespace
{
constexpr double kMetersPerSecondToKilometersPerHour = 3.6;
constexpr double kMetersPerSecondToMilesPerHour = 2.2369362920544;
constexpr double kDefaultLatitude = 39.9042;
constexpr double kDefaultLongitude = 116.4074;
constexpr int kRefreshIntervalMs = 30 * 60 * 1000;
constexpr qint64 kRefreshDedupWindowMs = 60 * 1000;

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

QString
ipApiLanguageCodeForLocale (const QLocale &locale)
{
  switch (locale.language ())
    {
    case QLocale::German:
      return QStringLiteral ("de");
    case QLocale::Spanish:
      return QStringLiteral ("es");
    case QLocale::Portuguese:
      return QStringLiteral ("pt-BR");
    case QLocale::French:
      return QStringLiteral ("fr");
    case QLocale::Japanese:
      return QStringLiteral ("ja");
    case QLocale::Chinese:
      return QStringLiteral ("zh-CN");
    case QLocale::Russian:
      return QStringLiteral ("ru");
    default:
      return QStringLiteral ("en");
    }
}

QString
ipApiLanguageCode ()
{
  const QLocale locale = formatLocale ();
  const QStringList uiLanguages = locale.uiLanguages ();

  for (const QString &uiLanguage : uiLanguages)
    {
      const QLocale uiLocale (uiLanguage);
      if (uiLocale.language () == QLocale::C)
        {
          continue;
        }

      return ipApiLanguageCodeForLocale (uiLocale);
    }

  return ipApiLanguageCodeForLocale (locale);
}

bool
jsonValueToDouble (const QJsonValue &value, double *result)
{
  if (!result)
    {
      return false;
    }

  if (value.isDouble ())
    {
      *result = value.toDouble ();
      return true;
    }

  if (value.isString ())
    {
      bool ok = false;
      const double parsed = value.toString ().toDouble (&ok);
      if (ok)
        {
          *result = parsed;
          return true;
        }
    }

  return false;
}

#ifdef QT_POSITIONING_LIB
QString
positionErrorName (QGeoPositionInfoSource::Error error)
{
  switch (error)
    {
    case QGeoPositionInfoSource::NoError:
      return QStringLiteral ("NoError");
    case QGeoPositionInfoSource::AccessError:
      return QStringLiteral ("AccessError");
    case QGeoPositionInfoSource::ClosedError:
      return QStringLiteral ("ClosedError");
    case QGeoPositionInfoSource::UnknownSourceError:
      return QStringLiteral ("UnknownSourceError");
    case QGeoPositionInfoSource::UpdateTimeoutError:
      return QStringLiteral ("UpdateTimeoutError");
    }

  return QStringLiteral ("UnknownError");
}

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
      m_providerName (tr ("MET Norway")), m_initialized (false),
      m_autoLocationEnabled (true), m_manualLatitude (0),
      m_manualLongitude (0), m_lastAutoLatitude (0), m_lastAutoLongitude (0),
      m_citySearchReply (nullptr), m_isSearchingCities (false),
      m_citySearchRequestSerial (0),
      m_locationLookupPurpose (LocationLookupPurpose::WeatherUpdate),
      m_locationRequestSerial (0), m_positionSource (nullptr),
      m_geoclueAgentProcess (nullptr), m_geoclueAgentPrestartDisabled (false),
      m_geoclueAgentStopRequested (false), m_positionRequestPending (false),
      m_locationLookupInProgress (false), m_ipLocationRequestPending (false),
      m_lastRefreshRequestAtMs (0)
{
#ifdef QT_POSITIONING_LIB
  m_geoclueAgentProcess = new QProcess (this);
  m_geoclueAgentProcess->setProcessChannelMode (QProcess::MergedChannels);
  connect (m_geoclueAgentProcess, &QProcess::readyReadStandardOutput, this,
           [this] () { logGeoclueAgentOutput (m_geoclueAgentProcess); });
  connect (m_geoclueAgentProcess, &QProcess::started, this,
           &WeatherProvider::onGeoclueAgentStarted);
  connect (m_geoclueAgentProcess, &QProcess::errorOccurred, this,
           &WeatherProvider::onGeoclueAgentErrorOccurred);
  connect (m_geoclueAgentProcess,
           qOverload<int, QProcess::ExitStatus> (&QProcess::finished), this,
           &WeatherProvider::onGeoclueAgentFinished);
#endif

  // Setup refresh timer (update every 30 minutes)
  connect (m_refreshTimer, &QTimer::timeout, this,
           [this] () { triggerRefresh (QStringLiteral ("periodic-timer")); });
  resetRefreshTimer ();

  QDBusConnection systemBus = QDBusConnection::systemBus ();
  if (!systemBus.isConnected ())
    {
      qWarning () << "System D-Bus unavailable; suspend resume refresh "
                     "monitoring disabled";
    }
  else
    {
      const bool connected = systemBus.connect (
          QStringLiteral ("org.freedesktop.login1"),
          QStringLiteral ("/org/freedesktop/login1"),
          QStringLiteral ("org.freedesktop.login1.Manager"),
          QStringLiteral ("PrepareForSleep"), this,
          SLOT (onPrepareForSleep (bool)));

      if (!connected)
        {
          qWarning () << "Failed to subscribe to login1 PrepareForSleep";
        }
      else
        {
          qInfo () << "Suspend resume monitoring enabled"
                   << "signal=org.freedesktop.login1.Manager.PrepareForSleep";
        }
    }

  // Initialize location source
}

WeatherProvider::~WeatherProvider ()
{
  if (m_citySearchReply)
    {
      m_citySearchReply->abort ();
    }

#ifdef QT_POSITIONING_LIB
  stopGeoclueAgent ();
  if (m_positionSource)
    {
      m_positionSource->stopUpdates ();
    }
#endif
}

void
WeatherProvider::initialize ()
{
  if (m_initialized)
    {
      return;
    }

  m_initialized = true;
  initLocationSource ();

  if (!m_autoLocationEnabled && hasManualLocationPreference ())
    {
      ++m_locationRequestSerial;
      updateLocation (m_manualLatitude, m_manualLongitude, m_manualCity);
      return;
    }

  requestLocationUpdate ();
}

void
WeatherProvider::restoreLocationPreference (bool autoLocationEnabled,
                                            const QString &manualCity,
                                            double manualLatitude,
                                            double manualLongitude)
{
  m_autoLocationEnabled = autoLocationEnabled;
  m_manualCity = manualCity.trimmed ();
  m_manualLatitude = manualLatitude;
  m_manualLongitude = manualLongitude;

  emit locationPreferenceChanged ();
}

void
WeatherProvider::setLocationPreference (bool autoLocationEnabled,
                                        const QString &manualCity,
                                        double manualLatitude,
                                        double manualLongitude)
{
  const QString normalizedManualCity = manualCity.trimmed ();
  const bool changed
      = m_autoLocationEnabled != autoLocationEnabled
        || m_manualCity != normalizedManualCity
        || !qFuzzyCompare (m_manualLatitude + 1.0, manualLatitude + 1.0)
        || !qFuzzyCompare (m_manualLongitude + 1.0, manualLongitude + 1.0);

  m_autoLocationEnabled = autoLocationEnabled;
  m_manualCity = normalizedManualCity;
  m_manualLatitude = manualLatitude;
  m_manualLongitude = manualLongitude;

  if (changed)
    {
      emit locationPreferenceChanged ();
    }

  if (!m_initialized)
    {
      return;
    }

  ++m_locationRequestSerial;

  if (!m_autoLocationEnabled && hasManualLocationPreference ())
    {
      updateLocation (m_manualLatitude, m_manualLongitude, m_manualCity);
      return;
    }

  requestLocationUpdate ();
}

bool
WeatherProvider::autoLocationEnabled () const
{
  return m_autoLocationEnabled;
}

QString
WeatherProvider::manualLocationCity () const
{
  return m_manualCity;
}

double
WeatherProvider::manualLocationLatitude () const
{
  return m_manualLatitude;
}

double
WeatherProvider::manualLocationLongitude () const
{
  return m_manualLongitude;
}

QString
WeatherProvider::autoLocationCity () const
{
  if (!m_lastAutoCity.isEmpty ())
    {
      return m_lastAutoCity;
    }

  if (m_autoLocationEnabled)
    {
      return m_weatherData.city;
    }

  return QString ();
}

QVariantList
WeatherProvider::citySuggestions () const
{
  return m_citySuggestions;
}

bool
WeatherProvider::isSearchingCities () const
{
  return m_isSearchingCities;
}

void
WeatherProvider::initLocationSource ()
{
#ifdef QT_POSITIONING_LIB
  m_positionSource = QGeoPositionInfoSource::createDefaultSource (this);

  if (m_positionSource)
    {
      qInfo () << "Location backend initialized"
               << "backend="
               << locationBackendName (LocationBackend::QtPositioning)
               << "source=" << m_positionSource->sourceName ();
      connect (m_positionSource, &QGeoPositionInfoSource::positionUpdated,
               this, &WeatherProvider::onPositionUpdated);
      connect (m_positionSource, &QGeoPositionInfoSource::errorOccurred, this,
               &WeatherProvider::onPositionError);
    }
  else
    {
      qWarning ()
          << "Qt Positioning default source unavailable during initialization";
    }
#else
  qInfo () << "Qt Positioning not available at build time, using IP location "
              "fallback";
#endif
}

void
WeatherProvider::refresh ()
{
  triggerRefresh (QStringLiteral ("manual"), true);
}

void
WeatherProvider::onPrepareForSleep (bool sleeping)
{
  qInfo () << "System sleep state changed"
           << "sleeping=" << sleeping;

  if (sleeping)
    {
      return;
    }

  triggerRefresh (QStringLiteral ("system-resume"));
}

void
WeatherProvider::triggerRefresh (const QString &reason, bool force)
{
  const qint64 currentMs = nowMs ();
  const qint64 elapsedSinceLastRequest
      = m_lastRefreshRequestAtMs > 0 ? currentMs - m_lastRefreshRequestAtMs
                                     : -1;

  qInfo () << "Weather refresh requested"
           << "reason=" << reason << "latitude=" << m_latitude
           << "longitude=" << m_longitude << "loading=" << m_isLoading
           << "autoLocationEnabled=" << m_autoLocationEnabled
           << "locationLookupInProgress=" << m_locationLookupInProgress
           << "ipLocationPending=" << m_ipLocationRequestPending
#ifdef QT_POSITIONING_LIB
           << "positionPending=" << m_positionRequestPending
#endif
           << "elapsedSinceLastRequestMs=" << elapsedSinceLastRequest;

  if (!force && elapsedSinceLastRequest >= 0
      && elapsedSinceLastRequest < kRefreshDedupWindowMs)
    {
      qInfo () << "Skipping duplicate weather refresh"
               << "reason=" << reason
               << "elapsedSinceLastRequestMs=" << elapsedSinceLastRequest;
      resetRefreshTimer ();
      return;
    }

  if (isRefreshInProgress ())
    {
      qInfo () << "Skipping weather refresh while request is in progress"
               << "reason=" << reason;
      resetRefreshTimer ();
      return;
    }

  m_lastRefreshRequestAtMs = currentMs;
  resetRefreshTimer ();

  if (m_autoLocationEnabled)
    {
      requestLocationUpdate ();
      return;
    }

  if (m_latitude != 0 && m_longitude != 0)
    {
      fetchWeather (m_latitude, m_longitude);
    }
  else
    {
      requestLocationUpdate ();
    }
}

void
WeatherProvider::resetRefreshTimer ()
{
  m_refreshTimer->start (kRefreshIntervalMs);
}

bool
WeatherProvider::isRefreshInProgress () const
{
  return m_isLoading || m_locationLookupInProgress
         || m_ipLocationRequestPending
#ifdef QT_POSITIONING_LIB
         || m_positionRequestPending
#endif
      ;
}

void
WeatherProvider::setLocation (double latitude, double longitude)
{
  ++m_locationRequestSerial;
  updateLocation (latitude, longitude, QString ());
}

void
WeatherProvider::searchCities (const QString &query)
{
  const QString trimmedQuery = query.trimmed ();
  const quint64 requestSerial = ++m_citySearchRequestSerial;

  if (trimmedQuery.size () < 2)
    {
      m_isSearchingCities = false;
      if (!m_citySuggestions.isEmpty ())
        {
          m_citySuggestions.clear ();
          emit citySuggestionsChanged ();
        }
      return;
    }

  QUrl url ("https://geocoding-api.open-meteo.com/v1/search");
  QUrlQuery urlQuery;
  urlQuery.addQueryItem ("name", trimmedQuery);
  urlQuery.addQueryItem ("count", QStringLiteral ("8"));
  urlQuery.addQueryItem ("language", uiLanguageCode ());
  urlQuery.addQueryItem ("format", QStringLiteral ("json"));
  url.setQuery (urlQuery);

  qInfo () << "City search request start"
           << "query=" << trimmedQuery << "url=" << url.toString ();

  QNetworkRequest request (url);
  request.setRawHeader (
      "User-Agent",
      "dde-shell-weather-plugin/1.0 "
      "(https://github.com/linuxdeepin/dde-shell-weather-plugin)");
  request.setRawHeader ("Accept", "application/json");

  QNetworkReply *reply = m_networkManager->get (request);
  reply->setProperty ("requestStartedAt", nowMs ());
  reply->setProperty ("query", trimmedQuery);
  reply->setProperty ("searchSerial", static_cast<qulonglong> (requestSerial));
  m_citySearchReply = reply;
  m_isSearchingCities = true;

  connect (reply, &QNetworkReply::finished, this,
           [this, reply] () { onCitySearchReplyFinished (reply); });
}

void
WeatherProvider::requestLocationPreview ()
{
  startLocationLookup (LocationLookupPurpose::PreviewSelection);
}

void
WeatherProvider::startLocationLookup (LocationLookupPurpose purpose)
{
  if (m_locationLookupInProgress || m_ipLocationRequestPending)
    {
      qInfo () << "Location lookup already in progress";
      if (purpose == LocationLookupPurpose::PreviewSelection)
        {
          emit locationPreviewFailed (
              tr ("Location lookup already in progress"));
        }
      return;
    }

  ++m_locationRequestSerial;
  m_locationLookupPurpose = purpose;
  m_locationLookupInProgress = false;

  if (purpose == LocationLookupPurpose::PreviewSelection)
    {
      emit locationPreviewStarted ();
    }

  m_locationLookupInProgress = true;

#ifdef QT_POSITIONING_LIB
  if (m_positionSource)
    {
      if (m_positionRequestPending)
        {
          qInfo () << "Position request already pending";
          return;
        }

      m_positionRequestPending = true;
      qInfo () << "Location lookup start"
               << "backend="
               << locationBackendName (LocationBackend::QtPositioning)
               << "purpose=" << static_cast<int> (purpose)
               << "source=" << m_positionSource->sourceName ();
      ensureGeoclueAgentForLocation ();
      return;
    }

  fallbackToIpLocation (QStringLiteral ("qt-source-unavailable"));
#else
  fetchLocationFromIp (
      QStringLiteral ("qt-positioning-disabled-at-build-time"));
#endif
}

void
WeatherProvider::updateLocation (double latitude, double longitude,
                                 const QString &resolvedCity)
{
  m_locationLookupInProgress = false;

#ifdef QT_POSITIONING_LIB
  stopGeoclueAgent ();
#endif

  qInfo () << "Weather location updated"
           << "latitude=" << latitude << "longitude=" << longitude;

  m_latitude = latitude;
  m_longitude = longitude;
  if (!resolvedCity.isEmpty ())
    {
      m_weatherData.city = resolvedCity;
      if (m_autoLocationEnabled)
        {
          updateAutoLocationCache (latitude, longitude, resolvedCity);
        }
      qInfo () << "Location city updated"
               << "backend="
               << locationBackendName (LocationBackend::IpGeolocation)
               << "city=" << m_weatherData.city;
      emit weatherChanged ();
    }

  fetchWeather (latitude, longitude);
  if (resolvedCity.isEmpty ())
    {
      fetchCityName (latitude, longitude, LocationLookupPurpose::WeatherUpdate,
                     m_locationRequestSerial);
    }
}

void
WeatherProvider::handleResolvedLocation (double latitude, double longitude,
                                         const QString &resolvedCity)
{
  if (m_locationLookupPurpose == LocationLookupPurpose::PreviewSelection)
    {
      m_locationLookupInProgress = false;

#ifdef QT_POSITIONING_LIB
      stopGeoclueAgent ();
#endif

      const QString previewCity = resolvedCity.trimmed ();
      if (!previewCity.isEmpty ())
        {
          updateAutoLocationCache (latitude, longitude, previewCity);
          emit locationPreviewResolved (previewCity, latitude, longitude);
          return;
        }

      fetchCityName (latitude, longitude,
                     LocationLookupPurpose::PreviewSelection,
                     m_locationRequestSerial);
      return;
    }

  updateLocation (latitude, longitude, resolvedCity);
}

void
WeatherProvider::updateAutoLocationCache (double latitude, double longitude,
                                          const QString &city)
{
  const QString normalizedCity = city.trimmed ();
  if (normalizedCity.isEmpty ())
    {
      return;
    }

  const bool changed
      = m_lastAutoCity != normalizedCity
        || !qFuzzyCompare (m_lastAutoLatitude + 1.0, latitude + 1.0)
        || !qFuzzyCompare (m_lastAutoLongitude + 1.0, longitude + 1.0);

  m_lastAutoCity = normalizedCity;
  m_lastAutoLatitude = latitude;
  m_lastAutoLongitude = longitude;

  if (changed)
    {
      emit locationPreferenceChanged ();
    }
}

bool
WeatherProvider::hasManualLocationPreference () const
{
  if (m_manualCity.trimmed ().isEmpty ())
    {
      return false;
    }

  if (!std::isfinite (m_manualLatitude) || !std::isfinite (m_manualLongitude))
    {
      return false;
    }

  if (m_manualLatitude < -90.0 || m_manualLatitude > 90.0
      || m_manualLongitude < -180.0 || m_manualLongitude > 180.0)
    {
      return false;
    }

  return true;
}

#ifdef QT_POSITIONING_LIB
void
WeatherProvider::onPositionUpdated (const QGeoPositionInfo &info)
{
  if (!m_locationLookupInProgress)
    {
      qInfo () << "Ignoring stale position update";
      return;
    }

  QGeoCoordinate coord = info.coordinate ();
  if (!coord.isValid ())
    {
      stopGeoclueAgent ();
      qWarning () << "Position update returned invalid coordinate"
                  << "backend="
                  << locationBackendName (LocationBackend::QtPositioning)
                  << "source="
                  << (m_positionSource ? m_positionSource->sourceName ()
                                       : QStringLiteral ("unknown"));
      fallbackToIpLocation (QStringLiteral ("invalid-coordinate"));
      return;
    }

  const qreal horizontalAccuracy
      = info.hasAttribute (QGeoPositionInfo::HorizontalAccuracy)
            ? info.attribute (QGeoPositionInfo::HorizontalAccuracy)
            : -1;

  qInfo () << "Location lookup success"
           << "backend="
           << locationBackendName (LocationBackend::QtPositioning) << "source="
           << (m_positionSource ? m_positionSource->sourceName ()
                                : QStringLiteral ("unknown"))
           << "latitude=" << coord.latitude ()
           << "longitude=" << coord.longitude ()
           << "horizontalAccuracy=" << horizontalAccuracy;
  handleResolvedLocation (coord.latitude (), coord.longitude (), QString ());
}

void
WeatherProvider::onPositionError (QGeoPositionInfoSource::Error error)
{
  if (!m_locationLookupInProgress)
    {
      qInfo () << "Ignoring stale position error"
               << "error=" << positionErrorName (error);
      return;
    }

  stopGeoclueAgent ();
  qWarning () << "Location backend failed"
              << "backend="
              << locationBackendName (LocationBackend::QtPositioning)
              << "source="
              << (m_positionSource ? m_positionSource->sourceName ()
                                   : QStringLiteral ("unknown"))
              << "error=" << positionErrorName (error)
              << "errorCode=" << error;
  fallbackToIpLocation (
      QStringLiteral ("qt-position-error:%1").arg (positionErrorName (error)));
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
  if (m_geoclueAgentStopRequested && error == QProcess::Crashed)
    {
      qInfo () << "Geoclue agent exited after stop request"
               << "error=" << error;
      return;
    }

  m_geoclueAgentPrestartDisabled = true;
  qWarning () << "Geoclue agent prestart failed"
              << "error=" << error
              << "message=" << m_geoclueAgentProcess->errorString ()
              << "disablePrestartForSession=" << true;
  requestPositionUpdate ();
}

void
WeatherProvider::onGeoclueAgentFinished (int exitCode,
                                         QProcess::ExitStatus exitStatus)
{
  logGeoclueAgentOutput (m_geoclueAgentProcess);
  if (m_geoclueAgentStopRequested)
    {
      qInfo () << "Geoclue agent stopped"
               << "exitCode=" << exitCode << "exitStatus=" << exitStatus;
      m_geoclueAgentStopRequested = false;
      return;
    }

  qInfo () << "Geoclue agent finished"
           << "exitCode=" << exitCode << "exitStatus=" << exitStatus;
  requestPositionUpdate ();
}
#endif

void
WeatherProvider::requestLocationUpdate ()
{
  startLocationLookup (LocationLookupPurpose::WeatherUpdate);
}

void
WeatherProvider::fetchLocationFromIp (const QString &reason)
{
  if (m_ipLocationRequestPending)
    {
      qInfo () << "IP geolocation request already pending";
      return;
    }

  QUrl url ("http://ip-api.com/json/");
  QUrlQuery query;
  query.addQueryItem ("fields", "status,message,city,regionName,country,lat,"
                                "lon");
  query.addQueryItem ("lang", ipApiLanguageCode ());
  url.setQuery (query);

  qInfo () << "Location lookup start"
           << "backend="
           << locationBackendName (LocationBackend::IpGeolocation)
           << "reason=" << reason << "url=" << url.toString ();

  QNetworkRequest request (url);
  request.setRawHeader (
      "User-Agent",
      "dde-shell-weather-plugin/1.0 "
      "(https://github.com/linuxdeepin/dde-shell-weather-plugin)");
  request.setRawHeader ("Accept", "application/json");

  QNetworkReply *reply = m_networkManager->get (request);
  reply->setProperty ("requestStartedAt", nowMs ());
  reply->setProperty ("reason", reason);
  m_ipLocationRequestPending = true;

  connect (reply, &QNetworkReply::finished, this,
           [this, reply] () { onIpLocationReplyFinished (reply); });
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
  query.addQueryItem ("current", "temperature_2m,relative_humidity_2m,weather_"
                                 "code,wind_speed_10m,is_day");
  query.addQueryItem ("daily", "temperature_2m_max,temperature_2m_min");
  query.addQueryItem ("hourly", "temperature_2m,weather_code,is_day");
  query.addQueryItem ("forecast_hours", QStringLiteral ("24"));
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
WeatherProvider::fetchCityName (double latitude, double longitude,
                                LocationLookupPurpose purpose,
                                quint64 requestSerial)
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
  reply->setProperty ("latitude", latitude);
  reply->setProperty ("longitude", longitude);
  reply->setProperty ("locationPurpose", static_cast<int> (purpose));
  reply->setProperty ("requestSerial",
                      static_cast<qulonglong> (requestSerial));

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

  m_weatherData.updatedAt = QDateTime::currentDateTimeUtc ();

  qInfo () << "Weather parse success"
           << "backend=" << backendName (backend)
           << "providerName=" << m_providerName
           << "temperature=" << m_weatherData.temperature
           << "humidity=" << m_weatherData.humidity
           << "windSpeed=" << m_weatherData.windSpeed
           << "weatherCode=" << m_weatherData.weatherCode
           << "hourlyForecastEntries=" << m_hourlyForecast.size ()
           << "elapsedMs=" << elapsedMs;

  finishWeatherRequest ();
  m_weatherData.isValid = true;
  emit weatherChanged ();
}

void
WeatherProvider::onCityReplyFinished (QNetworkReply *reply)
{
  const double latitude = reply->property ("latitude").toDouble ();
  const double longitude = reply->property ("longitude").toDouble ();
  const quint64 requestSerial
      = reply->property ("requestSerial").toULongLong ();
  const LocationLookupPurpose purpose = static_cast<LocationLookupPurpose> (
      reply->property ("locationPurpose").toInt ());
  const qint64 startedAt = reply->property ("requestStartedAt").toLongLong ();
  const qint64 elapsedMs = startedAt > 0 ? nowMs () - startedAt : -1;
  const QVariant httpStatus
      = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute);

  qInfo () << "City request finished"
           << "elapsedMs=" << elapsedMs << "httpStatus=" << httpStatus
           << "networkError=" << reply->error ();

  reply->deleteLater ();

  if (requestSerial != 0 && requestSerial != m_locationRequestSerial)
    {
      qInfo () << "Ignoring stale city reply"
               << "replySerial=" << requestSerial
               << "currentSerial=" << m_locationRequestSerial;
      return;
    }

  if (reply->error () != QNetworkReply::NoError)
    {
      qWarning () << "City API error"
                  << "elapsedMs=" << elapsedMs
                  << "message=" << reply->errorString ();
      const QString unknownCity = tr ("Unknown");
      if (purpose == LocationLookupPurpose::PreviewSelection)
        {
          updateAutoLocationCache (latitude, longitude, unknownCity);
          emit locationPreviewResolved (unknownCity, latitude, longitude);
          return;
        }

      m_weatherData.city = unknownCity;
      if (m_autoLocationEnabled)
        {
          updateAutoLocationCache (latitude, longitude, m_weatherData.city);
        }
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

  const QString resolvedCity = city.isEmpty () ? tr ("Unknown") : city;
  qInfo () << "City parse success"
           << "city=" << resolvedCity << "elapsedMs=" << elapsedMs;

  if (purpose == LocationLookupPurpose::PreviewSelection)
    {
      updateAutoLocationCache (latitude, longitude, resolvedCity);
      emit locationPreviewResolved (resolvedCity, latitude, longitude);
      return;
    }

  m_weatherData.city = resolvedCity;
  if (m_autoLocationEnabled)
    {
      updateAutoLocationCache (latitude, longitude, m_weatherData.city);
    }
  emit weatherChanged ();
}

void
WeatherProvider::onCitySearchReplyFinished (QNetworkReply *reply)
{
  const quint64 requestSerial
      = reply->property ("searchSerial").toULongLong ();
  const bool isLatestReply = requestSerial == m_citySearchRequestSerial;

  if (reply == m_citySearchReply)
    {
      m_citySearchReply = nullptr;
    }

  const qint64 startedAt = reply->property ("requestStartedAt").toLongLong ();
  const qint64 elapsedMs = startedAt > 0 ? nowMs () - startedAt : -1;
  const QString query = reply->property ("query").toString ();
  const QVariant httpStatus
      = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute);

  qInfo () << "City search request finished"
           << "query=" << query << "elapsedMs=" << elapsedMs
           << "httpStatus=" << httpStatus
           << "networkError=" << reply->error ();

  if (!isLatestReply)
    {
      qInfo () << "Ignoring stale city search reply"
               << "replySerial=" << requestSerial
               << "currentSerial=" << m_citySearchRequestSerial;
      reply->deleteLater ();
      return;
    }

  m_isSearchingCities = false;

  QVariantList suggestions;

  if (reply->error () == QNetworkReply::NoError)
    {
      const QJsonDocument doc = QJsonDocument::fromJson (reply->readAll ());
      const QJsonArray results = doc.object ()["results"].toArray ();

      for (const QJsonValue &value : results)
        {
          const QJsonObject item = value.toObject ();
          const QString name = item["name"].toString ().trimmed ();
          if (name.isEmpty ())
            {
              continue;
            }

          const QString admin1 = item["admin1"].toString ().trimmed ();
          const QString country = item["country"].toString ().trimmed ();

          QString shortName = name;
          if (!admin1.isEmpty () && admin1 != name)
            {
              shortName += QStringLiteral (", ") + admin1;
            }

          QString displayName = shortName;
          if (!country.isEmpty () && country != admin1 && country != name)
            {
              displayName += QStringLiteral (", ") + country;
            }

          suggestions.append (QVariantMap{
              { "name", name },
              { "shortName", shortName },
              { "displayName", displayName },
              { "latitude", item["latitude"].toDouble () },
              { "longitude", item["longitude"].toDouble () },
              { "country", country },
              { "admin1", admin1 },
          });
        }
    }
  else if (reply->error () != QNetworkReply::OperationCanceledError)
    {
      qWarning () << "City search failed"
                  << "query=" << query << "elapsedMs=" << elapsedMs
                  << "message=" << reply->errorString ();
    }

  reply->deleteLater ();

  if (m_citySuggestions != suggestions)
    {
      m_citySuggestions = suggestions;
      emit citySuggestionsChanged ();
    }
}

void
WeatherProvider::onIpLocationReplyFinished (QNetworkReply *reply)
{
  const qint64 startedAt = reply->property ("requestStartedAt").toLongLong ();
  const qint64 elapsedMs = startedAt > 0 ? nowMs () - startedAt : -1;
  const QString reason = reply->property ("reason").toString ();
  const QVariant httpStatus
      = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute);

  qInfo () << "IP geolocation request finished"
           << "elapsedMs=" << elapsedMs << "httpStatus=" << httpStatus
           << "networkError=" << reply->error () << "reason=" << reason;

  m_ipLocationRequestPending = false;
  const QByteArray data = reply->readAll ();
  reply->deleteLater ();

  if (!m_locationLookupInProgress)
    {
      qInfo () << "Ignoring stale IP geolocation reply"
               << "elapsedMs=" << elapsedMs;
      return;
    }

  if (reply->error () != QNetworkReply::NoError)
    {
      qWarning () << "IP geolocation failed"
                  << "elapsedMs=" << elapsedMs << "reason=" << reason
                  << "message=" << reply->errorString ();
      useDefaultLocation (
          QStringLiteral ("ip-network-error:%1").arg (reply->errorString ()));
      return;
    }

  const QJsonDocument doc = QJsonDocument::fromJson (data);
  const QJsonObject root = doc.object ();
  const QString status = root["status"].toString ();
  if (status == QStringLiteral ("fail"))
    {
      const QString message = root["message"].toString ();
      qWarning () << "IP geolocation request rejected"
                  << "elapsedMs=" << elapsedMs << "reason=" << reason
                  << "message=" << message;
      useDefaultLocation (QStringLiteral ("ip-api-failed:%1").arg (message));
      return;
    }

  double latitude = 0;
  double longitude = 0;

  if (!parseIpLocation (root, &latitude, &longitude))
    {
      qWarning () << "IP geolocation payload missing usable coordinates"
                  << "elapsedMs=" << elapsedMs << "reason=" << reason
                  << "status=" << status;
      useDefaultLocation (QStringLiteral ("ip-invalid-payload"));
      return;
    }

  QString city = root["city"].toString ().trimmed ();
  if (city.isEmpty ())
    {
      city = root["regionName"].toString ().trimmed ();
    }
  if (city.isEmpty ())
    {
      city = root["country"].toString ().trimmed ();
    }

  qInfo () << "Location lookup success"
           << "backend="
           << locationBackendName (LocationBackend::IpGeolocation)
           << "latitude=" << latitude << "longitude=" << longitude
           << "city=" << city << "elapsedMs=" << elapsedMs;
  handleResolvedLocation (latitude, longitude, city);
}

bool
WeatherProvider::parseIpLocation (const QJsonObject &root, double *latitude,
                                  double *longitude) const
{
  if (!latitude || !longitude)
    {
      return false;
    }

  bool hasCoordinates = jsonValueToDouble (root["lat"], latitude)
                        && jsonValueToDouble (root["lon"], longitude);
  if (!hasCoordinates)
    {
      hasCoordinates = jsonValueToDouble (root["latitude"], latitude)
                       && jsonValueToDouble (root["longitude"], longitude);
    }

  if (!hasCoordinates)
    {
      return false;
    }

  if (!std::isfinite (*latitude) || !std::isfinite (*longitude))
    {
      return false;
    }

  if (*latitude < -90.0 || *latitude > 90.0 || *longitude < -180.0
      || *longitude > 180.0)
    {
      return false;
    }

  return true;
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
  m_weatherData.iconName = iconNameForMetNoSymbol (symbolCode);
  m_weatherData.weatherDescription = parseMetNoSymbolCode (symbolCode);
  const QVariantList parsedHourlyForecast
      = parseMetNoHourlyForecast (timeseries);

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
  m_hourlyForecast = parsedHourlyForecast;

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
  const QVariantList parsedHourlyForecast
      = parseOpenMeteoHourlyForecast (root);

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
  const bool isDay
      = !current.contains ("is_day") || current["is_day"].toInt () != 0;
  m_weatherData.weatherCode = QString::number (weatherCode);
  m_weatherData.iconName = iconNameForOpenMeteoCode (weatherCode, isDay);
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

  m_hourlyForecast = parsedHourlyForecast;

  return true;
}

QVariantList
WeatherProvider::parseMetNoHourlyForecast (const QJsonArray &timeseries) const
{
  QVariantList forecast;

  for (const QJsonValue &entryValue : timeseries)
    {
      if (forecast.size () >= 24)
        {
          break;
        }

      const QJsonObject entry = entryValue.toObject ();
      const QDateTime timestamp
          = QDateTime::fromString (entry["time"].toString (), Qt::ISODate);
      if (!timestamp.isValid ())
        {
          continue;
        }

      const QJsonObject data = entry["data"].toObject ();
      const QJsonObject details
          = data["instant"].toObject ()["details"].toObject ();
      if (!details.contains ("air_temperature"))
        {
          continue;
        }

      QString weatherCode = data["next_1_hours"]
                                .toObject ()["summary"]
                                .toObject ()["symbol_code"]
                                .toString ();
      if (weatherCode.isEmpty ())
        {
          weatherCode = data["next_6_hours"]
                            .toObject ()["summary"]
                            .toObject ()["symbol_code"]
                            .toString ();
        }
      if (weatherCode.isEmpty ())
        {
          weatherCode = data["next_12_hours"]
                            .toObject ()["summary"]
                            .toObject ()["symbol_code"]
                            .toString ();
        }
      if (weatherCode.isEmpty ())
        {
          continue;
        }

      forecast.append (buildHourlyForecastEntry (
          timestamp, weatherCode, iconNameForMetNoSymbol (weatherCode),
          details["air_temperature"].toDouble ()));
    }

  return forecast;
}

QVariantList
WeatherProvider::parseOpenMeteoHourlyForecast (const QJsonObject &root) const
{
  QVariantList forecast;

  const QJsonObject hourly = root["hourly"].toObject ();
  const QJsonArray times = hourly["time"].toArray ();
  const QJsonArray temperatures = hourly["temperature_2m"].toArray ();
  const QJsonArray weatherCodes = hourly["weather_code"].toArray ();
  const QJsonArray isDayValues = hourly["is_day"].toArray ();
  const int count = qMin (times.size (),
                          qMin (temperatures.size (), weatherCodes.size ()));

  for (int index = 0; index < count && forecast.size () < 24; ++index)
    {
      const QDateTime timestamp
          = QDateTime::fromString (times[index].toString (), Qt::ISODate);
      if (!timestamp.isValid ())
        {
          continue;
        }

      const int weatherCode = weatherCodes[index].toInt ();
      const bool isDay
          = index >= isDayValues.size () || isDayValues[index].toInt () != 0;
      const QString weatherCodeString = QString::number (weatherCode);

      forecast.append (buildHourlyForecastEntry (
          timestamp, weatherCodeString,
          iconNameForOpenMeteoCode (weatherCode, isDay),
          temperatures[index].toDouble ()));
    }

  return forecast;
}

QVariantMap
WeatherProvider::buildHourlyForecastEntry (const QDateTime &time,
                                           const QString &weatherCode,
                                           const QString &iconName,
                                           double temperature) const
{
  const QLocale locale = formatLocale ();
  const double displayTemperature
      = celsiusToDisplayValue (temperature, locale);

  return QVariantMap{
    { "time", time },
    { "displayHour", formatHourlyDisplayTime (time) },
    { "weatherCode", weatherCode },
    { "iconName", iconName },
    { "temperature", temperature },
    { "formattedTemperature",
      tr ("%1%2")
          .arg (locale.toString (qRound (displayTemperature)))
          .arg (temperatureUnitForLocale (locale)) },
  };
}

QString
WeatherProvider::formatHourlyDisplayTime (const QDateTime &time) const
{
  if (!time.isValid ())
    {
      return QString ();
    }

  const QString label = formatLocale ().toString (time.toLocalTime ().time (),
                                                  QLocale::ShortFormat);
  if (!label.isEmpty ())
    {
      return label;
    }

  return time.toLocalTime ().toString (QStringLiteral ("HH:mm"));
}

QString
WeatherProvider::iconNameForMetNoSymbol (const QString &symbolCode) const
{
  const QString normalized = symbolCode.toLower ();
  const bool preferNight
      = normalized.contains (QStringLiteral ("night"))
        || normalized.contains (QStringLiteral ("polartwilight"));

  if (normalized.contains (QStringLiteral ("clearsky")))
    {
      return preferNight ? QStringLiteral ("clear-night")
                         : QStringLiteral ("clear-day");
    }
  if (normalized.contains (QStringLiteral ("partlycloudy")))
    {
      return preferNight ? QStringLiteral ("cloudy-2-night")
                         : QStringLiteral ("cloudy-2-day");
    }
  if (normalized.contains (QStringLiteral ("fair")))
    {
      return preferNight ? QStringLiteral ("cloudy-1-night")
                         : QStringLiteral ("cloudy-1-day");
    }
  if (normalized.contains (QStringLiteral ("fog")))
    {
      return preferNight ? QStringLiteral ("fog-night")
                         : QStringLiteral ("fog-day");
    }
  if (normalized.contains (QStringLiteral ("thunder")))
    {
      return preferNight ? QStringLiteral ("scattered-thunderstorms-night")
                         : QStringLiteral ("scattered-thunderstorms-day");
    }
  if (normalized.contains (QStringLiteral ("sleet")))
    {
      return QStringLiteral ("rain-and-sleet-mix");
    }
  if (normalized.contains (QStringLiteral ("snowshowers")))
    {
      return preferNight ? QStringLiteral ("snowy-1-night")
                         : QStringLiteral ("snowy-1-day");
    }
  if (normalized.contains (QStringLiteral ("snow")))
    {
      return preferNight ? QStringLiteral ("snowy-2-night")
                         : QStringLiteral ("snowy-2-day");
    }
  if (normalized.contains (QStringLiteral ("drizzle")))
    {
      return preferNight ? QStringLiteral ("rainy-1-night")
                         : QStringLiteral ("rainy-1-day");
    }
  if (normalized.contains (QStringLiteral ("rainshowers")))
    {
      return preferNight ? QStringLiteral ("rainy-1-night")
                         : QStringLiteral ("rainy-1-day");
    }
  if (normalized.contains (QStringLiteral ("rain")))
    {
      return preferNight ? QStringLiteral ("rainy-3-night")
                         : QStringLiteral ("rainy-3-day");
    }
  if (normalized.contains (QStringLiteral ("cloudy")))
    {
      return preferNight ? QStringLiteral ("cloudy-3-night")
                         : QStringLiteral ("cloudy");
    }

  return preferNight ? QStringLiteral ("cloudy-3-night")
                     : QStringLiteral ("cloudy");
}

QString
WeatherProvider::iconNameForOpenMeteoCode (int code, bool isDay) const
{
  const QString cloudyIcon
      = isDay ? QStringLiteral ("cloudy") : QStringLiteral ("cloudy-3-night");

  if (code == 0)
    {
      return isDay ? QStringLiteral ("clear-day")
                   : QStringLiteral ("clear-night");
    }
  if (code == 1)
    {
      return isDay ? QStringLiteral ("cloudy-1-day")
                   : QStringLiteral ("cloudy-1-night");
    }
  if (code == 2)
    {
      return isDay ? QStringLiteral ("cloudy-2-day")
                   : QStringLiteral ("cloudy-2-night");
    }
  if (code == 3)
    {
      return cloudyIcon;
    }
  if (code >= 45 && code <= 48)
    {
      return isDay ? QStringLiteral ("fog-day") : QStringLiteral ("fog-night");
    }
  if (code >= 51 && code <= 53)
    {
      return isDay ? QStringLiteral ("rainy-1-day")
                   : QStringLiteral ("rainy-1-night");
    }
  if (code >= 55 && code <= 57)
    {
      return isDay ? QStringLiteral ("rainy-2-day")
                   : QStringLiteral ("rainy-2-night");
    }
  if (code >= 61 && code <= 65)
    {
      return isDay ? QStringLiteral ("rainy-3-day")
                   : QStringLiteral ("rainy-3-night");
    }
  if (code >= 66 && code <= 67)
    {
      return QStringLiteral ("rain-and-sleet-mix");
    }
  if (code >= 71 && code <= 73)
    {
      return isDay ? QStringLiteral ("snowy-1-day")
                   : QStringLiteral ("snowy-1-night");
    }
  if (code >= 74 && code <= 77)
    {
      return isDay ? QStringLiteral ("snowy-2-day")
                   : QStringLiteral ("snowy-2-night");
    }
  if (code >= 80 && code <= 81)
    {
      return isDay ? QStringLiteral ("rainy-1-day")
                   : QStringLiteral ("rainy-1-night");
    }
  if (code == 82)
    {
      return isDay ? QStringLiteral ("rainy-2-day")
                   : QStringLiteral ("rainy-2-night");
    }
  if (code >= 85 && code <= 86)
    {
      return isDay ? QStringLiteral ("snowy-1-day")
                   : QStringLiteral ("snowy-1-night");
    }
  if (code >= 95 && code <= 99)
    {
      return isDay ? QStringLiteral ("scattered-thunderstorms-day")
                   : QStringLiteral ("scattered-thunderstorms-night");
    }

  return cloudyIcon;
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
WeatherProvider::locationBackendName (LocationBackend backend)
{
  switch (backend)
    {
#ifdef QT_POSITIONING_LIB
    case LocationBackend::QtPositioning:
      return QStringLiteral ("Qt Positioning");
#endif
    case LocationBackend::IpGeolocation:
      return QStringLiteral ("IP geolocation");
    case LocationBackend::DefaultLocation:
      return QStringLiteral ("Default location");
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

QVariantList
WeatherProvider::hourlyForecast () const
{
  return m_hourlyForecast;
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
WeatherProvider::formattedUpdatedAt () const
{
  if (!m_weatherData.updatedAt.isValid ())
    {
      return tr ("Unavailable");
    }

  return formatLocale ().toString (m_weatherData.updatedAt.toLocalTime (),
                                   QLocale::ShortFormat);
}

QString
WeatherProvider::tooltipText () const
{
  const QString displayCity = m_weatherData.city.trimmed ().isEmpty ()
                                  ? tr ("Unknown")
                                  : m_weatherData.city.trimmed ();
  const QString displayDescription
      = m_weatherData.weatherDescription.trimmed ().isEmpty ()
            ? tr ("Unknown")
            : m_weatherData.weatherDescription.trimmed ();

  return tr ("Location: %1\nWeather: %2\nUpdated: %3")
      .arg (displayCity, displayDescription, formattedUpdatedAt ());
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
WeatherProvider::fallbackToIpLocation (const QString &reason)
{
  qWarning () << "Location backend fallback"
              << "from="
              << locationBackendName (LocationBackend::QtPositioning)
              << "to=" << locationBackendName (LocationBackend::IpGeolocation)
              << "reason=" << reason;
  fetchLocationFromIp (reason);
}

void
WeatherProvider::ensureGeoclueAgentForLocation ()
{
  if (!m_geoclueAgentProcess)
    {
      requestPositionUpdate ();
      return;
    }

  if (m_geoclueAgentPrestartDisabled)
    {
      qInfo () << "Geoclue agent prestart disabled for this session, "
                  "requesting location without authorization helper";
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
  m_geoclueAgentStopRequested = false;
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
  m_geoclueAgentStopRequested = true;
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

void
WeatherProvider::useDefaultLocation (const QString &reason)
{
  m_locationLookupInProgress = false;

#ifdef QT_POSITIONING_LIB
  stopGeoclueAgent ();
#endif

  if (m_locationLookupPurpose == LocationLookupPurpose::PreviewSelection)
    {
      qWarning () << "Location preview failed"
                  << "reason=" << reason;
      emit locationPreviewFailed (tr ("Failed to get location"));
      return;
    }

  qWarning () << "Location backend fallback"
              << "from="
              << locationBackendName (LocationBackend::IpGeolocation) << "to="
              << locationBackendName (LocationBackend::DefaultLocation)
              << "reason=" << reason << "latitude=" << kDefaultLatitude
              << "longitude=" << kDefaultLongitude;

  m_hasError = true;
  m_errorMessage = tr ("Failed to get location");
  emit errorChanged ();

  setLocation (kDefaultLatitude, kDefaultLongitude);
}
