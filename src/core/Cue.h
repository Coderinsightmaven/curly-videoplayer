#pragma once

#include <QString>

#include "core/Transition.h"

struct Cue {
  QString id;
  QString name;
  QString filePath;
  int targetScreen = 0;
  QString targetSetId;
  int layer = 0;
  bool loop = false;
  QString hotkey;
  QString timecodeTrigger;
  int midiNote = -1;
  int dmxChannel = -1;
  int dmxValue = 255;
  bool preload = false;
  bool isLiveInput = false;
  QString liveInputUrl;
  QString videoFilter;
  bool useTransitionOverride = false;
  TransitionStyle transitionStyle = TransitionStyle::Fade;
  int transitionDurationMs = 600;
  bool autoFollow = false;
  int followCueRow = -1;
  int followDelayMs = 0;
};
