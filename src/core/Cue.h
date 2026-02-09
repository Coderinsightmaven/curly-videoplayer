#pragma once

#include <QString>

struct Cue {
  QString id;
  QString name;
  QString filePath;
  int targetScreen = 0;
  int layer = 0;
  bool loop = false;
  QString hotkey;
  QString timecodeTrigger;
  int midiNote = -1;
  bool preload = false;
};
