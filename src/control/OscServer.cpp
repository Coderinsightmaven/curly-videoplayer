#include "control/OscServer.h"

#include <cstring>

#include <QtEndian>
#include <QHostAddress>
#include <QRegularExpression>
#include <QUdpSocket>

namespace {

bool readPaddedString(const QByteArray& data, int* offset, QString* out) {
  if (offset == nullptr || out == nullptr || *offset < 0 || *offset >= data.size()) {
    return false;
  }

  const int start = *offset;
  int end = start;
  while (end < data.size() && data.at(end) != '\0') {
    ++end;
  }

  if (end >= data.size()) {
    return false;
  }

  *out = QString::fromUtf8(data.constData() + start, end - start);

  int aligned = end + 1;
  while (aligned % 4 != 0) {
    ++aligned;
  }

  if (aligned > data.size()) {
    return false;
  }

  *offset = aligned;
  return true;
}

int extractTrailingRow(const QString& address) {
  const QStringList segments = address.split('/', Qt::SkipEmptyParts);
  if (segments.isEmpty()) {
    return -1;
  }

  bool ok = false;
  const int row = segments.last().toInt(&ok);
  return ok ? row : -1;
}

}  // namespace

OscServer::OscServer(QObject* parent) : QObject(parent), socket_(new QUdpSocket(this)) {
  connect(socket_, &QUdpSocket::readyRead, this, &OscServer::readPendingDatagrams);
}

bool OscServer::start(quint16 port) {
  stop();

  if (!socket_->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
    emit statusMessage(QString("OSC bind failed on port %1: %2").arg(port).arg(socket_->errorString()));
    return false;
  }

  port_ = port;
  emit statusMessage(QString("OSC listening on UDP %1").arg(port_));
  return true;
}

void OscServer::stop() {
  if (socket_->state() == QAbstractSocket::BoundState) {
    socket_->close();
    emit statusMessage("OSC stopped.");
  }
  port_ = 0;
}

quint16 OscServer::port() const { return port_; }

void OscServer::readPendingDatagrams() {
  while (socket_->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(static_cast<int>(socket_->pendingDatagramSize()));
    socket_->readDatagram(datagram.data(), datagram.size());

    OscMessage message;
    if (!parseOscPacket(datagram, &message) && !parseTextCommand(datagram, &message)) {
      emit statusMessage("Received unsupported OSC packet.");
      continue;
    }

    dispatch(message);
  }
}

bool OscServer::parseOscPacket(const QByteArray& datagram, OscMessage* message) {
  if (message == nullptr || datagram.isEmpty() || datagram.at(0) != '/') {
    return false;
  }

  int offset = 0;
  if (!readPaddedString(datagram, &offset, &message->address)) {
    return false;
  }

  QString typeTags;
  if (!readPaddedString(datagram, &offset, &typeTags)) {
    return false;
  }

  if (!typeTags.startsWith(',')) {
    return false;
  }

  message->args.clear();

  for (int i = 1; i < typeTags.size(); ++i) {
    const QChar type = typeTags.at(i);
    OscArgument arg;

    if (type == 'i') {
      if (offset + 4 > datagram.size()) {
        return false;
      }
      arg.type = OscArgument::Type::Int;
      arg.intValue = static_cast<int>(qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(datagram.constData() + offset)));
      offset += 4;
      message->args.push_back(arg);
      continue;
    }

    if (type == 'f') {
      if (offset + 4 > datagram.size()) {
        return false;
      }
      arg.type = OscArgument::Type::Float;
      quint32 raw = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(datagram.constData() + offset));
      float value;
      static_assert(sizeof(float) == sizeof(quint32), "Unexpected float size");
      std::memcpy(&value, &raw, sizeof(float));
      arg.floatValue = value;
      offset += 4;
      message->args.push_back(arg);
      continue;
    }

    if (type == 's') {
      arg.type = OscArgument::Type::String;
      if (!readPaddedString(datagram, &offset, &arg.stringValue)) {
        return false;
      }
      message->args.push_back(arg);
      continue;
    }

    return false;
  }

  return true;
}

bool OscServer::parseTextCommand(const QByteArray& datagram, OscMessage* message) {
  if (message == nullptr) {
    return false;
  }

  const QString line = QString::fromUtf8(datagram).trimmed();
  if (line.isEmpty()) {
    return false;
  }

  const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (parts.isEmpty()) {
    return false;
  }

  OscMessage parsed;
  QString command = parts.first().toLower();

  if (!command.startsWith('/')) {
    if (command == "play") {
      command = "/cue/play";
    } else if (command == "preview") {
      command = "/cue/preview";
    } else if (command == "preload") {
      command = "/cue/preload";
    } else if (command == "take") {
      command = "/cue/take";
    } else if (command == "stopall") {
      command = "/cue/stop_all";
    } else if (command == "timecode") {
      command = "/timecode";
    } else if (command == "dmx") {
      command = "/dmx";
    } else if (command == "text") {
      command = "/text";
    } else {
      return false;
    }
  }

  parsed.address = command;

  if (parsed.address == "/text") {
    OscArgument arg;
    arg.type = OscArgument::Type::String;
    arg.stringValue = parts.mid(1).join(' ');
    if (!arg.stringValue.isNull()) {
      parsed.args.push_back(arg);
    }
    *message = parsed;
    return true;
  }

  for (int i = 1; i < parts.size(); ++i) {
    const QString token = parts.at(i);
    bool ok = false;
    const int value = token.toInt(&ok);
    OscArgument arg;
    if (ok) {
      arg.type = OscArgument::Type::Int;
      arg.intValue = value;
    } else {
      arg.type = OscArgument::Type::String;
      arg.stringValue = token;
    }
    parsed.args.push_back(arg);
  }

  *message = parsed;
  return true;
}

void OscServer::dispatch(const OscMessage& message) {
  auto readRowArg = [&message]() -> int {
    if (!message.args.isEmpty() && message.args.first().type == OscArgument::Type::Int) {
      return message.args.first().intValue;
    }
    return extractTrailingRow(message.address);
  };

  if (message.address.startsWith("/cue/play")) {
    const int row = readRowArg();
    if (row >= 0) {
      emit playRowRequested(row);
    }
    return;
  }

  if (message.address.startsWith("/cue/preview")) {
    const int row = readRowArg();
    if (row >= 0) {
      emit previewRowRequested(row);
    }
    return;
  }

  if (message.address.startsWith("/cue/preload")) {
    const int row = readRowArg();
    if (row >= 0) {
      emit preloadRowRequested(row);
    }
    return;
  }

  if (message.address == "/cue/take" || message.address == "/take") {
    emit takeRequested();
    return;
  }

  if (message.address == "/cue/stop_all" || message.address == "/stop_all") {
    emit stopAllRequested();
    return;
  }

  if (message.address == "/timecode" || message.address == "/tc") {
    if (!message.args.isEmpty()) {
      if (message.args.first().type == OscArgument::Type::String) {
        emit timecodeReceived(message.args.first().stringValue);
      } else if (message.args.first().type == OscArgument::Type::Int) {
        emit timecodeReceived(QString::number(message.args.first().intValue));
      }
    }
    return;
  }

  if (message.address == "/dmx") {
    if (message.args.size() >= 2 && message.args.at(0).type == OscArgument::Type::Int &&
        message.args.at(1).type == OscArgument::Type::Int) {
      emit dmxValueReceived(message.args.at(0).intValue, message.args.at(1).intValue);
    }
    return;
  }

  if (message.address == "/text" || message.address == "/overlay/text") {
    if (!message.args.isEmpty()) {
      if (message.args.first().type == OscArgument::Type::String) {
        emit overlayTextReceived(message.args.first().stringValue);
      } else if (message.args.first().type == OscArgument::Type::Int) {
        emit overlayTextReceived(QString::number(message.args.first().intValue));
      }
    } else {
      emit overlayTextReceived(QString());
    }
    return;
  }

  emit statusMessage(QString("Unhandled OSC address: %1").arg(message.address));
}
