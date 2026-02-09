#include "control/FailoverSyncService.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUdpSocket>
#include <QUuid>

namespace {

constexpr int kMaxRecentEvents = 512;

}  // namespace

FailoverSyncService::FailoverSyncService(QObject* parent)
    : QObject(parent), socket_(new QUdpSocket(this)), nodeId_(QUuid::createUuid().toString(QUuid::WithoutBraces)) {
  connect(socket_, &QUdpSocket::readyRead, this, &FailoverSyncService::readPendingDatagrams);
}

bool FailoverSyncService::start(quint16 listenPort, const QString& sharedKey) {
  stop();

  sharedKey_ = sharedKey.trimmed();
  if (sharedKey_.isEmpty()) {
    emit statusMessage("Failover sync shared key is required.");
    return false;
  }

  if (!socket_->bind(QHostAddress::AnyIPv4, listenPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
    emit statusMessage(QString("Failover sync bind failed on UDP %1: %2").arg(listenPort).arg(socket_->errorString()));
    return false;
  }

  listenPort_ = listenPort;
  emit statusMessage(QString("Failover sync listening on UDP %1").arg(listenPort_));
  return true;
}

void FailoverSyncService::stop() {
  if (socket_->state() == QAbstractSocket::BoundState) {
    socket_->close();
    emit statusMessage("Failover sync stopped.");
  }
  listenPort_ = 0;
}

bool FailoverSyncService::isRunning() const { return socket_->state() == QAbstractSocket::BoundState; }

void FailoverSyncService::setPeer(const QString& host, quint16 port) {
  peerAddress_ = QHostAddress();
  peerPort_ = port;

  const QString trimmedHost = host.trimmed();
  if (trimmedHost.isEmpty() || port == 0) {
    return;
  }

  QHostAddress direct;
  if (direct.setAddress(trimmedHost)) {
    peerAddress_ = direct;
    return;
  }

  const QHostInfo info = QHostInfo::fromName(trimmedHost);
  if (!info.addresses().isEmpty()) {
    peerAddress_ = info.addresses().first();
  } else {
    emit statusMessage(QString("Failover peer host resolution failed: %1").arg(trimmedHost));
  }
}

void FailoverSyncService::publishCueLive(const Cue& cue) {
  QJsonObject payload;
  payload.insert("cueId", cue.id);
  payload.insert("cueName", cue.name);
  payload.insert("targetSetId", cue.targetSetId);
  payload.insert("targetScreen", cue.targetScreen);
  payload.insert("layer", cue.layer);
  sendEvent("cue_live", payload);
}

void FailoverSyncService::publishStopAll() { sendEvent("stop_all", QJsonObject{}); }

void FailoverSyncService::publishOverlayText(const QString& text) {
  QJsonObject payload;
  payload.insert("text", text);
  sendEvent("overlay_text", payload);
}

void FailoverSyncService::readPendingDatagrams() {
  while (socket_->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(static_cast<int>(socket_->pendingDatagramSize()));
    socket_->readDatagram(datagram.data(), datagram.size());

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(datagram, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
      continue;
    }

    const QJsonObject envelope = document.object();
    const QString eventId = envelope.value("eventId").toString();
    const QString source = envelope.value("source").toString();
    const QString type = envelope.value("type").toString();
    const QString auth = envelope.value("auth").toString();
    const QJsonObject payload = envelope.value("payload").toObject();

    if (eventId.isEmpty() || source.isEmpty() || type.isEmpty() || auth.isEmpty()) {
      continue;
    }

    if (source == nodeId_) {
      continue;
    }

    if (hasSeenEvent(eventId)) {
      continue;
    }

    if (auth != computeAuth(eventId, type, payload)) {
      continue;
    }

    rememberEvent(eventId);

    if (type == "cue_live") {
      const QString cueId = payload.value("cueId").toString();
      if (!cueId.isEmpty()) {
        emit remoteCueLiveRequested(cueId);
      }
      continue;
    }

    if (type == "stop_all") {
      emit remoteStopAllRequested();
      continue;
    }

    if (type == "overlay_text") {
      emit remoteOverlayTextReceived(payload.value("text").toString());
      continue;
    }
  }
}

void FailoverSyncService::sendEvent(const QString& type, const QJsonObject& payload) {
  if (!isRunning()) {
    return;
  }

  if (peerAddress_.isNull() || peerPort_ == 0) {
    return;
  }

  const QString eventId = QUuid::createUuid().toString(QUuid::WithoutBraces);
  rememberEvent(eventId);

  QJsonObject envelope;
  envelope.insert("version", 1);
  envelope.insert("eventId", eventId);
  envelope.insert("source", nodeId_);
  envelope.insert("type", type);
  envelope.insert("timestampUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  envelope.insert("auth", computeAuth(eventId, type, payload));
  envelope.insert("payload", payload);

  const QByteArray datagram = QJsonDocument(envelope).toJson(QJsonDocument::Compact);
  socket_->writeDatagram(datagram, peerAddress_, peerPort_);
}

QString FailoverSyncService::computeAuth(const QString& eventId, const QString& type, const QJsonObject& payload) const {
  const QByteArray payloadBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);
  const QString material = QString("%1|%2|%3|%4").arg(sharedKey_, eventId, type, QString::fromUtf8(payloadBytes));
  const QByteArray hash = QCryptographicHash::hash(material.toUtf8(), QCryptographicHash::Sha256);
  return hash.toHex();
}

void FailoverSyncService::rememberEvent(const QString& eventId) {
  if (eventId.isEmpty()) {
    return;
  }

  if (recentEventIds_.contains(eventId)) {
    return;
  }

  recentEventIds_.insert(eventId);
  recentEventOrder_.push_back(eventId);

  while (recentEventOrder_.size() > kMaxRecentEvents) {
    const QString oldest = recentEventOrder_.front();
    recentEventOrder_.pop_front();
    recentEventIds_.remove(oldest);
  }
}

bool FailoverSyncService::hasSeenEvent(const QString& eventId) const { return recentEventIds_.contains(eventId); }
