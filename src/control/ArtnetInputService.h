#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class QUdpSocket;

class ArtnetInputService : public QObject {
  Q_OBJECT

 public:
  explicit ArtnetInputService(QObject* parent = nullptr);

  bool start(quint16 port, int universe);
  void stop();
  bool isRunning() const;
  quint16 port() const;
  int universe() const;

 signals:
  void dmxValueReceived(int channel, int value);
  void statusMessage(const QString& message);

 private slots:
  void readPendingDatagrams();

 private:
  bool parseArtDmxPacket(const QByteArray& datagram, int* universeOut, QByteArray* dataOut) const;

  QUdpSocket* socket_;
  quint16 port_ = 0;
  int universe_ = 0;
  QByteArray lastLevels_;
};
