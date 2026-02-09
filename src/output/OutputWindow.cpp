#include "output/OutputWindow.h"

#include <QEventLoop>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#include "output/EdgeBlendOverlay.h"
#include "output/LayerSurface.h"

OutputWindow::OutputWindow(QWidget* parent)
    : QWidget(parent),
      surface_(new LayerSurface(this)),
      edgeBlendOverlay_(new EdgeBlendOverlay(this)),
      fadeOverlay_(new QWidget(this)),
      fadeEffect_(new QGraphicsOpacityEffect(this)),
      slateLabel_(new QLabel(this)) {
  setWindowFlag(Qt::FramelessWindowHint, true);
  setWindowFlag(Qt::WindowStaysOnTopHint, true);
  setWindowFlag(Qt::Tool, true);
  setAttribute(Qt::WA_DeleteOnClose, false);
  setStyleSheet("background: black;");

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(surface_);

  fadeOverlay_->setStyleSheet("background: black;");
  fadeOverlay_->setGraphicsEffect(fadeEffect_);
  fadeEffect_->setOpacity(0.0);
  fadeOverlay_->hide();

  edgeBlendOverlay_->setAttribute(Qt::WA_TransparentForMouseEvents);
  edgeBlendOverlay_->show();

  slateLabel_->setAlignment(Qt::AlignCenter);
  slateLabel_->setStyleSheet(
      "color: #f7f7f7; background: rgba(0, 0, 0, 190); font-size: 22px; font-weight: 600; padding: 24px;");
  slateLabel_->setText("SLATE\nNo media loaded");
  slateLabel_->show();

  connect(surface_, &LayerSurface::playbackError, this, [this](const QString& message) {
    showSlate(message);
    emit playbackError(message);
  });
}

void OutputWindow::showOnScreen(QScreen* screen) {
  if (screen != nullptr) {
    winId();
    if (windowHandle() != nullptr) {
      windowHandle()->setScreen(screen);
    }
    setGeometry(screen->geometry());
  }

  showFullScreen();
  raise();
}

bool OutputWindow::playCue(const Cue& cue) {
  const bool ok = surface_->playCue(cue);
  if (ok) {
    hideSlate();
  } else {
    showSlate(QString("Failed to play '%1'").arg(cue.name));
  }
  return ok;
}

bool OutputWindow::playCueWithTransition(const Cue& cue, TransitionStyle style, int durationMs) {
  if (style == TransitionStyle::Cut) {
    return playCue(cue);
  }

  const int fadeDuration = qMax(60, durationMs);
  runFade(0.0, 1.0, fadeDuration / 2);

  const bool ok = playCue(cue);

  if (style == TransitionStyle::DipToBlack) {
    QEventLoop holdLoop;
    QTimer::singleShot(100, &holdLoop, &QEventLoop::quit);
    holdLoop.exec();
  }

  runFade(1.0, 0.0, fadeDuration / 2);
  return ok;
}

bool OutputWindow::preloadCue(const Cue& cue) { return surface_->preloadCue(cue); }

void OutputWindow::stopLayer(int layer) { surface_->stopLayer(layer); }

void OutputWindow::stopAll() {
  surface_->stopAll();
  showSlate("SLATE\nPlayback stopped");
}

void OutputWindow::setCalibration(const OutputCalibration& calibration) {
  calibration_ = calibration;
  edgeBlendOverlay_->setBlendSize(calibration.edgeBlendPx);
  surface_->setCalibration(calibration);
}

OutputCalibration OutputWindow::calibration() const { return calibration_; }

void OutputWindow::setFallbackSlatePath(const QString& path) {
  fallbackSlatePath_ = path;
  if (slateLabel_->isVisible()) {
    showSlate();
  }
}

void OutputWindow::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  fadeOverlay_->setGeometry(rect());
  edgeBlendOverlay_->setGeometry(rect());
  slateLabel_->setGeometry(rect());
}

void OutputWindow::showSlate(const QString& message) {
  if (!fallbackSlatePath_.isEmpty()) {
    QPixmap pixmap(fallbackSlatePath_);
    if (!pixmap.isNull()) {
      slateLabel_->setPixmap(pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
      slateLabel_->setText({});
      slateLabel_->show();
      return;
    }
  }

  slateLabel_->setPixmap(QPixmap());
  if (message.isEmpty()) {
    slateLabel_->setText("SLATE\nNo media loaded");
  } else {
    slateLabel_->setText(QString("SLATE\n%1").arg(message));
  }
  slateLabel_->show();
}

void OutputWindow::hideSlate() { slateLabel_->hide(); }

void OutputWindow::runFade(double from, double to, int durationMs) {
  fadeOverlay_->show();
  fadeOverlay_->raise();

  auto* animation = new QPropertyAnimation(fadeEffect_, "opacity", this);
  animation->setStartValue(from);
  animation->setEndValue(to);
  animation->setDuration(durationMs);

  QEventLoop loop;
  connect(animation, &QPropertyAnimation::finished, &loop, &QEventLoop::quit);
  animation->start(QAbstractAnimation::DeleteWhenStopped);
  loop.exec();

  if (to <= 0.01) {
    fadeOverlay_->hide();
  }
}
