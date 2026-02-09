#pragma once

#include <QHostAddress>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include "core/Cue.h"

class QUdpSocket;

class FailoverSyncService : public QObject {
  Q_OBJECT

 public:
  explicit FailoverSyncService(QObject* parent = nullptr);

  bool start(quint16 listenPort, const QString& sharedKey);
  void stop();
  bool isRunning() const;
  void setPeer(const QString& host, quint16 port);

  void publishCueLive(const Cue& cue);
  void publishStopAll();
  void publishOverlayText(const QString& text);

 signals:
  void remoteCueLiveRequested(const QString& cueId);
  void remoteStopAllRequested();
  void remoteOverlayTextReceived(const QString& text);
  void statusMessage(const QString& message);

 private slots:
  void readPendingDatagrams();

 private:
  void sendEvent(const QString& type, const QJsonObject& payload);
  QString computeAuth(const QString& eventId, const QString& type, const QJsonObject& payload) const;
  void rememberEvent(const QString& eventId);
  bool hasSeenEvent(const QString& eventId) const;

  QUdpSocket* socket_;
  QHostAddress peerAddress_;
  quint16 peerPort_ = 0;
  quint16 listenPort_ = 0;
  QString sharedKey_;
  QString nodeId_;
  QSet<QString> recentEventIds_;
  QStringList recentEventOrder_;
};
