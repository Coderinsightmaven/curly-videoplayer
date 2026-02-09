#pragma once

#include <QMap>
#include <QObject>
#include <QString>

#include "core/Cue.h"
#include "core/Transition.h"

class CueListModel;
class OutputRouter;

class PlaybackController : public QObject {
  Q_OBJECT

 public:
  explicit PlaybackController(CueListModel* cueModel, OutputRouter* outputRouter, QObject* parent = nullptr);

  bool playCueAtRow(int row, TransitionStyle style, int durationMs);
  bool previewCueAtRow(int row);
  bool preloadCueAtRow(int row);
  bool takePreviewCue(TransitionStyle style, int durationMs);
  bool playCueById(const QString& cueId, TransitionStyle style, int durationMs, bool previewOnly);
  bool triggerByTimecode(const QString& timecode, TransitionStyle style, int durationMs);

  void stopCueAtRow(int row);
  void stopAll();

 signals:
  void playbackError(const QString& message);
  void playbackStatus(const QString& message);
  void cueWentLive(const Cue& cue);

 private:
  static QString normalizeTimecode(const QString& rawTimecode);
  static bool cueMatchesTimecode(const QString& cueTrigger, const QString& normalizedTimecode);
  void scheduleFollowCue(const Cue& cue, TransitionStyle style, int durationMs);
  void schedulePlaylistAdvance(const Cue& cue, TransitionStyle style, int durationMs);
  void scheduleAutoStop(const Cue& cue);

  CueListModel* cueModel_;
  OutputRouter* outputRouter_;
  QMap<QString, QString> lastTimecodeByCueId_;
};
