#include "control/MidiInputService.h"

#include <QMetaObject>

#ifdef HAVE_RTMIDI
#include <RtMidi.h>
#endif

MidiInputService::MidiInputService(QObject* parent) : QObject(parent) {}

MidiInputService::~MidiInputService() { stop(); }

bool MidiInputService::start() {
#ifdef HAVE_RTMIDI
  try {
    if (!midiIn_) {
      midiIn_ = std::make_unique<RtMidiIn>();
    }

    if (midiIn_->getPortCount() == 0) {
      emit statusMessage("MIDI enabled but no MIDI input ports are available.");
      return false;
    }

    if (!midiIn_->isPortOpen()) {
      midiIn_->openPort(0);
      midiIn_->ignoreTypes(false, false, false);
      midiIn_->setCallback(&MidiInputService::midiCallback, this);
    }

    running_ = true;
    emit statusMessage(QString("MIDI listening on: %1").arg(QString::fromStdString(midiIn_->getPortName(0))));
    return true;
  } catch (const RtMidiError& error) {
    emit statusMessage(QString("MIDI start failed: %1").arg(QString::fromStdString(error.getMessage())));
    return false;
  }
#else
  emit statusMessage("MIDI support unavailable. Install RtMidi and rebuild.");
  return false;
#endif
}

void MidiInputService::stop() {
#ifdef HAVE_RTMIDI
  if (midiIn_ && midiIn_->isPortOpen()) {
    midiIn_->cancelCallback();
    midiIn_->closePort();
    emit statusMessage("MIDI stopped.");
  }
#endif
  running_ = false;
}

bool MidiInputService::isAvailable() const {
#ifdef HAVE_RTMIDI
  return true;
#else
  return false;
#endif
}

#ifdef HAVE_RTMIDI
void MidiInputService::midiCallback(double, std::vector<unsigned char>* message, void* userData) {
  auto* self = static_cast<MidiInputService*>(userData);
  if (self == nullptr || message == nullptr || message->empty()) {
    return;
  }

  const std::vector<unsigned char> copy = *message;
  QMetaObject::invokeMethod(self, [self, copy]() { self->handleMessage(copy); }, Qt::QueuedConnection);
}

void MidiInputService::handleMessage(const std::vector<unsigned char>& message) {
  if (message.empty()) {
    return;
  }

  const unsigned char status = message[0];

  // Note on: emit the incoming MIDI note number for cue-note matching in the UI layer.
  if ((status & 0xF0) == 0x90 && message.size() >= 3) {
    const int note = static_cast<int>(message[1]);
    const int velocity = static_cast<int>(message[2]);
    if (velocity > 0) {
      emit cueNoteRequested(note);
    }
    return;
  }

  // MTC quarter frame message.
  if (status == 0xF1 && message.size() >= 2) {
    const int data = static_cast<int>(message[1]);
    const int type = (data >> 4) & 0x07;
    const int value = data & 0x0F;
    mtcNibbles_[type] = value;

    if (type == 7) {
      const int frame = mtcNibbles_[0] | ((mtcNibbles_[1] & 0x1) << 4);
      const int second = mtcNibbles_[2] | (mtcNibbles_[3] << 4);
      const int minute = mtcNibbles_[4] | (mtcNibbles_[5] << 4);
      const int hour = mtcNibbles_[6] | ((mtcNibbles_[7] & 0x1) << 4);

      emit timecodeReceived(QString("%1:%2:%3:%4")
                                .arg(hour, 2, 10, QChar('0'))
                                .arg(minute, 2, 10, QChar('0'))
                                .arg(second, 2, 10, QChar('0'))
                                .arg(frame, 2, 10, QChar('0')));
    }
  }
}
#endif
