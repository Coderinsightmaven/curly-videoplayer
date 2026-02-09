#include "output/LayerSurface.h"

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>

#include "player/IPlayer.h"
#include "player/MpvPlayer.h"

namespace {

QString perspectiveFilterFromCalibration(const QSize& size, const OutputCalibration& calibration) {
  if (size.width() <= 0 || size.height() <= 0) {
    return {};
  }

  if (calibration.keystoneHorizontal == 0 && calibration.keystoneVertical == 0) {
    return {};
  }

  const double width = static_cast<double>(size.width());
  const double height = static_cast<double>(size.height());

  const double hNorm = static_cast<double>(calibration.keystoneHorizontal) / 100.0;
  const double vNorm = static_cast<double>(calibration.keystoneVertical) / 100.0;

  const double topInset = hNorm > 0.0 ? width * hNorm * 0.45 : 0.0;
  const double bottomInset = hNorm < 0.0 ? width * -hNorm * 0.45 : 0.0;
  const double leftInset = vNorm > 0.0 ? height * vNorm * 0.45 : 0.0;
  const double rightInset = vNorm < 0.0 ? height * -vNorm * 0.45 : 0.0;

  const double x0 = topInset;
  const double y0 = leftInset;
  const double x1 = width - topInset;
  const double y1 = rightInset;
  const double x2 = bottomInset;
  const double y2 = height - leftInset;
  const double x3 = width - bottomInset;
  const double y3 = height - rightInset;

  return QString(
             "lavfi=[perspective=x0=%1:y0=%2:x1=%3:y1=%4:x2=%5:y2=%6:x3=%7:y3=%8]")
      .arg(x0, 0, 'f', 2)
      .arg(y0, 0, 'f', 2)
      .arg(x1, 0, 'f', 2)
      .arg(y1, 0, 'f', 2)
      .arg(x2, 0, 'f', 2)
      .arg(y2, 0, 'f', 2)
      .arg(x3, 0, 'f', 2)
      .arg(y3, 0, 'f', 2);
}

}  // namespace

LayerSurface::LayerSurface(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_NoSystemBackground, false);
  setStyleSheet("background: black;");
}

bool LayerSurface::playCue(const Cue& cue) {
  IPlayer* player = ensurePlayerForLayer(cue.layer);
  if (player == nullptr) {
    return false;
  }

  applyFilterToPlayer(player);

  if (!player->load(cue.filePath, cue.loop, false)) {
    emit playbackError(QString("Failed to load cue '%1'.").arg(cue.name));
    return false;
  }

  player->play();
  return true;
}

bool LayerSurface::preloadCue(const Cue& cue) {
  IPlayer* player = ensurePlayerForLayer(cue.layer);
  if (player == nullptr) {
    return false;
  }

  applyFilterToPlayer(player);

  if (!player->load(cue.filePath, cue.loop, true)) {
    emit playbackError(QString("Failed to preload cue '%1'.").arg(cue.name));
    return false;
  }

  return true;
}

void LayerSurface::stopLayer(int layer) {
  if (!layers_.contains(layer)) {
    return;
  }
  layers_.value(layer)->stop();
}

void LayerSurface::stopAll() {
  for (auto it = layers_.begin(); it != layers_.end(); ++it) {
    it.value()->stop();
  }
}

void LayerSurface::setCalibration(const OutputCalibration& calibration) {
  calibration_ = calibration;

  for (auto it = layers_.begin(); it != layers_.end(); ++it) {
    applyFilterToPlayer(it.value());
  }
}

OutputCalibration LayerSurface::calibration() const { return calibration_; }

void LayerSurface::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);

  for (auto it = layers_.begin(); it != layers_.end(); ++it) {
    QWidget* view = it.value()->view();
    if (view != nullptr) {
      view->setGeometry(rect());
    }
    applyFilterToPlayer(it.value());
  }
}

void LayerSurface::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.fillRect(event->rect(), Qt::black);
}

IPlayer* LayerSurface::ensurePlayerForLayer(int layer) {
  if (layers_.contains(layer)) {
    IPlayer* existing = layers_.value(layer);
    QWidget* view = existing->view();
    if (view != nullptr) {
      view->raise();
    }
    return existing;
  }

  auto* player = new MpvPlayer(this);
  QWidget* view = player->view();
  if (view == nullptr) {
    player->deleteLater();
    return nullptr;
  }

  view->setParent(this);
  view->setGeometry(rect());
  view->show();
  view->raise();

  connect(player, &IPlayer::playbackError, this, [this, layer](const QString& message) {
    qWarning() << "Layer" << layer << "error:" << message;
    emit playbackError(QString("Layer %1: %2").arg(layer).arg(message));
  });

  layers_.insert(layer, player);
  applyFilterToPlayer(player);
  return player;
}

QString LayerSurface::buildKeystoneFilter() const { return perspectiveFilterFromCalibration(size(), calibration_); }

void LayerSurface::applyFilterToPlayer(IPlayer* player) {
  if (player == nullptr) {
    return;
  }
  player->setVideoFilter(buildKeystoneFilter());
}
