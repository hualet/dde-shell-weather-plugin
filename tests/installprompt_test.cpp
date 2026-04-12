// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "installprompt.h"

class InstallPromptTest : public QObject
{
  Q_OBJECT

private slots:
  void parsesInstallMode ();
  void parsesUpgradeMode ();
  void rejectsUnknownMode ();
  void returnsInstallStrings ();
  void returnsUpgradeStrings ();
};

void
InstallPromptTest::parsesInstallMode ()
{
  QCOMPARE (InstallPrompt::parseMode ("install"),
            InstallPrompt::Mode::Install);
}

void
InstallPromptTest::parsesUpgradeMode ()
{
  QCOMPARE (InstallPrompt::parseMode ("upgrade"),
            InstallPrompt::Mode::Upgrade);
}

void
InstallPromptTest::rejectsUnknownMode ()
{
  QVERIFY (!InstallPrompt::parseMode ("unexpected").has_value ());
}

void
InstallPromptTest::returnsInstallStrings ()
{
  const InstallPrompt::DialogText text
      = InstallPrompt::dialogTextForMode (InstallPrompt::Mode::Install);

  QVERIFY (!text.title.isEmpty ());
  QVERIFY (!text.body.isEmpty ());
  QVERIFY (!text.acceptLabel.isEmpty ());
  QVERIFY (!text.rejectLabel.isEmpty ());
}

void
InstallPromptTest::returnsUpgradeStrings ()
{
  const InstallPrompt::DialogText text
      = InstallPrompt::dialogTextForMode (InstallPrompt::Mode::Upgrade);

  QVERIFY (!text.title.isEmpty ());
  QVERIFY (!text.body.isEmpty ());
  QVERIFY (!text.acceptLabel.isEmpty ());
  QVERIFY (!text.rejectLabel.isEmpty ());
}

QTEST_MAIN (InstallPromptTest)

#include "installprompt_test.moc"
