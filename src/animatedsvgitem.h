// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickPaintedItem>
#include <QRectF>
#include <QSvgRenderer>

class QTimer;

class AnimatedSvgItem : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY (QString source READ source WRITE setSource NOTIFY sourceChanged)
  Q_PROPERTY (
      bool animated READ animated WRITE setAnimated NOTIFY animatedChanged)

public:
  explicit AnimatedSvgItem (QQuickItem *parent = nullptr);
  ~AnimatedSvgItem () override;

  void paint (QPainter *painter) override;

  QString source () const;
  void setSource (const QString &source);

  bool animated () const;
  void setAnimated (bool animated);

signals:
  void sourceChanged ();
  void animatedChanged ();

private:
  QRectF calculateVisibleBounds () const;
  void playAnimationOnce ();
  void stopAnimation ();

  QString m_source;
  QSvgRenderer *m_renderer;
  QTimer *m_stopTimer;
  QRectF m_visibleBounds;
  bool m_animated = true;
};
