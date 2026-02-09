#pragma once

#include <QObject>
#include <QString>

#include <vector>

#ifdef HAVE_RTMIDI
#include <memory>

class RtMidiIn;
#endif

class MidiInputService : public QObject {
  Q_OBJECT

 public:
  explicit MidiInputService(QObject* parent = nullptr);
  ~MidiInputService() override;

  bool start();
  void stop();
  bool isAvailable() const;

 signals:
  void cueNoteRequested(int note);
  void timecodeReceived(const QString& timecode);
  void statusMessage(const QString& message);

 private:
#ifdef HAVE_RTMIDI
  static void midiCallback(double timestamp, std::vector<unsigned char>* message, void* userData);
  void handleMessage(const std::vector<unsigned char>& message);

  std::unique_ptr<RtMidiIn> midiIn_;
  int mtcNibbles_[8] = {0};
#endif

  bool running_ = false;
};
