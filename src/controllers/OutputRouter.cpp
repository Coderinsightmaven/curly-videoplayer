#include "controllers/OutputRouter.h"

#include <QScreen>

#include "display/DisplayManager.h"
#include "output/OutputWindow.h"
#include "output/PreviewWindow.h"

OutputRouter::OutputRouter(DisplayManager* displayManager, QObject* parent)
    : QObject(parent), displayManager_(displayManager) {}

OutputRouter::~OutputRouter() {
  for (auto it = windows_.begin(); it != windows_.end(); ++it) {
    delete it.value();
  }
  delete previewWindow_;
}

bool OutputRouter::routeCue(const Cue& cue, TransitionStyle style, int durationMs) {
  OutputWindow* window = ensureWindow(cue.targetScreen);
  if (window == nullptr) {
    return false;
  }

  const bool ok = window->playCueWithTransition(cue, style, durationMs);
  if (!ok) {
    emit routingError(
        QString("Failed to play cue '%1' on screen %2 layer %3.").arg(cue.name).arg(cue.targetScreen).arg(cue.layer));
    return false;
  }

  emit routingStatus(QString("Program: '%1' on screen %2 layer %3").arg(cue.name).arg(cue.targetScreen).arg(cue.layer));
  return true;
}

bool OutputRouter::previewCue(const Cue& cue) {
  PreviewWindow* window = ensurePreviewWindow();
  if (window == nullptr) {
    emit routingError("Preview window is unavailable.");
    return false;
  }

  window->show();
  window->raise();

  const bool ok = window->previewCue(cue);
  if (!ok) {
    emit routingError(QString("Failed to preview cue '%1'.").arg(cue.name));
    return false;
  }

  previewCue_ = cue;
  emit routingStatus(QString("Preview: '%1'").arg(cue.name));
  return true;
}

bool OutputRouter::preloadCue(const Cue& cue) {
  OutputWindow* window = ensureWindow(cue.targetScreen);
  if (window == nullptr) {
    return false;
  }

  const bool ok = window->preloadCue(cue);
  if (!ok) {
    emit routingError(QString("Failed to preload cue '%1'.").arg(cue.name));
    return false;
  }

  emit routingStatus(QString("Preloaded: '%1'").arg(cue.name));
  return true;
}

bool OutputRouter::takePreview(TransitionStyle style, int durationMs) {
  if (previewCue_.id.isEmpty()) {
    emit routingError("Nothing is in preview to take live.");
    return false;
  }

  return routeCue(previewCue_, style, durationMs);
}

Cue OutputRouter::lastPreviewCue() const { return previewCue_; }

void OutputRouter::stopLayer(int screenIndex, int layer) {
  if (!windows_.contains(screenIndex)) {
    return;
  }
  windows_.value(screenIndex)->stopLayer(layer);
}

void OutputRouter::stopAll() {
  for (auto it = windows_.begin(); it != windows_.end(); ++it) {
    it.value()->stopAll();
  }

  if (previewWindow_ != nullptr) {
    previewWindow_->stopAll();
  }
}

void OutputRouter::showOutputs() {
  if (displayManager_ == nullptr) {
    return;
  }

  const QVector<DisplayTarget> targets = displayManager_->displays();
  for (const DisplayTarget& target : targets) {
    ensureWindow(target.index);
  }
}

void OutputRouter::hideOutputs() {
  for (auto it = windows_.begin(); it != windows_.end(); ++it) {
    it.value()->hide();
  }
}

void OutputRouter::showPreview() {
  PreviewWindow* preview = ensurePreviewWindow();
  if (preview == nullptr) {
    return;
  }
  preview->show();
  preview->raise();
}

void OutputRouter::hidePreview() {
  if (previewWindow_ != nullptr) {
    previewWindow_->hide();
  }
}

void OutputRouter::setOutputCalibration(int screenIndex, const OutputCalibration& calibration) {
  calibrations_.insert(screenIndex, calibration);
  if (windows_.contains(screenIndex)) {
    windows_.value(screenIndex)->setCalibration(calibration);
  }
}

void OutputRouter::clearCalibrations() {
  calibrations_.clear();
  for (auto it = windows_.begin(); it != windows_.end(); ++it) {
    it.value()->setCalibration(OutputCalibration{});
  }
}

OutputCalibration OutputRouter::calibrationForScreen(int screenIndex) const {
  return calibrations_.value(screenIndex, OutputCalibration{});
}

QMap<int, OutputCalibration> OutputRouter::calibrations() const { return calibrations_; }

void OutputRouter::setFallbackSlatePath(const QString& path) {
  fallbackSlatePath_ = path;

  for (auto it = windows_.begin(); it != windows_.end(); ++it) {
    it.value()->setFallbackSlatePath(path);
  }
}

OutputWindow* OutputRouter::ensureWindow(int screenIndex) {
  if (windows_.contains(screenIndex)) {
    OutputWindow* existing = windows_.value(screenIndex);
    QScreen* screen = displayManager_ != nullptr ? displayManager_->screenAt(screenIndex) : nullptr;
    existing->showOnScreen(screen);
    return existing;
  }

  if (displayManager_ == nullptr) {
    emit routingError("Display manager is unavailable.");
    return nullptr;
  }

  QScreen* screen = displayManager_->screenAt(screenIndex);
  if (screen == nullptr) {
    emit routingError(QString("No screen available for index %1.").arg(screenIndex));
    return nullptr;
  }

  auto* window = new OutputWindow();
  if (!fallbackSlatePath_.isEmpty()) {
    window->setFallbackSlatePath(fallbackSlatePath_);
  }

  if (calibrations_.contains(screenIndex)) {
    window->setCalibration(calibrations_.value(screenIndex));
  }

  connect(window, &OutputWindow::playbackError, this, &OutputRouter::routingError);

  window->showOnScreen(screen);
  windows_.insert(screenIndex, window);
  return window;
}

PreviewWindow* OutputRouter::ensurePreviewWindow() {
  if (previewWindow_ != nullptr) {
    return previewWindow_;
  }

  previewWindow_ = new PreviewWindow();
  connect(previewWindow_, &PreviewWindow::previewError, this, &OutputRouter::routingError);
  return previewWindow_;
}
