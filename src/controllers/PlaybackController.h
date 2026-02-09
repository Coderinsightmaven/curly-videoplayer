#pragma once

#include <QMap>
#include <QObject>
#include <QString>

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

 private:
  CueListModel* cueModel_;
  OutputRouter* outputRouter_;
  QMap<QString, QString> lastTimecodeByCueId_;
};
