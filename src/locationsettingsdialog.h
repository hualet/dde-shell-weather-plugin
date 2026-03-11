// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <DDialog>

#include <QModelIndex>

class QAbstractButton;
class QCompleter;
class QLabel;
class QPushButton;
class QRadioButton;
class QStandardItemModel;
class QTimer;
class WeatherProvider;

namespace Dtk
{
namespace Widget
{
class DLineEdit;
class DSpinner;
}
}

class LocationSettingsDialog : public Dtk::Widget::DDialog
{
  Q_OBJECT

public:
  explicit LocationSettingsDialog (WeatherProvider *provider,
                                   QWidget *parent = nullptr);

  bool autoLocationEnabled () const;
  QString manualCity () const;
  double manualLatitude () const;
  double manualLongitude () const;

private slots:
  void onModeChanged ();
  void onCityTextEdited (const QString &text);
  void onSearchTimeout ();
  void onSuggestionActivated (const QModelIndex &index);
  void onLocateClicked ();
  void onLocationPreviewStarted ();
  void onLocationPreviewResolved (const QString &city, double latitude,
                                  double longitude);
  void onLocationPreviewFailed (const QString &message);
  void onCitySuggestionsChanged ();
  void onDialogButtonClicked (int index, const QString &text);

private:
  void setBusyState (bool busy);
  void setupUi ();
  void populateInitialState ();
  void updateUiForMode ();
  void updateAcceptState ();
  void applyManualSuggestion (const QVariantMap &suggestion);
  void setStatusMessage (const QString &message, bool error);
  bool hasValidManualSelection () const;

private:
  WeatherProvider *m_provider;
  QRadioButton *m_autoRadio;
  QRadioButton *m_manualRadio;
  Dtk::Widget::DLineEdit *m_cityEdit;
  QPushButton *m_locateButton;
  QWidget *m_statusWidget;
  Dtk::Widget::DSpinner *m_statusSpinner;
  QLabel *m_statusLabel;
  QAbstractButton *m_cancelButton;
  QAbstractButton *m_saveButton;
  QCompleter *m_completer;
  QStandardItemModel *m_completerModel;
  QTimer *m_searchTimer;
  QString m_autoPreviewCity;
  QString m_manualCity;
  double m_manualLatitude;
  double m_manualLongitude;
  bool m_hasManualSelection;
  int m_cancelButtonIndex;
  int m_saveButtonIndex;
};
