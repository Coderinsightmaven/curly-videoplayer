#pragma once

#include <QWidget>

#include "core/Cue.h"

class LayerSurface;

class PreviewWindow : public QWidget {
  Q_OBJECT

 public:
  explicit PreviewWindow(QWidget* parent = nullptr);

  bool previewCue(const Cue& cue);
  bool preloadCue(const Cue& cue);
  void stopAll();
  Cue lastCue() const;

 signals:
  void previewError(const QString& message);

 private:
  LayerSurface* surface_;
  Cue lastCue_;
};
