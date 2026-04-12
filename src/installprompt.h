// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include <QString>

class QWidget;

class InstallPrompt
{
public:
  enum class Mode
  {
    Install,
    Upgrade,
  };

  struct DialogText
  {
    QString title;
    QString body;
    QString acceptLabel;
    QString rejectLabel;
  };

  static std::optional<Mode> parseMode (const QString &value);
  static DialogText dialogTextForMode (Mode mode);
  static int run (Mode mode, QWidget *parent = nullptr);

private:
  static bool restartShell ();
};
