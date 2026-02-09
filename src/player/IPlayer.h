#pragma once

#include <QObject>
#include <QString>

class QWidget;

class IPlayer : public QObject {
  Q_OBJECT

 public:
  explicit IPlayer(QObject* parent = nullptr) : QObject(parent) {}
  ~IPlayer() override = default;

  virtual QWidget* view() = 0;
  virtual bool load(const QString& filePath, bool loop, bool startPaused) = 0;
  virtual void setVideoFilter(const QString& filter) = 0;
  virtual void play() = 0;
  virtual void stop() = 0;
  virtual void pause() = 0;

 signals:
  void playbackError(const QString& message);
};
