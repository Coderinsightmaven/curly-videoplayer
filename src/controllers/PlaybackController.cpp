#include "controllers/PlaybackController.h"

#include "controllers/OutputRouter.h"
#include "core/CueListModel.h"

PlaybackController::PlaybackController(CueListModel* cueModel, OutputRouter* outputRouter, QObject* parent)
    : QObject(parent), cueModel_(cueModel), outputRouter_(outputRouter) {}

bool PlaybackController::playCueAtRow(int row, TransitionStyle style, int durationMs) {
  if (cueModel_ == nullptr || outputRouter_ == nullptr) {
    emit playbackError("Playback system is not initialized.");
    return false;
  }

  if (!cueModel_->isValidRow(row)) {
    emit playbackError("Select a valid cue before playing.");
    return false;
  }

  const Cue cue = cueModel_->cueAt(row);
  if (cue.filePath.isEmpty()) {
    emit playbackError(QString("Cue '%1' has no media file.").arg(cue.name));
    return false;
  }

  const bool ok = outputRouter_->routeCue(cue, style, durationMs);
  if (!ok) {
    emit playbackError(QString("Failed to route cue '%1'.").arg(cue.name));
    return false;
  }

  emit playbackStatus(QString("Live: '%1'").arg(cue.name));
  return true;
}

bool PlaybackController::previewCueAtRow(int row) {
  if (cueModel_ == nullptr || outputRouter_ == nullptr) {
    emit playbackError("Playback system is not initialized.");
    return false;
  }

  if (!cueModel_->isValidRow(row)) {
    emit playbackError("Select a valid cue before previewing.");
    return false;
  }

  const Cue cue = cueModel_->cueAt(row);
  if (cue.filePath.isEmpty()) {
    emit playbackError(QString("Cue '%1' has no media file.").arg(cue.name));
    return false;
  }

  const bool ok = outputRouter_->previewCue(cue);
  if (!ok) {
    emit playbackError(QString("Failed to preview cue '%1'.").arg(cue.name));
    return false;
  }

  emit playbackStatus(QString("Preview: '%1'").arg(cue.name));
  return true;
}

bool PlaybackController::preloadCueAtRow(int row) {
  if (cueModel_ == nullptr || outputRouter_ == nullptr) {
    emit playbackError("Playback system is not initialized.");
    return false;
  }

  if (!cueModel_->isValidRow(row)) {
    emit playbackError("Select a valid cue before preloading.");
    return false;
  }

  const Cue cue = cueModel_->cueAt(row);
  if (cue.filePath.isEmpty()) {
    emit playbackError(QString("Cue '%1' has no media file.").arg(cue.name));
    return false;
  }

  const bool ok = outputRouter_->preloadCue(cue);
  if (!ok) {
    emit playbackError(QString("Failed to preload cue '%1'.").arg(cue.name));
    return false;
  }

  emit playbackStatus(QString("Preloaded: '%1'").arg(cue.name));
  return true;
}

bool PlaybackController::takePreviewCue(TransitionStyle style, int durationMs) {
  if (outputRouter_ == nullptr) {
    emit playbackError("Playback system is not initialized.");
    return false;
  }

  const bool ok = outputRouter_->takePreview(style, durationMs);
  if (!ok) {
    emit playbackError("Failed to take preview live.");
    return false;
  }

  const Cue previewCue = outputRouter_->lastPreviewCue();
  emit playbackStatus(QString("Taken live: '%1'").arg(previewCue.name));
  return true;
}

bool PlaybackController::playCueById(const QString& cueId, TransitionStyle style, int durationMs, bool previewOnly) {
  if (cueModel_ == nullptr) {
    return false;
  }

  const int row = cueModel_->rowForCueId(cueId);
  if (row < 0) {
    emit playbackError(QString("Hotkey cue '%1' no longer exists.").arg(cueId));
    return false;
  }

  return previewOnly ? previewCueAtRow(row) : playCueAtRow(row, style, durationMs);
}

bool PlaybackController::triggerByTimecode(const QString& timecode, TransitionStyle style, int durationMs) {
  if (cueModel_ == nullptr || outputRouter_ == nullptr || timecode.isEmpty()) {
    return false;
  }

  bool triggered = false;

  const QVector<Cue> cues = cueModel_->cues();
  for (const Cue& cue : cues) {
    if (cue.timecodeTrigger.isEmpty()) {
      continue;
    }

    if (cue.timecodeTrigger.compare(timecode, Qt::CaseInsensitive) == 0) {
      const QString last = lastTimecodeByCueId_.value(cue.id);
      if (last == timecode) {
        continue;
      }

      if (outputRouter_->routeCue(cue, style, durationMs)) {
        lastTimecodeByCueId_.insert(cue.id, timecode);
        emit playbackStatus(QString("Timecode %1 -> '%2'").arg(timecode).arg(cue.name));
        triggered = true;
      }
    } else {
      lastTimecodeByCueId_.remove(cue.id);
    }
  }

  return triggered;
}

void PlaybackController::stopCueAtRow(int row) {
  if (cueModel_ == nullptr || outputRouter_ == nullptr || !cueModel_->isValidRow(row)) {
    return;
  }

  const Cue cue = cueModel_->cueAt(row);
  outputRouter_->stopLayer(cue.targetScreen, cue.layer);
}

void PlaybackController::stopAll() {
  if (outputRouter_ == nullptr) {
    return;
  }
  outputRouter_->stopAll();
}
