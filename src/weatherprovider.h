// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>

#ifdef QT_POSITIONING_LIB
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QProcess>
#endif

struct WeatherData
{
  QString city;
  QString weatherCode;
  QString iconName;
  double temperature;
  double temperatureMax;
  double temperatureMin;
  double humidity;
  double windSpeed;
  QString weatherDescription;
  QDateTime updatedAt;
  bool isValid;

  WeatherData ()
      : temperature (0), temperatureMax (0), temperatureMin (0), humidity (0),
        windSpeed (0), isValid (false)
  {
  }
};

class WeatherProvider : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString city READ city NOTIFY weatherChanged)
  Q_PROPERTY (QString weatherCode READ weatherCode NOTIFY weatherChanged)
  Q_PROPERTY (QString iconName READ iconName NOTIFY weatherChanged)
  Q_PROPERTY (double temperature READ temperature NOTIFY weatherChanged)
  Q_PROPERTY (double humidity READ humidity NOTIFY weatherChanged)
  Q_PROPERTY (double windSpeed READ windSpeed NOTIFY weatherChanged)
  Q_PROPERTY (
      QString weatherDescription READ weatherDescription NOTIFY weatherChanged)
  Q_PROPERTY (bool isLoading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY (bool hasError READ hasError NOTIFY errorChanged)
  Q_PROPERTY (QString errorMessage READ errorMessage NOTIFY errorChanged)
  Q_PROPERTY (double temperatureMax READ temperatureMax NOTIFY weatherChanged)
  Q_PROPERTY (double temperatureMin READ temperatureMin NOTIFY weatherChanged)
  Q_PROPERTY (QString providerName READ providerName NOTIFY weatherChanged)
  Q_PROPERTY (QVariantList candidateServices READ candidateServices CONSTANT)
  Q_PROPERTY (
      QVariantList hourlyForecast READ hourlyForecast NOTIFY weatherChanged)
  Q_PROPERTY (QString formattedTemperature READ formattedTemperature NOTIFY
                  weatherChanged)
  Q_PROPERTY (QString formattedTemperatureRange READ formattedTemperatureRange
                  NOTIFY weatherChanged)
  Q_PROPERTY (
      QString formattedWindSpeed READ formattedWindSpeed NOTIFY weatherChanged)
  Q_PROPERTY (
      QString formattedUpdatedAt READ formattedUpdatedAt NOTIFY weatherChanged)
  Q_PROPERTY (QString tooltipText READ tooltipText NOTIFY weatherChanged)
  Q_PROPERTY (
      QString temperatureUnit READ temperatureUnit NOTIFY weatherChanged)
  Q_PROPERTY (QString windSpeedUnit READ windSpeedUnit NOTIFY weatherChanged)

public:
  explicit WeatherProvider (QObject *parent = nullptr);
  ~WeatherProvider ();

  void initialize ();
  void restoreLocationPreference (bool autoLocationEnabled,
                                  const QString &manualCity,
                                  double manualLatitude,
                                  double manualLongitude);
  void setLocationPreference (bool autoLocationEnabled,
                              const QString &manualCity, double manualLatitude,
                              double manualLongitude);

  // Property getters
  QString
  city () const
  {
    return m_weatherData.city;
  }
  QString
  weatherCode () const
  {
    return m_weatherData.weatherCode;
  }
  QString
  iconName () const
  {
    return m_weatherData.iconName;
  }
  double
  temperature () const
  {
    return m_weatherData.temperature;
  }
  double
  humidity () const
  {
    return m_weatherData.humidity;
  }
  double
  windSpeed () const
  {
    return m_weatherData.windSpeed;
  }
  QString
  weatherDescription () const
  {
    return m_weatherData.weatherDescription;
  }

  double
  temperatureMax () const
  {
    return m_weatherData.temperatureMax;
  }
  double
  temperatureMin () const
  {
    return m_weatherData.temperatureMin;
  }
  bool
  isLoading () const
  {
    return m_isLoading;
  }
  bool
  hasError () const
  {
    return m_hasError;
  }
  QString
  errorMessage () const
  {
    return m_errorMessage;
  }
  QString
  providerName () const
  {
    return m_providerName;
  }
  QVariantList candidateServices () const;
  QVariantList hourlyForecast () const;
  QString formattedTemperature () const;
  QString formattedTemperatureRange () const;
  QString formattedWindSpeed () const;
  QString formattedUpdatedAt () const;
  QString tooltipText () const;
  QString temperatureUnit () const;
  QString windSpeedUnit () const;
  bool autoLocationEnabled () const;
  QString manualLocationCity () const;
  double manualLocationLatitude () const;
  double manualLocationLongitude () const;
  QString autoLocationCity () const;
  QVariantList citySuggestions () const;
  bool isSearchingCities () const;

  Q_INVOKABLE void refresh ();
  Q_INVOKABLE void setLocation (double latitude, double longitude);
  Q_INVOKABLE void searchCities (const QString &query);
  Q_INVOKABLE void requestLocationPreview ();

signals:
  void weatherChanged ();
  void loadingChanged ();
  void errorChanged ();
  void locationPreferenceChanged ();
  void citySuggestionsChanged ();
  void locationPreviewStarted ();
  void locationPreviewResolved (const QString &city, double latitude,
                                double longitude);
  void locationPreviewFailed (const QString &message);

private slots:
  void onWeatherReplyFinished (QNetworkReply *reply);
  void onCityReplyFinished (QNetworkReply *reply);
  void onCitySearchReplyFinished (QNetworkReply *reply);
  void onIpLocationReplyFinished (QNetworkReply *reply);
  void onPrepareForSleep (bool sleeping);

#ifdef QT_POSITIONING_LIB
  void onPositionUpdated (const QGeoPositionInfo &info);
  void onPositionError (QGeoPositionInfoSource::Error error);
  void onGeoclueAgentStarted ();
  void onGeoclueAgentErrorOccurred (QProcess::ProcessError error);
  void onGeoclueAgentFinished (int exitCode, QProcess::ExitStatus exitStatus);
#endif

private:
  enum class WeatherBackend
  {
    MetNo,
    OpenMeteo,
  };

  enum class LocationLookupPurpose
  {
    WeatherUpdate,
    PreviewSelection,
  };

  enum class LocationBackend
  {
#ifdef QT_POSITIONING_LIB
    QtPositioning,
#endif
    IpGeolocation,
    DefaultLocation,
  };

  void startLocationLookup (LocationLookupPurpose purpose);
  void initLocationSource ();
  void requestLocationUpdate ();
  void triggerRefresh (const QString &reason, bool force = false);
  void resetRefreshTimer ();
  bool isRefreshInProgress () const;
  void fetchLocationFromIp (const QString &reason);
  void useDefaultLocation (const QString &reason);
  void fetchWeather (double latitude, double longitude);
  void updateLocation (double latitude, double longitude,
                       const QString &resolvedCity);
  void handleResolvedLocation (double latitude, double longitude,
                               const QString &resolvedCity);
  void updateAutoLocationCache (double latitude, double longitude,
                                const QString &city);
  void fetchWeatherFromBackend (WeatherBackend backend, double latitude,
                                double longitude);
  void fetchMetNoWeather (double latitude, double longitude);
  void fetchOpenMeteoWeather (double latitude, double longitude);
  void fetchCityName (double latitude, double longitude,
                      LocationLookupPurpose purpose, quint64 requestSerial);
  bool hasManualLocationPreference () const;
  bool parseIpLocation (const QJsonObject &root, double *latitude,
                        double *longitude) const;
  bool parseMetNoWeather (const QJsonObject &root);
  bool parseOpenMeteoWeather (const QJsonObject &root);
  QVariantList parseMetNoHourlyForecast (const QJsonArray &timeseries) const;
  QVariantList parseOpenMeteoHourlyForecast (const QJsonObject &root) const;
  void finishWeatherRequest ();
  bool fallbackToNextBackend (WeatherBackend backend, double latitude,
                              double longitude);
  static QString backendName (WeatherBackend backend);
  static QString locationBackendName (LocationBackend backend);
  QVariantMap buildHourlyForecastEntry (const QDateTime &time,
                                        const QString &weatherCode,
                                        const QString &iconName,
                                        double temperature) const;
  QString formatHourlyDisplayTime (const QDateTime &time) const;
  QString iconNameForMetNoSymbol (const QString &symbolCode) const;
  QString iconNameForOpenMeteoCode (int code, bool isDay) const;
  QString parseOpenMeteoWeatherCode (int code);
  QString parseMetNoSymbolCode (const QString &symbolCode);
  QVariantList buildCandidateServices () const;
#ifdef QT_POSITIONING_LIB
  void fallbackToIpLocation (const QString &reason);
  void ensureGeoclueAgentForLocation ();
  void requestPositionUpdate ();
  void stopGeoclueAgent ();
  QString geoclueAgentProgram () const;
#endif

private:
  QNetworkAccessManager *m_networkManager;
  QTimer *m_refreshTimer;
  WeatherData m_weatherData;
  double m_latitude;
  double m_longitude;
  bool m_isLoading;
  bool m_hasError;
  QString m_errorMessage;
  QString m_providerName;
  bool m_initialized;
  bool m_autoLocationEnabled;
  QString m_manualCity;
  double m_manualLatitude;
  double m_manualLongitude;
  QString m_lastAutoCity;
  double m_lastAutoLatitude;
  double m_lastAutoLongitude;
  QVariantList m_hourlyForecast;
  QVariantList m_citySuggestions;
  QPointer<QNetworkReply> m_citySearchReply;
  bool m_isSearchingCities;
  quint64 m_citySearchRequestSerial;
  LocationLookupPurpose m_locationLookupPurpose;
  quint64 m_locationRequestSerial;

#ifdef QT_POSITIONING_LIB
  QGeoPositionInfoSource *m_positionSource;
  QProcess *m_geoclueAgentProcess;
#else
  void *m_positionSource;      // Placeholder when positioning not available
  void *m_geoclueAgentProcess; // Placeholder when positioning not available
#endif
  bool m_geoclueAgentPrestartDisabled;
  bool m_geoclueAgentStopRequested;
  bool m_positionRequestPending;
  bool m_locationLookupInProgress;
  bool m_ipLocationRequestPending;
  qint64 m_lastRefreshRequestAtMs;
};
