#pragma once

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
  bool backupTriggerEnabled = false;
  QString backupTriggerUrl;
  QString backupTriggerToken;
  int backupTriggerTimeoutMs = 1500;
};
