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

void EdgeBlendOverlay::setMask(bool enabled, int leftPx, int topPx, int rightPx, int bottomPx) {
  const int clampedLeft = qMax(0, leftPx);
  const int clampedTop = qMax(0, topPx);
  const int clampedRight = qMax(0, rightPx);
  const int clampedBottom = qMax(0, bottomPx);

  if (maskEnabled_ == enabled && maskLeftPx_ == clampedLeft && maskTopPx_ == clampedTop && maskRightPx_ == clampedRight &&
      maskBottomPx_ == clampedBottom) {
    return;
  }

  maskEnabled_ = enabled;
  maskLeftPx_ = clampedLeft;
  maskTopPx_ = clampedTop;
  maskRightPx_ = clampedRight;
  maskBottomPx_ = clampedBottom;
  update();
}

void EdgeBlendOverlay::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setClipRect(event->rect());

  const QRect r = rect();
  const int s = qMin(blendSize_, qMin(r.width(), r.height()) / 2);

  if (s > 0) {
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

  if (!maskEnabled_) {
    return;
  }

  const int left = qBound(0, maskLeftPx_, r.width());
  const int right = qBound(0, maskRightPx_, r.width());
  const int top = qBound(0, maskTopPx_, r.height());
  const int bottom = qBound(0, maskBottomPx_, r.height());

  if (left > 0) {
    painter.fillRect(QRect(0, 0, left, r.height()), Qt::black);
  }
  if (right > 0) {
    painter.fillRect(QRect(r.width() - right, 0, right, r.height()), Qt::black);
  }
  if (top > 0) {
    painter.fillRect(QRect(0, 0, r.width(), top), Qt::black);
  }
  if (bottom > 0) {
    painter.fillRect(QRect(0, r.height() - bottom, r.width(), bottom), Qt::black);
  }
}
