// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "animatedsvgitem.h"

#include <QImage>
#include <QPainter>
#include <QTimer>
#include <QUrl>

AnimatedSvgItem::AnimatedSvgItem (QQuickItem *parent)
    : QQuickPaintedItem (parent), m_renderer (new QSvgRenderer (this)),
      m_stopTimer (new QTimer (this))
{
  setRenderTarget (QQuickPaintedItem::FramebufferObject);
  m_stopTimer->setSingleShot (true);

  connect (m_renderer, &QSvgRenderer::repaintNeeded, this,
           [this] () { update (); });
  connect (m_stopTimer, &QTimer::timeout, this,
           &AnimatedSvgItem::stopAnimation);
}

AnimatedSvgItem::~AnimatedSvgItem () { stopAnimation (); }

void
AnimatedSvgItem::paint (QPainter *painter)
{
  if (!m_renderer->isValid ())
    return;

  painter->setRenderHint (QPainter::Antialiasing);
  painter->setRenderHint (QPainter::SmoothPixmapTransform);

  QSizeF itemSize (width (), height ());
  QSizeF defaultSize = m_renderer->defaultSize ();
  if (defaultSize.isEmpty () || itemSize.isEmpty ())
    return;

  QRectF visibleBounds = m_visibleBounds;
  if (!visibleBounds.isValid () || visibleBounds.isEmpty ())
    visibleBounds = QRectF (QPointF (0, 0), defaultSize);

  double scale = qMin (itemSize.width () / visibleBounds.width (),
                       itemSize.height () / visibleBounds.height ());
  double dx = (itemSize.width () - visibleBounds.width () * scale) / 2.0
              - visibleBounds.x () * scale;
  double dy = (itemSize.height () - visibleBounds.height () * scale) / 2.0
              - visibleBounds.y () * scale;

  QRectF bounds (dx, dy, defaultSize.width () * scale,
                 defaultSize.height () * scale);
  m_renderer->render (painter, bounds);
}

QString
AnimatedSvgItem::source () const
{
  return m_source;
}

void
AnimatedSvgItem::setSource (const QString &source)
{
  if (m_source == source)
    return;
  m_source = source;

  QString path = source;
  if (path.startsWith ("qrc:"))
    path = path.mid (3);
  else if (path.startsWith ("file://"))
    path = QUrl (source).toLocalFile ();

  stopAnimation ();

  if (m_renderer->load (path))
    {
      m_visibleBounds = calculateVisibleBounds ();
      if (m_animated && m_renderer->animated ())
        {
          playAnimationOnce ();
        }
      update ();
    }
  else
    {
      m_visibleBounds = QRectF ();
      qWarning () << "Failed to load SVG:" << source;
    }

  emit sourceChanged ();
}

bool
AnimatedSvgItem::animated () const
{
  return m_animated;
}

void
AnimatedSvgItem::setAnimated (bool animated)
{
  if (m_animated == animated)
    return;
  m_animated = animated;

  stopAnimation ();

  if (m_animated && m_renderer->isValid () && m_renderer->animated ())
    playAnimationOnce ();
  else
    update ();

  emit animatedChanged ();
}

QRectF
AnimatedSvgItem::calculateVisibleBounds () const
{
  const QSize defaultSize = m_renderer->defaultSize ();
  if (defaultSize.isEmpty ())
    return QRectF ();

  QImage image (defaultSize, QImage::Format_ARGB32_Premultiplied);
  image.fill (Qt::transparent);

  {
    QPainter painter (&image);
    painter.setRenderHint (QPainter::Antialiasing);
    painter.setRenderHint (QPainter::SmoothPixmapTransform);
    m_renderer->render (&painter,
                        QRectF (QPointF (0, 0), QSizeF (defaultSize)));
  }

  int left = defaultSize.width ();
  int top = defaultSize.height ();
  int right = -1;
  int bottom = -1;

  constexpr int alphaThreshold = 8;
  for (int y = 0; y < image.height (); ++y)
    {
      const QRgb *line
          = reinterpret_cast<const QRgb *> (image.constScanLine (y));
      for (int x = 0; x < image.width (); ++x)
        {
          if (qAlpha (line[x]) < alphaThreshold)
            continue;

          left = qMin (left, x);
          top = qMin (top, y);
          right = qMax (right, x);
          bottom = qMax (bottom, y);
        }
    }

  if (right < left || bottom < top)
    return QRectF (QPointF (0, 0), QSizeF (defaultSize));

  QRectF bounds (left, top, right - left + 1, bottom - top + 1);
  bounds.adjust (-1.0, -1.0, 1.0, 1.0);
  return bounds.intersected (QRectF (QPointF (0, 0), QSizeF (defaultSize)));
}

void
AnimatedSvgItem::playAnimationOnce ()
{
  if (!m_renderer->isValid () || !m_renderer->animated ())
    return;

  m_renderer->setFramesPerSecond (30);
  m_renderer->setAnimationEnabled (true);
  m_stopTimer->stop ();

  // Qt 6.8 on Deepin reports milliseconds here for these SMIL assets,
  // even though the public header still documents seconds. Normalize the
  // value defensively so the icon always stops after one visible cycle.
  const int rawDuration = m_renderer->animationDuration ();
  int durationMs = 3000;
  if (rawDuration > 0)
    durationMs = (rawDuration > 100) ? rawDuration : rawDuration * 1000;

  m_stopTimer->start (qBound (500, durationMs, 15000));
}

void
AnimatedSvgItem::stopAnimation ()
{
  m_stopTimer->stop ();
  m_renderer->setAnimationEnabled (false);
  update ();
}
