#include "controllers/PlaybackController.h"

#include <QRegularExpression>
#include <QTimer>
#include <QtGlobal>

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
  if (cue.filePath.isEmpty() && (!cue.isLiveInput || cue.liveInputUrl.trimmed().isEmpty())) {
    emit playbackError(QString("Cue '%1' has no media file.").arg(cue.name));
    return false;
  }

  const TransitionStyle effectiveStyle = cue.useTransitionOverride ? cue.transitionStyle : style;
  const int effectiveDuration = cue.useTransitionOverride ? cue.transitionDurationMs : durationMs;

  const bool ok = outputRouter_->routeCue(cue, effectiveStyle, effectiveDuration);
  if (!ok) {
    emit playbackError(QString("Failed to route cue '%1'.").arg(cue.name));
    return false;
  }

  emit playbackStatus(QString("Live: '%1'").arg(cue.name));
  emit cueWentLive(cue);
  scheduleFollowCue(cue, effectiveStyle, effectiveDuration);
  scheduleAutoStop(cue);
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
  if (cue.filePath.isEmpty() && (!cue.isLiveInput || cue.liveInputUrl.trimmed().isEmpty())) {
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
  if (cue.filePath.isEmpty() && (!cue.isLiveInput || cue.liveInputUrl.trimmed().isEmpty())) {
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

  const Cue previewCue = outputRouter_->lastPreviewCue();
  const TransitionStyle effectiveStyle = previewCue.useTransitionOverride ? previewCue.transitionStyle : style;
  const int effectiveDuration = previewCue.useTransitionOverride ? previewCue.transitionDurationMs : durationMs;

  const bool ok = outputRouter_->takePreview(effectiveStyle, effectiveDuration);
  if (!ok) {
    emit playbackError("Failed to take preview live.");
    return false;
  }

  emit playbackStatus(QString("Taken live: '%1'").arg(previewCue.name));
  emit cueWentLive(previewCue);
  scheduleFollowCue(previewCue, effectiveStyle, effectiveDuration);
  scheduleAutoStop(previewCue);
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

  const QString normalizedTimecode = normalizeTimecode(timecode);
  if (normalizedTimecode.isEmpty()) {
    return false;
  }

  bool triggered = false;

  const QVector<Cue> cues = cueModel_->cues();
  for (const Cue& cue : cues) {
    if (cue.timecodeTrigger.isEmpty()) {
      continue;
    }

    if (cueMatchesTimecode(cue.timecodeTrigger, normalizedTimecode)) {
      const QString last = lastTimecodeByCueId_.value(cue.id);
      if (last == normalizedTimecode) {
        continue;
      }

      const TransitionStyle effectiveStyle = cue.useTransitionOverride ? cue.transitionStyle : style;
      const int effectiveDuration = cue.useTransitionOverride ? cue.transitionDurationMs : durationMs;
      if (outputRouter_->routeCue(cue, effectiveStyle, effectiveDuration)) {
        lastTimecodeByCueId_.insert(cue.id, normalizedTimecode);
        emit playbackStatus(QString("Timecode %1 -> '%2'").arg(normalizedTimecode).arg(cue.name));
        emit cueWentLive(cue);
        scheduleFollowCue(cue, effectiveStyle, effectiveDuration);
        scheduleAutoStop(cue);
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
  outputRouter_->stopCue(cue);
}

void PlaybackController::stopAll() {
  if (outputRouter_ == nullptr) {
    return;
  }
  outputRouter_->stopAll();
}

QString PlaybackController::normalizeTimecode(const QString& rawTimecode) {
  const QString trimmed = rawTimecode.trimmed();
  if (trimmed.isEmpty()) {
    return {};
  }

  QRegularExpression fullPattern(R"(^(\d{1,2}):(\d{1,2}):(\d{1,2})(?::(\d{1,2}))?$)");
  QRegularExpressionMatch match = fullPattern.match(trimmed);
  if (!match.hasMatch()) {
    return {};
  }

  const int hour = match.captured(1).toInt();
  const int minute = match.captured(2).toInt();
  const int second = match.captured(3).toInt();
  const int frame = match.captured(4).isEmpty() ? 0 : match.captured(4).toInt();

  return QString("%1:%2:%3:%4")
      .arg(hour, 2, 10, QChar('0'))
      .arg(minute, 2, 10, QChar('0'))
      .arg(second, 2, 10, QChar('0'))
      .arg(frame, 2, 10, QChar('0'));
}

bool PlaybackController::cueMatchesTimecode(const QString& cueTrigger, const QString& normalizedTimecode) {
  const QString normalizedTrigger = cueTrigger.trimmed().toLower();
  if (normalizedTrigger.isEmpty()) {
    return false;
  }

  if (!normalizedTrigger.contains('*')) {
    return normalizeTimecode(normalizedTrigger) == normalizedTimecode;
  }

  const QStringList triggerParts = normalizedTrigger.split(':');
  const QStringList incomingParts = normalizedTimecode.toLower().split(':');
  if (triggerParts.size() != 4 || incomingParts.size() != 4) {
    return false;
  }

  for (int i = 0; i < 4; ++i) {
    if (triggerParts.at(i) == "*") {
      continue;
    }

    bool ok = false;
    const int expectedValue = triggerParts.at(i).toInt(&ok);
    if (!ok) {
      return false;
    }

    const QString expected = QString("%1").arg(expectedValue, 2, 10, QChar('0'));
    if (expected != incomingParts.at(i)) {
      return false;
    }
  }

  return true;
}

void PlaybackController::scheduleFollowCue(const Cue& cue, TransitionStyle style, int durationMs) {
  if (cueModel_ == nullptr) {
    return;
  }

  if (cue.autoFollow) {
    const int followRow = cue.followCueRow >= 0 ? cue.followCueRow : cueModel_->rowForCueId(cue.id) + 1;
    if (!cueModel_->isValidRow(followRow)) {
      return;
    }

    const int delayMs = qMax(0, cue.followDelayMs);
    QTimer::singleShot(delayMs, this, [this, followRow, style, durationMs]() { playCueAtRow(followRow, style, durationMs); });
    return;
  }

  schedulePlaylistAdvance(cue, style, durationMs);
}

void PlaybackController::schedulePlaylistAdvance(const Cue& cue, TransitionStyle style, int durationMs) {
  if (cueModel_ == nullptr || !cue.playlistAutoAdvance || cue.playlistId.trimmed().isEmpty()) {
    return;
  }

  const QString playlistId = cue.playlistId.trimmed();
  const QVector<Cue> cues = cueModel_->cues();

  const int currentRow = cueModel_->rowForCueId(cue.id);
  if (currentRow < 0) {
    return;
  }

  int nextRow = -1;
  for (int i = currentRow + 1; i < cues.size(); ++i) {
    if (cues.at(i).playlistId.trimmed() == playlistId) {
      nextRow = i;
      break;
    }
  }

  if (nextRow < 0 && cue.playlistLoop) {
    for (int i = 0; i < cues.size(); ++i) {
      if (i == currentRow) {
        continue;
      }
      if (cues.at(i).playlistId.trimmed() == playlistId) {
        nextRow = i;
        break;
      }
    }
  }

  if (!cueModel_->isValidRow(nextRow)) {
    return;
  }

  const int delayMs = qMax(0, cue.playlistAdvanceDelayMs);
  QTimer::singleShot(delayMs, this, [this, nextRow, style, durationMs]() { playCueAtRow(nextRow, style, durationMs); });
}

void PlaybackController::scheduleAutoStop(const Cue& cue) {
  if (outputRouter_ == nullptr || cue.autoStopMs <= 0) {
    return;
  }

  const int delayMs = qMax(0, cue.autoStopMs);
  QTimer::singleShot(delayMs, this, [this, cue]() { outputRouter_->stopCue(cue); });
}
