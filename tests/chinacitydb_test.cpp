// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "chinacitydb.h"

#include <QSet>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

class ChinaCityDbTest : public QObject
{
  Q_OBJECT

private slots:
  void districtLevelYongzhouEntriesAreSearchable_data ();
  void districtLevelYongzhouEntriesAreSearchable ();
  void duplicateDistrictNamesRemainSearchable ();
};

void
ChinaCityDbTest::districtLevelYongzhouEntriesAreSearchable_data ()
{
  QTest::addColumn<QString> ("query");
  QTest::addColumn<QString> ("expectedName");
  QTest::addColumn<QString> ("expectedDisplayName");

  QTest::newRow ("lingling")
      << QStringLiteral ("零陵") << QStringLiteral ("零陵区")
      << QStringLiteral ("零陵区, 永州市, 湖南省");
  QTest::newRow ("lengshuitan")
      << QStringLiteral ("冷水滩") << QStringLiteral ("冷水滩区")
      << QStringLiteral ("冷水滩区, 永州市, 湖南省");
}

void
ChinaCityDbTest::districtLevelYongzhouEntriesAreSearchable ()
{
  QFETCH (QString, query);
  QFETCH (QString, expectedName);
  QFETCH (QString, expectedDisplayName);

  const QVariantList results = ChinaCityDb::search (query, 8);

  for (const QVariant &resultValue : results)
    {
      const QVariantMap result = resultValue.toMap ();
      if (result.value (QStringLiteral ("shortName")).toString ()
          != expectedName)
        {
          continue;
        }

      QCOMPARE (result.value (QStringLiteral ("displayName")).toString (),
                expectedDisplayName);
      QVERIFY (result.value (QStringLiteral ("latitude")).toDouble () != 0.0);
      QVERIFY (result.value (QStringLiteral ("longitude")).toDouble () != 0.0);
      return;
    }

  QFAIL (
      qPrintable (QStringLiteral ("Missing expected result for query '%1': %2")
                      .arg (query, expectedName)));
}

void
ChinaCityDbTest::duplicateDistrictNamesRemainSearchable ()
{
  const QVariantList results
      = ChinaCityDb::search (QStringLiteral ("长安区"), 8);

  QSet<QString> matches;
  for (const QVariant &resultValue : results)
    {
      const QVariantMap result = resultValue.toMap ();
      if (result.value (QStringLiteral ("shortName")).toString ()
          == QStringLiteral ("长安区"))
        {
          matches.insert (
              result.value (QStringLiteral ("displayName")).toString ());
        }
    }

  QVERIFY (matches.contains (QStringLiteral ("长安区, 西安市, 陕西省")));
  QVERIFY (matches.contains (QStringLiteral ("长安区, 石家庄市, 河北省")));
}

QTEST_MAIN (ChinaCityDbTest)

#include "chinacitydb_test.moc"
