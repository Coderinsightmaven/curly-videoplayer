#pragma once

#include <QMap>
#include <QString>

#include "core/Transition.h"

struct AppConfig {
  int oscPort = 9000;
  int transitionDurationMs = 600;
  TransitionStyle transitionStyle = TransitionStyle::Fade;
  QString fallbackSlatePath;
  bool useRelativeMediaPaths = true;
  bool midiEnabled = true;
  bool ndiEnabled = false;
  bool syphonEnabled = false;
  bool deckLinkEnabled = false;
  bool backupTriggerEnabled = false;
  QString backupTriggerUrl;
  QString backupTriggerToken;
  int backupTriggerTimeoutMs = 1500;
  QMap<QString, QString> filterPresets;
  bool artnetEnabled = false;
  int artnetPort = 6454;
  int artnetUniverse = 0;
  bool failoverSyncEnabled = false;
  QString failoverPeerHost;
  int failoverPeerPort = 9101;
  int failoverListenPort = 9101;
  QString failoverSharedKey;
};
