// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#ifdef QT_POSITIONING_LIB
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#endif

struct WeatherData {
    QString city;
    QString weatherCode;
    double temperature;
    double temperatureMax;
    double temperatureMin;
    double humidity;
    double windSpeed;
    QString weatherDescription;
    bool isValid;
    
    WeatherData() : temperature(0), temperatureMax(0), temperatureMin(0), humidity(0), windSpeed(0), isValid(false) {}
};

class WeatherProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString city READ city NOTIFY weatherChanged)
    Q_PROPERTY(QString weatherCode READ weatherCode NOTIFY weatherChanged)
    Q_PROPERTY(double temperature READ temperature NOTIFY weatherChanged)
    Q_PROPERTY(double humidity READ humidity NOTIFY weatherChanged)
    Q_PROPERTY(double windSpeed READ windSpeed NOTIFY weatherChanged)
    Q_PROPERTY(QString weatherDescription READ weatherDescription NOTIFY weatherChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

    Q_PROPERTY(double temperatureMax READ temperatureMax NOTIFY weatherChanged)
    Q_PROPERTY(double temperatureMin READ temperatureMin NOTIFY weatherChanged)

public:
    explicit WeatherProvider(QObject *parent = nullptr);
    ~WeatherProvider();

    // Property getters
    QString city() const { return m_weatherData.city; }
    QString weatherCode() const { return m_weatherData.weatherCode; }
    double temperature() const { return m_weatherData.temperature; }
    double humidity() const { return m_weatherData.humidity; }
    double windSpeed() const { return m_weatherData.windSpeed; }
    QString weatherDescription() const { return m_weatherData.weatherDescription; }

    double temperatureMax() const { return m_weatherData.temperatureMax; }
    double temperatureMin() const { return m_weatherData.temperatureMin; }
    bool isLoading() const { return m_isLoading; }
    bool hasError() const { return m_hasError; }
    QString errorMessage() const { return m_errorMessage; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setLocation(double latitude, double longitude);

signals:
    void weatherChanged();
    void loadingChanged();
    void errorChanged();

private slots:
    void onWeatherReplyFinished(QNetworkReply *reply);
    void onCityReplyFinished(QNetworkReply *reply);

#ifdef QT_POSITIONING_LIB
    void onPositionUpdated(const QGeoPositionInfo &info);
    void onPositionError(QGeoPositionInfoSource::Error error);
#endif

private:
    void initLocationSource();
    void fetchWeather(double latitude, double longitude);
    void fetchCityName(double latitude, double longitude);
    QString parseWeatherCode(int code);

private:
    QNetworkAccessManager *m_networkManager;
    QTimer *m_refreshTimer;
    
    WeatherData m_weatherData;
    double m_latitude;
    double m_longitude;
    
    bool m_isLoading;
    bool m_hasError;
    QString m_errorMessage;
    
#ifdef QT_POSITIONING_LIB
    QGeoPositionInfoSource *m_positionSource;
#else
    void *m_positionSource; // Placeholder when positioning not available
#endif
};
