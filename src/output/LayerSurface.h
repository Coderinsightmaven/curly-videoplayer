#pragma once

#include <QMap>
#include <QWidget>

#include "core/Cue.h"
#include "output/OutputCalibration.h"

class IPlayer;

class LayerSurface : public QWidget {
  Q_OBJECT

 public:
  explicit LayerSurface(QWidget* parent = nullptr);

  bool playCue(const Cue& cue);
  bool preloadCue(const Cue& cue);
  void stopLayer(int layer);
  void stopAll();
  void setCalibration(const OutputCalibration& calibration);
  OutputCalibration calibration() const;

 signals:
  void playbackError(const QString& message);

 protected:
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

 private:
  IPlayer* ensurePlayerForLayer(int layer);
  QString buildKeystoneFilter() const;
  void applyFilterToPlayer(IPlayer* player);

  QMap<int, IPlayer*> layers_;
  OutputCalibration calibration_;
};
