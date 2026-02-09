#pragma once

#include <QMap>
#include <QObject>
#include <QString>

#include "core/Cue.h"
#include "core/Transition.h"
#include "output/OutputCalibration.h"

class DisplayManager;
class OutputWindow;
class PreviewWindow;

class OutputRouter : public QObject {
  Q_OBJECT

 public:
  explicit OutputRouter(DisplayManager* displayManager, QObject* parent = nullptr);
  ~OutputRouter() override;

  bool routeCue(const Cue& cue, TransitionStyle style = TransitionStyle::Cut, int durationMs = 0);
  bool previewCue(const Cue& cue);
  bool preloadCue(const Cue& cue);
  bool takePreview(TransitionStyle style, int durationMs);
  Cue lastPreviewCue() const;
  void stopCue(const Cue& cue);

  void stopLayer(int screenIndex, int layer);
  void stopAll();
  void showOutputs();
  void hideOutputs();
  void showPreview();
  void hidePreview();
  void setOverlayText(const QString& text);

  void setOutputCalibration(int screenIndex, const OutputCalibration& calibration);
  void clearCalibrations();
  OutputCalibration calibrationForScreen(int screenIndex) const;
  QMap<int, OutputCalibration> calibrations() const;

  void setFallbackSlatePath(const QString& path);
  void setFilterPresets(const QMap<QString, QString>& presets);

 signals:
  void routingError(const QString& message);
  void routingStatus(const QString& message);

 private:
  Cue applyFilterPreset(const Cue& cue);
  OutputWindow* ensureWindow(int screenIndex);
  PreviewWindow* ensurePreviewWindow();

  DisplayManager* displayManager_;
  QMap<int, OutputWindow*> windows_;
  PreviewWindow* previewWindow_ = nullptr;
  Cue previewCue_;
  QMap<int, OutputCalibration> calibrations_;
  QMap<QString, QString> filterPresets_;
  QString fallbackSlatePath_;
  QString overlayText_;
};
