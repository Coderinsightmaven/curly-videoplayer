#include "controllers/OutputRouter.h"

#include <QSet>
#include <QScreen>

#include "display/DisplayManager.h"
#include "output/OutputWindow.h"
#include "output/PreviewWindow.h"

namespace {

QVector<int> resolveTargetScreens(const Cue& cue, DisplayManager* displayManager) {
  if (displayManager == nullptr) {
    return {cue.targetScreen};
  }

  const QVector<DisplayTarget> displays = displayManager->displays();
  if (cue.targetSetId == "all-screens") {
    QVector<int> all;
    all.reserve(displays.size());
    for (const DisplayTarget& display : displays) {
      all.push_back(display.index);
    }
    if (!all.isEmpty()) {
      return all;
    }
  }

  if (cue.targetSetId.startsWith("screen-")) {
    bool ok = false;
    const int index = cue.targetSetId.mid(QString("screen-").size()).toInt(&ok);
    if (ok) {
      return {index};
    }
  }

  return {cue.targetScreen};
}

}  // namespace

OutputRouter::OutputRouter(DisplayManager* displayManager, QObject* parent)
    : QObject(parent), displayManager_(displayManager) {}

OutputRouter::~OutputRouter() {
  for (auto it = windows_.begin(); it != windows_.end(); ++it) {
    delete it.value();
  }
  delete previewWindow_;
}

bool OutputRouter::routeCue(const Cue& cue, TransitionStyle style, int durationMs) {
  const QVector<int> targetScreens = resolveTargetScreens(cue, displayManager_);
  if (targetScreens.isEmpty()) {
    emit routingError(QString("No screens available for cue '%1'.").arg(cue.name));
    return false;
  }

  bool routedAny = false;
  QSet<int> attempted;

  for (int screenIndex : targetScreens) {
    if (attempted.contains(screenIndex)) {
      continue;
    }
    attempted.insert(screenIndex);

    OutputWindow* window = ensureWindow(screenIndex);
    if (window == nullptr) {
      continue;
    }

    Cue routedCue = cue;
    routedCue.targetScreen = screenIndex;
    const bool ok = window->playCueWithTransition(routedCue, style, durationMs);
    if (!ok) {
      emit routingError(
          QString("Failed to play cue '%1' on screen %2 layer %3.").arg(cue.name).arg(screenIndex).arg(cue.layer));
      continue;
    }
    routedAny = true;
  }

  if (!routedAny) {
    return false;
  }

  emit routingStatus(QString("Program: '%1' on %2 target(s) layer %3").arg(cue.name).arg(targetScreens.size()).arg(cue.layer));
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
  const QVector<int> targetScreens = resolveTargetScreens(cue, displayManager_);
  if (targetScreens.isEmpty()) {
    emit routingError(QString("No screens available for preload cue '%1'.").arg(cue.name));
    return false;
  }

  bool preloadedAny = false;
  QSet<int> attempted;
  for (int screenIndex : targetScreens) {
    if (attempted.contains(screenIndex)) {
      continue;
    }
    attempted.insert(screenIndex);

    OutputWindow* window = ensureWindow(screenIndex);
    if (window == nullptr) {
      continue;
    }

    Cue routedCue = cue;
    routedCue.targetScreen = screenIndex;
    const bool ok = window->preloadCue(routedCue);
    if (!ok) {
      emit routingError(QString("Failed to preload cue '%1' on screen %2.").arg(cue.name).arg(screenIndex));
      continue;
    }
    preloadedAny = true;
  }

  if (!preloadedAny) {
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

void OutputRouter::stopCue(const Cue& cue) {
  const QVector<int> targetScreens = resolveTargetScreens(cue, displayManager_);
  QSet<int> attempted;
  for (int screenIndex : targetScreens) {
    if (attempted.contains(screenIndex)) {
      continue;
    }
    attempted.insert(screenIndex);
    stopLayer(screenIndex, cue.layer);
  }
}

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

void OutputRouter::setOverlayText(const QString& text) {
  overlayText_ = text;
  for (auto it = windows_.begin(); it != windows_.end(); ++it) {
    it.value()->setOverlayText(text);
  }
  if (previewWindow_ != nullptr) {
    previewWindow_->setOverlayText(text);
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
  if (!overlayText_.isEmpty()) {
    window->setOverlayText(overlayText_);
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
  previewWindow_->setOverlayText(overlayText_);
  connect(previewWindow_, &PreviewWindow::previewError, this, &OutputRouter::routingError);
  return previewWindow_;
}
