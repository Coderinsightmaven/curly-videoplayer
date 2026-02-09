#include "output/EdgeBlendOverlay.h"

#include <QLinearGradient>
#include <QPaintEvent>
#include <QPainter>
#include <QtGlobal>

EdgeBlendOverlay::EdgeBlendOverlay(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_NoSystemBackground);
  setStyleSheet("background: transparent;");
}

void EdgeBlendOverlay::setBlendSize(int pixels) {
  const int clamped = qBound(0, pixels, 600);
  if (blendSize_ == clamped) {
    return;
  }
  blendSize_ = clamped;
  update();
}

int EdgeBlendOverlay::blendSize() const { return blendSize_; }

void EdgeBlendOverlay::paintEvent(QPaintEvent* event) {
  if (blendSize_ <= 0) {
    return;
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setClipRect(event->rect());

  const QRect r = rect();
  const int s = qMin(blendSize_, qMin(r.width(), r.height()) / 2);

  QLinearGradient topGradient(0, 0, 0, s);
  topGradient.setColorAt(0.0, QColor(0, 0, 0, 255));
  topGradient.setColorAt(1.0, QColor(0, 0, 0, 0));
  painter.fillRect(QRect(0, 0, r.width(), s), topGradient);

  QLinearGradient bottomGradient(0, r.height(), 0, r.height() - s);
  bottomGradient.setColorAt(0.0, QColor(0, 0, 0, 255));
  bottomGradient.setColorAt(1.0, QColor(0, 0, 0, 0));
  painter.fillRect(QRect(0, r.height() - s, r.width(), s), bottomGradient);

  QLinearGradient leftGradient(0, 0, s, 0);
  leftGradient.setColorAt(0.0, QColor(0, 0, 0, 255));
  leftGradient.setColorAt(1.0, QColor(0, 0, 0, 0));
  painter.fillRect(QRect(0, 0, s, r.height()), leftGradient);

  QLinearGradient rightGradient(r.width(), 0, r.width() - s, 0);
  rightGradient.setColorAt(0.0, QColor(0, 0, 0, 255));
  rightGradient.setColorAt(1.0, QColor(0, 0, 0, 0));
  painter.fillRect(QRect(r.width() - s, 0, s, r.height()), rightGradient);
}
