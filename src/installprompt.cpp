// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "installprompt.h"

#include <QCoreApplication>
#include <QDialog>
#include <QProcess>

#include <DDialog>

DWIDGET_USE_NAMESPACE

std::optional<InstallPrompt::Mode>
InstallPrompt::parseMode (const QString &value)
{
  if (value == QLatin1String ("install"))
    {
      return Mode::Install;
    }

  if (value == QLatin1String ("upgrade"))
    {
      return Mode::Upgrade;
    }

  return std::nullopt;
}

InstallPrompt::DialogText
InstallPrompt::dialogTextForMode (Mode mode)
{
  DialogText text;

  text.title
      = QCoreApplication::translate ("InstallPrompt", "Weather Plugin Notice");
  text.acceptLabel
      = QCoreApplication::translate ("InstallPrompt", "Restart Now");
  text.rejectLabel = QCoreApplication::translate ("InstallPrompt", "Later");

  if (mode == Mode::Install)
    {
      text.body = QCoreApplication::translate (
          "InstallPrompt",
          "The weather plugin has been installed. To show weather in the "
          "taskbar and keep it updating, the taskbar needs to be reloaded. "
          "If you restart it now, the weather plugin will appear and begin "
          "updating.");
    }
  else
    {
      text.body = QCoreApplication::translate (
          "InstallPrompt",
          "The weather plugin has been updated. To keep it displayed and "
          "updating normally, the taskbar needs to be reloaded. If you "
          "restart it now, the taskbar will recover shortly.");
    }

  return text;
}

int
InstallPrompt::run (Mode mode, QWidget *parent)
{
  const DialogText text = dialogTextForMode (mode);
  bool restartRequested = false;

  DDialog dialog (parent);
  dialog.setTitle (text.title);
  dialog.setMessage (text.body);
  dialog.setWordWrapMessage (true);
  dialog.setOnButtonClickedClose (false);

  const int rejectButtonIndex
      = dialog.addButton (text.rejectLabel, false, DDialog::ButtonNormal);
  const int acceptButtonIndex
      = dialog.addButton (text.acceptLabel, true, DDialog::ButtonRecommend);

  QObject::connect (&dialog, &DDialog::buttonClicked, &dialog,
                    [&dialog, &restartRequested, rejectButtonIndex,
                     acceptButtonIndex] (int index, const QString &) {
                      if (index == acceptButtonIndex)
                        {
                          restartRequested = true;
                          dialog.accept ();
                          return;
                        }

                      if (index == rejectButtonIndex)
                        {
                          dialog.reject ();
                        }
                    });

  const int result = dialog.exec ();
  if (restartRequested && result == QDialog::Accepted)
    {
      restartShell ();
    }

  return result;
}

bool
InstallPrompt::restartShell ()
{
  return QProcess::startDetached (
      QStringLiteral ("systemctl"),
      { QStringLiteral ("--user"), QStringLiteral ("restart"),
        QStringLiteral ("dde-shell@DDE.service") });
}
