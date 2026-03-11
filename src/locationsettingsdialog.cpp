// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "locationsettingsdialog.h"
#include "weatherprovider.h"

#include <DLineEdit>
#include <DSpinner>

#include <QAbstractButton>
#include <QCompleter>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QRadioButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>

DWIDGET_USE_NAMESPACE

namespace
{
constexpr int kSearchDelayMs = 250;
constexpr auto kSuggestionRole = Qt::UserRole + 1;
constexpr int kDialogWidth = 520;
constexpr int kContentSpacing = 16;
constexpr int kFieldLabelWidth = 78;
constexpr int kFieldHorizontalSpacing = 14;
constexpr int kFieldVerticalSpacing = 16;
constexpr int kModeOptionSpacing = 24;
constexpr int kCityControlSpacing = 12;
constexpr int kLocateButtonWidth = 110;
constexpr int kStatusAreaHeight = 22;

bool
isValidCoordinatePair (double latitude, double longitude)
{
  return std::isfinite (latitude) && std::isfinite (longitude)
         && latitude >= -90.0 && latitude <= 90.0 && longitude >= -180.0
         && longitude <= 180.0;
}
} // namespace

LocationSettingsDialog::LocationSettingsDialog (WeatherProvider *provider,
                                                QWidget *parent)
    : DDialog (parent), m_provider (provider), m_autoRadio (nullptr),
      m_manualRadio (nullptr), m_cityEdit (nullptr), m_locateButton (nullptr),
      m_statusWidget (nullptr), m_statusSpinner (nullptr),
      m_statusLabel (nullptr), m_cancelButton (nullptr),
      m_saveButton (nullptr), m_completer (nullptr),
      m_completerModel (nullptr), m_searchTimer (new QTimer (this)),
      m_manualLatitude (0), m_manualLongitude (0),
      m_hasManualSelection (false), m_cancelButtonIndex (-1),
      m_saveButtonIndex (-1)
{
  setTitle (tr ("Weather Location Settings"));
  setMessage (tr ("Choose whether the weather applet follows your current "
                  "location or stays fixed on a city."));
  setWordWrapMessage (true);
  setOnButtonClickedClose (false);
  setContentLayoutContentsMargins (QMargins (24, 8, 24, 24));
  setFixedWidth (kDialogWidth);

  setupUi ();
  populateInitialState ();

  connect (m_provider, &WeatherProvider::citySuggestionsChanged, this,
           &LocationSettingsDialog::onCitySuggestionsChanged);
  connect (m_provider, &WeatherProvider::locationPreviewStarted, this,
           &LocationSettingsDialog::onLocationPreviewStarted);
  connect (m_provider, &WeatherProvider::locationPreviewResolved, this,
           &LocationSettingsDialog::onLocationPreviewResolved);
  connect (m_provider, &WeatherProvider::locationPreviewFailed, this,
           &LocationSettingsDialog::onLocationPreviewFailed);
  connect (this, &DDialog::buttonClicked, this,
           &LocationSettingsDialog::onDialogButtonClicked);
}

bool
LocationSettingsDialog::autoLocationEnabled () const
{
  return m_autoRadio && m_autoRadio->isChecked ();
}

QString
LocationSettingsDialog::manualCity () const
{
  return m_manualCity.trimmed ();
}

double
LocationSettingsDialog::manualLatitude () const
{
  return m_manualLatitude;
}

double
LocationSettingsDialog::manualLongitude () const
{
  return m_manualLongitude;
}

void
LocationSettingsDialog::onModeChanged ()
{
  setBusyState (false);
  setStatusMessage (QString (), false);
  updateUiForMode ();
}

void
LocationSettingsDialog::onCityTextEdited (const QString &text)
{
  if (autoLocationEnabled ())
    {
      return;
    }

  setBusyState (false);
  m_manualCity = text.trimmed ();
  m_hasManualSelection = false;
  updateAcceptState ();
  setStatusMessage (QString (), false);
  m_searchTimer->start ();
}

void
LocationSettingsDialog::onSearchTimeout ()
{
  if (autoLocationEnabled ())
    {
      return;
    }

  m_provider->searchCities (m_cityEdit->text ());
}

void
LocationSettingsDialog::onSuggestionActivated (const QModelIndex &index)
{
  applyManualSuggestion (index.data (kSuggestionRole).toMap ());
}

void
LocationSettingsDialog::onLocateClicked ()
{
  setBusyState (true);
  m_provider->requestLocationPreview ();
}

void
LocationSettingsDialog::onLocationPreviewStarted ()
{
  setBusyState (true);
  m_locateButton->setEnabled (false);
}

void
LocationSettingsDialog::onLocationPreviewResolved (const QString &city,
                                                   double latitude,
                                                   double longitude)
{
  setBusyState (false);
  m_locateButton->setEnabled (true);
  m_locateButton->setText (tr ("Locate"));

  if (autoLocationEnabled ())
    {
      m_autoPreviewCity = city.trimmed ();
      m_cityEdit->setText (m_autoPreviewCity);
      setStatusMessage (tr ("Current city refreshed."), false);
      return;
    }

  m_manualCity = city.trimmed ();
  m_manualLatitude = latitude;
  m_manualLongitude = longitude;
  m_hasManualSelection = isValidCoordinatePair (latitude, longitude)
                         && !m_manualCity.isEmpty ();
  m_cityEdit->setText (m_manualCity);
  updateAcceptState ();
  setStatusMessage (tr ("Current city filled in."), false);
}

void
LocationSettingsDialog::onLocationPreviewFailed (const QString &message)
{
  setBusyState (false);
  m_locateButton->setEnabled (true);
  m_locateButton->setText (tr ("Locate"));
  setStatusMessage (message, true);
}

void
LocationSettingsDialog::onCitySuggestionsChanged ()
{
  m_completerModel->clear ();

  const QVariantList suggestions = m_provider->citySuggestions ();
  for (const QVariant &suggestionValue : suggestions)
    {
      const QVariantMap suggestion = suggestionValue.toMap ();
      QStandardItem *item
          = new QStandardItem (suggestion.value ("displayName").toString ());
      item->setData (suggestion, kSuggestionRole);
      m_completerModel->appendRow (item);
    }

  if (!autoLocationEnabled () && !suggestions.isEmpty ()
      && m_cityEdit->lineEdit ()->hasFocus ())
    {
      m_completer->complete ();
    }
}

void
LocationSettingsDialog::setupUi ()
{
  m_searchTimer->setSingleShot (true);
  m_searchTimer->setInterval (kSearchDelayMs);

  m_autoRadio = new QRadioButton (tr ("Automatic location"), this);
  m_manualRadio = new QRadioButton (tr ("Manual city"), this);

  QWidget *modeWidget = new QWidget (this);
  QHBoxLayout *modeLayout = new QHBoxLayout (modeWidget);
  modeLayout->setContentsMargins (0, 0, 0, 0);
  modeLayout->setSpacing (kModeOptionSpacing);
  modeLayout->addWidget (m_autoRadio);
  modeLayout->addWidget (m_manualRadio);
  modeLayout->addStretch ();

  m_cityEdit = new DLineEdit (this);
  m_cityEdit->setClearButtonEnabled (true);
  m_locateButton = new QPushButton (tr ("Locate"), this);
  m_locateButton->setMinimumWidth (kLocateButtonWidth);
  m_locateButton->setAutoDefault (false);
  m_locateButton->setDefault (false);
  {
    QSizePolicy locateButtonPolicy = m_locateButton->sizePolicy ();
    locateButtonPolicy.setRetainSizeWhenHidden (true);
    m_locateButton->setSizePolicy (locateButtonPolicy);
  }

  QWidget *cityWidget = new QWidget (this);
  QHBoxLayout *cityLayout = new QHBoxLayout (cityWidget);
  cityLayout->setContentsMargins (0, 0, 0, 0);
  cityLayout->setSpacing (kCityControlSpacing);
  cityLayout->addWidget (m_cityEdit, 1);
  cityLayout->addWidget (m_locateButton);

  m_statusWidget = new QWidget (this);
  m_statusWidget->setFixedHeight (kStatusAreaHeight);
  {
    QSizePolicy statusWidgetPolicy = m_statusWidget->sizePolicy ();
    statusWidgetPolicy.setRetainSizeWhenHidden (true);
    m_statusWidget->setSizePolicy (statusWidgetPolicy);
  }
  QHBoxLayout *statusLayout = new QHBoxLayout (m_statusWidget);
  statusLayout->setContentsMargins (0, 0, 0, 0);
  statusLayout->setSpacing (10);
  statusLayout->setAlignment (Qt::AlignLeft | Qt::AlignVCenter);

  m_statusSpinner = new DSpinner (m_statusWidget);
  m_statusSpinner->setFixedSize (18, 18);
  m_statusSpinner->hide ();
  statusLayout->addWidget (m_statusSpinner, 0, Qt::AlignVCenter);

  m_statusLabel = new QLabel (m_statusWidget);
  m_statusLabel->setWordWrap (true);
  m_statusLabel->hide ();
  statusLayout->addWidget (m_statusLabel, 0, Qt::AlignVCenter);
  statusLayout->addStretch ();

  QLabel *modeLabel = new QLabel (tr ("Mode"), this);
  modeLabel->setMinimumWidth (kFieldLabelWidth);
  modeLabel->setAlignment (Qt::AlignLeft | Qt::AlignTop);

  QLabel *cityLabel = new QLabel (tr ("City"), this);
  cityLabel->setMinimumWidth (kFieldLabelWidth);
  cityLabel->setAlignment (Qt::AlignLeft | Qt::AlignTop);

  QWidget *contentWidget = new QWidget (this);
  QVBoxLayout *contentLayout = new QVBoxLayout (contentWidget);
  contentLayout->setContentsMargins (0, 4, 0, 0);
  contentLayout->setSpacing (kContentSpacing);

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->setContentsMargins (0, 0, 0, 0);
  gridLayout->setHorizontalSpacing (kFieldHorizontalSpacing);
  gridLayout->setVerticalSpacing (kFieldVerticalSpacing);
  gridLayout->setColumnStretch (1, 1);
  gridLayout->addWidget (modeLabel, 0, 0);
  gridLayout->addWidget (modeWidget, 0, 1);
  gridLayout->addWidget (cityLabel, 1, 0);
  gridLayout->addWidget (cityWidget, 1, 1);
  gridLayout->addItem (new QSpacerItem (kFieldLabelWidth, 0,
                                        QSizePolicy::Fixed,
                                        QSizePolicy::Minimum),
                       2, 0);
  gridLayout->addWidget (m_statusWidget, 2, 1);

  contentLayout->addLayout (gridLayout);
  addContent (contentWidget);

  m_cancelButtonIndex = addButton (tr ("Cancel"));
  m_saveButtonIndex = addButton (tr ("Save"), true, DDialog::ButtonRecommend);
  m_cancelButton = getButton (m_cancelButtonIndex);
  m_saveButton = getButton (m_saveButtonIndex);
  if (QPushButton *cancelButton = qobject_cast<QPushButton *> (m_cancelButton))
    {
      cancelButton->setMinimumWidth (96);
    }
  if (QPushButton *saveButton = qobject_cast<QPushButton *> (m_saveButton))
    {
      saveButton->setMinimumWidth (96);
    }

  m_completerModel = new QStandardItemModel (this);
  m_completer = new QCompleter (m_completerModel, this);
  m_completer->setCaseSensitivity (Qt::CaseInsensitive);
  m_completer->setCompletionMode (QCompleter::PopupCompletion);
  m_cityEdit->lineEdit ()->setCompleter (m_completer);

  connect (m_autoRadio, &QRadioButton::toggled, this,
           &LocationSettingsDialog::onModeChanged);
  connect (m_cityEdit, &DLineEdit::textEdited, this,
           &LocationSettingsDialog::onCityTextEdited);
  connect (m_searchTimer, &QTimer::timeout, this,
           &LocationSettingsDialog::onSearchTimeout);
  connect (m_locateButton, &QPushButton::clicked, this,
           &LocationSettingsDialog::onLocateClicked);
  connect (m_completer,
           qOverload<const QModelIndex &> (&QCompleter::activated), this,
           &LocationSettingsDialog::onSuggestionActivated);
}

void
LocationSettingsDialog::populateInitialState ()
{
  const bool providerAutoMode = m_provider->autoLocationEnabled ();
  m_autoPreviewCity = m_provider->autoLocationCity ();
  m_manualCity = m_provider->manualLocationCity ();
  m_manualLatitude = m_provider->manualLocationLatitude ();
  m_manualLongitude = m_provider->manualLocationLongitude ();
  m_hasManualSelection
      = !m_manualCity.isEmpty ()
        && isValidCoordinatePair (m_manualLatitude, m_manualLongitude);

  m_autoRadio->setChecked (providerAutoMode);
  m_manualRadio->setChecked (!providerAutoMode);
  updateUiForMode ();
}

void
LocationSettingsDialog::updateUiForMode ()
{
  if (autoLocationEnabled ())
    {
      m_searchTimer->stop ();
      m_completerModel->clear ();
      m_statusWidget->setVisible (true);
      m_cityEdit->lineEdit ()->setReadOnly (true);
      m_cityEdit->setClearButtonEnabled (false);
      m_cityEdit->setPlaceholderText (
          tr ("Use the locate button to refresh the current city"));
      m_cityEdit->setText (m_autoPreviewCity);
      m_locateButton->setVisible (true);
      m_locateButton->setToolTip (
          tr ("Refresh the automatically detected city"));
    }
  else
    {
      m_statusWidget->setVisible (false);
      m_cityEdit->lineEdit ()->setReadOnly (false);
      m_cityEdit->setClearButtonEnabled (true);
      m_cityEdit->setPlaceholderText (
          tr ("Type a city name and choose a suggestion"));
      m_cityEdit->setText (m_manualCity);
      m_locateButton->setVisible (false);
    }

  updateAcceptState ();
}

void
LocationSettingsDialog::updateAcceptState ()
{
  if (!m_saveButton)
    {
      return;
    }

  m_saveButton->setEnabled (autoLocationEnabled ()
                            || hasValidManualSelection ());
}

void
LocationSettingsDialog::applyManualSuggestion (const QVariantMap &suggestion)
{
  if (suggestion.isEmpty ())
    {
      return;
    }

  m_manualCity = suggestion.value ("shortName").toString ().trimmed ();
  m_manualLatitude = suggestion.value ("latitude").toDouble ();
  m_manualLongitude = suggestion.value ("longitude").toDouble ();
  m_hasManualSelection
      = !m_manualCity.isEmpty ()
        && isValidCoordinatePair (m_manualLatitude, m_manualLongitude);
  m_cityEdit->setText (m_manualCity);
  updateAcceptState ();
  setStatusMessage (QString (), false);
}

void
LocationSettingsDialog::setStatusMessage (const QString &message, bool error)
{
  if (message.isEmpty ())
    {
      m_statusLabel->clear ();
      m_statusLabel->hide ();
      return;
    }

  QPalette statusPalette = m_statusLabel->palette ();
  if (error)
    {
      statusPalette.setColor (QPalette::WindowText,
                              QColor (QStringLiteral ("#c42b1c")));
    }
  else
    {
      statusPalette.setColor (
          QPalette::WindowText,
          this->palette ().color (QPalette::PlaceholderText));
    }

  m_statusLabel->setPalette (statusPalette);
  m_statusLabel->setText (message);
  m_statusLabel->show ();
}

bool
LocationSettingsDialog::hasValidManualSelection () const
{
  return m_hasManualSelection
         && isValidCoordinatePair (m_manualLatitude, m_manualLongitude)
         && !m_manualCity.trimmed ().isEmpty ();
}

void
LocationSettingsDialog::setBusyState (bool busy)
{
  if (!m_statusWidget || !m_statusSpinner)
    {
      return;
    }

  if (busy)
    {
      QPalette statusPalette = m_statusLabel->palette ();
      statusPalette.setColor (
          QPalette::WindowText,
          this->palette ().color (QPalette::PlaceholderText));
      m_statusLabel->setPalette (statusPalette);
      m_statusLabel->setText (tr ("Locating..."));
      m_statusLabel->show ();
      m_statusSpinner->show ();
      m_statusSpinner->start ();
      m_statusWidget->show ();
      return;
    }

  m_statusSpinner->stop ();
  m_statusSpinner->hide ();
}

void
LocationSettingsDialog::onDialogButtonClicked (int index, const QString &text)
{
  Q_UNUSED (text);

  if (index == m_cancelButtonIndex)
    {
      reject ();
      return;
    }

  if (index == m_saveButtonIndex
      && (autoLocationEnabled () || hasValidManualSelection ()))
    {
      accept ();
    }
}
