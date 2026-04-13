// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "installprompt.h"

class InstallPromptTest : public QObject
{
  Q_OBJECT

private slots:
  void init ();
  void cleanup ();
  void parsesInstallMode ();
  void parsesUpgradeMode ();
  void rejectsUnknownMode ();
  void returnsInstallStrings ();
  void returnsUpgradeStrings ();
  void detectsDarkThemePreferenceFromEnvironment ();
  void detectsLightThemePreferenceFromEnvironment ();
  void prefersExplicitThemeTypeEnvironment ();
  void ignoresUnknownThemePreference ();
};

void
InstallPromptTest::init ()
{
  qunsetenv ("D_THEME_TYPE");
  qunsetenv ("D_DXCB_THEME");
}

void
InstallPromptTest::cleanup ()
{
  qunsetenv ("D_THEME_TYPE");
  qunsetenv ("D_DXCB_THEME");
}

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

void
InstallPromptTest::detectsDarkThemePreferenceFromEnvironment ()
{
  qputenv ("D_DXCB_THEME", "dark");

  QCOMPARE (InstallPrompt::themePreferenceFromEnvironment (),
            InstallPrompt::ThemePreference::Dark);
}

void
InstallPromptTest::detectsLightThemePreferenceFromEnvironment ()
{
  qputenv ("D_DXCB_THEME", "light");

  QCOMPARE (InstallPrompt::themePreferenceFromEnvironment (),
            InstallPrompt::ThemePreference::Light);
}

void
InstallPromptTest::prefersExplicitThemeTypeEnvironment ()
{
  qputenv ("D_DXCB_THEME", "light");
  qputenv ("D_THEME_TYPE", "2");

  QCOMPARE (InstallPrompt::themePreferenceFromEnvironment (),
            InstallPrompt::ThemePreference::Dark);
}

void
InstallPromptTest::ignoresUnknownThemePreference ()
{
  qputenv ("D_THEME_TYPE", "follow-system");
  qputenv ("D_DXCB_THEME", "auto");

  QVERIFY (!InstallPrompt::themePreferenceFromEnvironment ().has_value ());
}

QTEST_MAIN (InstallPromptTest)

#include "installprompt_test.moc"
