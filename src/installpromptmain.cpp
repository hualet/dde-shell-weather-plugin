// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "installprompt.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include <DApplication>

DWIDGET_USE_NAMESPACE

namespace
{
constexpr auto kPackageId = "org.deepin.ds.weather";
constexpr auto kTranslationDir
    = "/usr/share/dde-shell/org.deepin.ds.weather/translations";
}

int
main (int argc, char *argv[])
{
  DApplication app (argc, argv);
  app.setApplicationName (QStringLiteral ("ds-weather-install-prompt"));

  QTranslator qtTranslator;
  if (qtTranslator.load (QLocale (), QStringLiteral ("qtbase"),
                         QStringLiteral ("_"),
                         QLibraryInfo::path (QLibraryInfo::TranslationsPath)))
    {
      app.installTranslator (&qtTranslator);
    }

  QTranslator appTranslator;
  if (appTranslator.load (QLocale (), QString::fromLatin1 (kPackageId),
                          QStringLiteral ("_"),
                          QString::fromLatin1 (kTranslationDir)))
    {
      app.installTranslator (&appTranslator);
    }

  QCommandLineParser parser;
  parser.addHelpOption ();
  parser.addOption ({ { QStringLiteral ("mode") },
                      QStringLiteral ("Prompt mode: install or upgrade."),
                      QStringLiteral ("mode") });
  parser.process (app);

  const std::optional<InstallPrompt::Mode> mode
      = InstallPrompt::parseMode (parser.value (QStringLiteral ("mode")));
  if (!mode.has_value ())
    {
      return 2;
    }

  return InstallPrompt::run (*mode);
}
