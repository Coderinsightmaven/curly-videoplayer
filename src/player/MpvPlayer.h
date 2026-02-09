#pragma once

#include <QPointer>

#include "player/IPlayer.h"

struct mpv_handle;

class MpvPlayer final : public IPlayer {
  Q_OBJECT

 public:
  explicit MpvPlayer(QObject* parent = nullptr);
  ~MpvPlayer() override;

  QWidget* view() override;
  bool load(const QString& filePath, bool loop, bool startPaused) override;
  void setVideoFilter(const QString& filter) override;
  void play() override;
  void stop() override;
  void pause() override;

 private:
  static void wakeup(void* context);

  void processEvents();
  bool initialize();
  bool setPropertyString(const char* name, const char* value);

  QPointer<QWidget> videoWidget_;
  mpv_handle* mpv_ = nullptr;
  bool initialized_ = false;
};
