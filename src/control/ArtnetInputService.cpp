#include "control/ArtnetInputService.h"

#include <QtEndian>
#include <QHostAddress>
#include <QUdpSocket>
#include <QtGlobal>

ArtnetInputService::ArtnetInputService(QObject* parent) : QObject(parent), socket_(new QUdpSocket(this)) {
  connect(socket_, &QUdpSocket::readyRead, this, &ArtnetInputService::readPendingDatagrams);
}

bool ArtnetInputService::start(quint16 port, int universe) {
  stop();

  if (universe < 0 || universe > 32767) {
    emit statusMessage("Art-Net universe must be between 0 and 32767.");
    return false;
  }

  if (!socket_->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
    emit statusMessage(QString("Art-Net bind failed on UDP %1: %2").arg(port).arg(socket_->errorString()));
    return false;
  }

  port_ = port;
  universe_ = universe;
  lastLevels_.clear();
  lastLevels_.resize(512);
  lastLevels_.fill('\0');

  emit statusMessage(QString("Art-Net listening on UDP %1 universe %2").arg(port_).arg(universe_));
  return true;
}

void ArtnetInputService::stop() {
  if (socket_->state() == QAbstractSocket::BoundState) {
    socket_->close();
    emit statusMessage("Art-Net stopped.");
  }

  port_ = 0;
  universe_ = 0;
  lastLevels_.clear();
}

bool ArtnetInputService::isRunning() const { return socket_->state() == QAbstractSocket::BoundState; }

quint16 ArtnetInputService::port() const { return port_; }

int ArtnetInputService::universe() const { return universe_; }

void ArtnetInputService::readPendingDatagrams() {
  while (socket_->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(static_cast<int>(socket_->pendingDatagramSize()));
    socket_->readDatagram(datagram.data(), datagram.size());

    int packetUniverse = -1;
    QByteArray levels;
    if (!parseArtDmxPacket(datagram, &packetUniverse, &levels)) {
      continue;
    }

    if (packetUniverse != universe_) {
      continue;
    }

    if (lastLevels_.isEmpty()) {
      lastLevels_.resize(512);
      lastLevels_.fill('\0');
    }

    const int channels = qMin(512, levels.size());
    for (int channel = 0; channel < channels; ++channel) {
      const unsigned char level = static_cast<unsigned char>(levels.at(channel));
      const unsigned char previous = static_cast<unsigned char>(lastLevels_.at(channel));
      if (level == previous) {
        continue;
      }

      lastLevels_[channel] = static_cast<char>(level);
      emit dmxValueReceived(channel + 1, static_cast<int>(level));
    }
  }
}

bool ArtnetInputService::parseArtDmxPacket(const QByteArray& datagram, int* universeOut, QByteArray* dataOut) const {
  if (universeOut == nullptr || dataOut == nullptr || datagram.size() < 18) {
    return false;
  }

  static const QByteArray kArtNetId("Art-Net\0", 8);
  if (datagram.left(8) != kArtNetId) {
    return false;
  }

  const quint16 opCode = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(datagram.constData() + 8));
  if (opCode != 0x5000) {  // OpDmx
    return false;
  }

  const quint16 packetUniverse = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(datagram.constData() + 14));
  const quint16 payloadLength = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(datagram.constData() + 16));
  if (payloadLength == 0 || datagram.size() < 18 + payloadLength) {
    return false;
  }

  *universeOut = static_cast<int>(packetUniverse);
  *dataOut = datagram.mid(18, payloadLength);
  return true;
}
