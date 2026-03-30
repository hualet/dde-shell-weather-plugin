// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "animatedsvgitem.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QSvgRenderer>
#include <QTest>
#include <QUrl>

namespace
{
QString
sourceFilePath (const QString &relativePath)
{
  return QStringLiteral (DS_WEATHER_SOURCE_DIR) + QLatin1Char ('/')
         + relativePath;
}

QString
sourceFileUrl (const QString &relativePath)
{
  return QUrl::fromLocalFile (sourceFilePath (relativePath)).toString ();
}
} // namespace

class AnimatedSvgItemTest : public QObject
{
  Q_OBJECT

private slots:
  void weatherIconExposesAnimatedProperty ();
  void staticModeDisablesSvgAnimation ();
  void animatedSvgItemStopsAfterOneCycle ();
};

void
AnimatedSvgItemTest::weatherIconExposesAnimatedProperty ()
{
  qmlRegisterType<AnimatedSvgItem> ("org.deepin.ds.weather", 1, 0,
                                    "AnimatedSvgItem");

  QQmlEngine engine;
  QQmlComponent component (&engine, QUrl::fromLocalFile (sourceFilePath (
                                        "package/WeatherIcon.qml")));

  QScopedPointer<QObject> weatherIcon (component.create ());
  QVERIFY2 (weatherIcon != nullptr, qPrintable (component.errorString ()));

  const int propertyIndex
      = weatherIcon->metaObject ()->indexOfProperty ("animated");
  QVERIFY2 (propertyIndex >= 0,
            "WeatherIcon must expose an animated property for QML callers");
  QVERIFY (weatherIcon->setProperty ("animated", false));

  auto *svgItem = weatherIcon->findChild<AnimatedSvgItem *> ();
  QVERIFY (svgItem != nullptr);
  QCOMPARE (svgItem->animated (), false);
}

void
AnimatedSvgItemTest::staticModeDisablesSvgAnimation ()
{
  AnimatedSvgItem item;
  item.setAnimated (false);
  item.setSource (sourceFileUrl ("package/icons/tropical-storm.svg"));

  auto *renderer = item.findChild<QSvgRenderer *> ();
  QVERIFY (renderer != nullptr);
  QVERIFY (renderer->animated ());
  QVERIFY (!renderer->isAnimationEnabled ());
}

void
AnimatedSvgItemTest::animatedSvgItemStopsAfterOneCycle ()
{
  AnimatedSvgItem item;
  item.setSource (sourceFileUrl ("package/icons/tropical-storm.svg"));

  auto *renderer = item.findChild<QSvgRenderer *> ();
  QVERIFY (renderer != nullptr);
  QVERIFY (renderer->animated ());
  QVERIFY (renderer->isAnimationEnabled ());

  const int rawDuration = renderer->animationDuration ();
  int durationMs = 3000;
  if (rawDuration > 0)
    durationMs = (rawDuration > 100) ? rawDuration : rawDuration * 1000;

  QTRY_VERIFY_WITH_TIMEOUT (!renderer->isAnimationEnabled (),
                            durationMs + 1500);
}

QTEST_MAIN (AnimatedSvgItemTest)

#include "animatedsvgitem_test.moc"
