#include "player/MpvPlayer.h"

#include <cstdint>

#include <QFileInfo>
#include <QMetaObject>
#include <QWidget>

extern "C" {
#include <mpv/client.h>
}

namespace {

class VideoHostWidget final : public QWidget {
 public:
  explicit VideoHostWidget(QWidget* parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_NativeWindow);
    setAutoFillBackground(false);
    setStyleSheet("background: transparent;");
  }
};

}  // namespace

MpvPlayer::MpvPlayer(QObject* parent) : IPlayer(parent), videoWidget_(new VideoHostWidget) {
  initialize();
}

MpvPlayer::~MpvPlayer() {
  if (mpv_ != nullptr) {
    mpv_set_wakeup_callback(mpv_, nullptr, nullptr);
    mpv_terminate_destroy(mpv_);
    mpv_ = nullptr;
  }

  delete videoWidget_;
}

QWidget* MpvPlayer::view() { return videoWidget_; }

bool MpvPlayer::load(const QString& filePath, bool loop, bool startPaused) {
  if (!initialized_ && !initialize()) {
    return false;
  }

  if (filePath.isEmpty()) {
    emit playbackError("Cannot load an empty file path.");
    return false;
  }

  const QString absolute = QFileInfo(filePath).absoluteFilePath();
  const QByteArray encodedPath = absolute.toUtf8();

  if (!setPropertyString("loop-file", loop ? "inf" : "no")) {
    return false;
  }

  const char* command[] = {"loadfile", encodedPath.constData(), "replace", nullptr};
  const int status = mpv_command(mpv_, command);
  if (status < 0) {
    emit playbackError(QString("libmpv failed to load '%1': %2").arg(absolute, mpv_error_string(status)));
    return false;
  }

  if (startPaused) {
    pause();
  }

  return true;
}

void MpvPlayer::setVideoFilter(const QString& filter) {
  if (mpv_ == nullptr) {
    return;
  }

  const QByteArray encoded = filter.toUtf8();
  const int status = mpv_set_property_string(mpv_, "vf", encoded.isEmpty() ? "" : encoded.constData());
  if (status < 0) {
    emit playbackError(QString("libmpv set video filter failed: %1").arg(mpv_error_string(status)));
  }
}

void MpvPlayer::play() {
  if (mpv_ == nullptr) {
    return;
  }
  setPropertyString("pause", "no");
}

void MpvPlayer::stop() {
  if (mpv_ == nullptr) {
    return;
  }

  const char* command[] = {"stop", nullptr};
  const int status = mpv_command(mpv_, command);
  if (status < 0) {
    emit playbackError(QString("libmpv failed to stop playback: %1").arg(mpv_error_string(status)));
  }
}

void MpvPlayer::pause() {
  if (mpv_ == nullptr) {
    return;
  }
  setPropertyString("pause", "yes");
}

void MpvPlayer::wakeup(void* context) {
  auto* self = static_cast<MpvPlayer*>(context);
  if (self == nullptr) {
    return;
  }

  QMetaObject::invokeMethod(self, [self]() { self->processEvents(); }, Qt::QueuedConnection);
}

void MpvPlayer::processEvents() {
  if (mpv_ == nullptr) {
    return;
  }

  while (true) {
    mpv_event* event = mpv_wait_event(mpv_, 0.0);
    if (event == nullptr || event->event_id == MPV_EVENT_NONE) {
      break;
    }

    if (event->event_id == MPV_EVENT_SHUTDOWN) {
      break;
    }

    if (event->event_id == MPV_EVENT_END_FILE) {
      // Playback completion hook for future cue-state integration.
      continue;
    }
  }
}

bool MpvPlayer::initialize() {
  if (initialized_) {
    return true;
  }

  if (videoWidget_ == nullptr) {
    emit playbackError("Failed to initialize video host widget.");
    return false;
  }

  mpv_ = mpv_create();
  if (mpv_ == nullptr) {
    emit playbackError("libmpv could not be created.");
    return false;
  }

  int64_t wid = static_cast<int64_t>(videoWidget_->winId());
  if (mpv_set_option(mpv_, "wid", MPV_FORMAT_INT64, &wid) < 0) {
    emit playbackError("libmpv could not bind to Qt window.");
    mpv_terminate_destroy(mpv_);
    mpv_ = nullptr;
    return false;
  }

  mpv_set_option_string(mpv_, "vo", "gpu-next");
  mpv_set_option_string(mpv_, "hwdec", "auto-safe");
  mpv_set_option_string(mpv_, "alpha", "yes");
  mpv_set_option_string(mpv_, "terminal", "no");
  mpv_set_option_string(mpv_, "keep-open", "yes");

  const int status = mpv_initialize(mpv_);
  if (status < 0) {
    emit playbackError(QString("libmpv initialization failed: %1").arg(mpv_error_string(status)));
    mpv_terminate_destroy(mpv_);
    mpv_ = nullptr;
    return false;
  }

  mpv_set_wakeup_callback(mpv_, &MpvPlayer::wakeup, this);
  initialized_ = true;
  return true;
}

bool MpvPlayer::setPropertyString(const char* name, const char* value) {
  if (mpv_ == nullptr) {
    return false;
  }

  const int status = mpv_set_property_string(mpv_, name, value);
  if (status < 0) {
    emit playbackError(
        QString("libmpv set property '%1' failed: %2").arg(QString::fromUtf8(name), mpv_error_string(status)));
    return false;
  }
  return true;
}
