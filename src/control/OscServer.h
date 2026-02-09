#pragma once

#include <QObject>
#include <QString>
#include <QVector>

class QUdpSocket;

class OscServer : public QObject {
  Q_OBJECT

 public:
  explicit OscServer(QObject* parent = nullptr);

  bool start(quint16 port);
  void stop();
  quint16 port() const;

 signals:
  void playRowRequested(int row);
  void previewRowRequested(int row);
  void preloadRowRequested(int row);
  void takeRequested();
  void stopAllRequested();
  void timecodeReceived(const QString& timecode);
  void dmxValueReceived(int channel, int value);
  void overlayTextReceived(const QString& text);
  void statusMessage(const QString& message);

 private slots:
  void readPendingDatagrams();

 private:
  struct OscArgument {
    enum class Type { Int, Float, String } type = Type::String;
    int intValue = 0;
    float floatValue = 0.0f;
    QString stringValue;
  };

  struct OscMessage {
    QString address;
    QVector<OscArgument> args;
  };

  static bool parseOscPacket(const QByteArray& datagram, OscMessage* message);
  static bool parseTextCommand(const QByteArray& datagram, OscMessage* message);
  void dispatch(const OscMessage& message);

  QUdpSocket* socket_;
  quint16 port_ = 0;
};
