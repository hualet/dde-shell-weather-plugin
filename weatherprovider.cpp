// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "weatherprovider.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#ifdef QT_POSITIONING_LIB
#include <QGeoPositionInfoSource>
#endif

WeatherProvider::WeatherProvider(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_positionSource(nullptr)
    , m_refreshTimer(new QTimer(this))
    , m_latitude(0)
    , m_longitude(0)
    , m_isLoading(false)
    , m_hasError(false)
{
    // Setup refresh timer (update every 30 minutes)
    connect(m_refreshTimer, &QTimer::timeout, this, &WeatherProvider::refresh);
    m_refreshTimer->start(30 * 60 * 1000);

    // Initialize location source
    initLocationSource();
}

WeatherProvider::~WeatherProvider()
{
#ifdef QT_POSITIONING_LIB
    if (m_positionSource) {
        m_positionSource->stopUpdates();
    }
#endif
}

void WeatherProvider::initLocationSource()
{
#ifdef QT_POSITIONING_LIB
    m_positionSource = QGeoPositionInfoSource::createDefaultSource(this);
    
    if (m_positionSource) {
        connect(m_positionSource, &QGeoPositionInfoSource::positionUpdated,
                this, &WeatherProvider::onPositionUpdated);
        connect(m_positionSource, QOverload<QGeoPositionInfoSource::Error>::of(&QGeoPositionInfoSource::error),
                this, &WeatherProvider::onPositionError);
        
        // Request position update
        m_positionSource->requestUpdate(10000); // 10 seconds timeout
        return;
    }
#endif
    
    qWarning() << "Position source not available, using default location";
    // Use Beijing as default
    setLocation(39.9042, 116.4074);
}

void WeatherProvider::refresh()
{
    if (m_latitude != 0 && m_longitude != 0) {
        fetchWeather(m_latitude, m_longitude);
#ifdef QT_POSITIONING_LIB
    } else if (m_positionSource) {
        m_positionSource->requestUpdate(10000);
#endif
    }
}

void WeatherProvider::setLocation(double latitude, double longitude)
{
    m_latitude = latitude;
    m_longitude = longitude;
    fetchWeather(latitude, longitude);
    fetchCityName(latitude, longitude);
}

#ifdef QT_POSITIONING_LIB
void WeatherProvider::onPositionUpdated(const QGeoPositionInfo &info)
{
    QGeoCoordinate coord = info.coordinate();
    if (coord.isValid()) {
        setLocation(coord.latitude(), coord.longitude());
    }
}

void WeatherProvider::onPositionError(QGeoPositionInfoSource::Error error)
{
    qWarning() << "Position error:" << error;
    m_hasError = true;
    m_errorMessage = tr("Failed to get location");
    emit errorChanged();
    
    // Use default location
    setLocation(39.9042, 116.4074);
}
#endif

void WeatherProvider::fetchWeather(double latitude, double longitude)
{
    m_isLoading = true;
    m_hasError = false;
    emit loadingChanged();

    // Open-Meteo API endpoint
    QUrl url("https://api.open-meteo.com/v1/forecast");
    QUrlQuery query;
    query.addQueryItem("latitude", QString::number(latitude));
    query.addQueryItem("longitude", QString::number(longitude));
    query.addQueryItem("current", "temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m");
    query.addQueryItem("timezone", "auto");
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onWeatherReplyFinished(reply);
    });
}

void WeatherProvider::fetchCityName(double latitude, double longitude)
{
    // Use BigData Cloud free reverse geocoding API (no auth required)
    QUrl url("https://api.bigdatacloud.net/data/reverse-geocode-client");
    QUrlQuery query;
    query.addQueryItem("latitude", QString::number(latitude));
    query.addQueryItem("longitude", QString::number(longitude));
    query.addQueryItem("localityLanguage", "en");
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onCityReplyFinished(reply);
    });
}

void WeatherProvider::onWeatherReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    m_isLoading = false;
    emit loadingChanged();

    if (reply->error() != QNetworkReply::NoError) {
        m_hasError = true;
        m_errorMessage = reply->errorString();
        emit errorChanged();
        qWarning() << "Weather API error:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    
    if (root.contains("current")) {
        QJsonObject current = root["current"].toObject();
        
        m_weatherData.temperature = current["temperature_2m"].toDouble();
        m_weatherData.humidity = current["relative_humidity_2m"].toDouble();
        m_weatherData.windSpeed = current["wind_speed_10m"].toDouble();
        
        int weatherCode = current["weather_code"].toInt();
        m_weatherData.weatherCode = QString::number(weatherCode);
        m_weatherData.weatherDescription = parseWeatherCode(weatherCode);
        m_weatherData.isValid = true;
        
        emit weatherChanged();
    }
}

void WeatherProvider::onCityReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "City API error:" << reply->errorString();
        m_weatherData.city = tr("Unknown");
        emit weatherChanged();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    
    // BigData Cloud returns city directly
    QString city = root["city"].toString();
    if (city.isEmpty()) {
        city = root["locality"].toString();
    }
    if (city.isEmpty()) {
        city = root["principalSubdivision"].toString();
    }
    
    m_weatherData.city = city.isEmpty() ? tr("Unknown") : city;
    emit weatherChanged();
}

QString WeatherProvider::parseWeatherCode(int code)
{
    // WMO Weather interpretation codes (WW)
    // https://open-meteo.com/en/docs
    
    if (code == 0) return tr("Clear sky");
    if (code >= 1 && code <= 3) return tr("Partly cloudy");
    if (code >= 45 && code <= 48) return tr("Fog");
    if (code >= 51 && code <= 57) return tr("Drizzle");
    if (code >= 61 && code <= 67) return tr("Rain");
    if (code >= 71 && code <= 77) return tr("Snow");
    if (code >= 80 && code <= 82) return tr("Rain showers");
    if (code >= 85 && code <= 86) return tr("Snow showers");
    if (code >= 95 && code <= 99) return tr("Thunderstorm");
    
    return tr("Unknown");
}
