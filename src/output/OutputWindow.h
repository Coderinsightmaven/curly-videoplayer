#pragma once

#include <QWidget>

#include "core/Cue.h"
#include "core/Transition.h"
#include "output/OutputCalibration.h"

class EdgeBlendOverlay;
class LayerSurface;
class QLabel;
class QGraphicsOpacityEffect;
class QScreen;

class OutputWindow : public QWidget {
  Q_OBJECT

 public:
  explicit OutputWindow(QWidget* parent = nullptr);

  void showOnScreen(QScreen* screen);
  bool playCue(const Cue& cue);
  bool playCueWithTransition(const Cue& cue, TransitionStyle style, int durationMs);
  bool preloadCue(const Cue& cue);
  void stopLayer(int layer);
  void stopAll();

  void setCalibration(const OutputCalibration& calibration);
  OutputCalibration calibration() const;

  void setFallbackSlatePath(const QString& path);

 signals:
  void playbackError(const QString& message);

 protected:
  void resizeEvent(QResizeEvent* event) override;

 private:
  void showSlate(const QString& message = QString());
  void hideSlate();
  void runFade(double from, double to, int durationMs);

  LayerSurface* surface_;
  EdgeBlendOverlay* edgeBlendOverlay_;
  QWidget* fadeOverlay_;
  QGraphicsOpacityEffect* fadeEffect_;
  QLabel* slateLabel_;
  OutputCalibration calibration_;
  QString fallbackSlatePath_;
};
