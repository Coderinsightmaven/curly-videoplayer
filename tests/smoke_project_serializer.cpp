#include <iostream>

#include <QCoreApplication>
#include <QTemporaryDir>

#include "project/ProjectSerializer.h"

namespace {

bool require(bool condition, const char* message) {
  if (condition) {
    return true;
  }

  std::cerr << "Smoke check failed: " << message << '\n';
  return false;
}

}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  Q_UNUSED(app);

  QTemporaryDir tempDir;
  if (!require(tempDir.isValid(), "Could not create temporary directory.")) {
    return 1;
  }

  ProjectData input;
  Cue cue;
  cue.id = "cue-001";
  cue.name = "Intro";
  cue.filePath = "/tmp/intro.mov";
  cue.targetScreen = 2;
  cue.targetSetId = "all-screens";
  cue.layer = 1;
  cue.loop = true;
  cue.hotkey = "Ctrl+1";
  cue.timecodeTrigger = "01:00:00:00";
  cue.midiNote = 64;
  cue.dmxChannel = 12;
  cue.dmxValue = 200;
  cue.preload = true;
  cue.isLiveInput = false;
  cue.liveInputUrl = "av://camera0";
  cue.videoFilter = "hflip";
  cue.useTransitionOverride = true;
  cue.transitionStyle = TransitionStyle::Cut;
  cue.transitionDurationMs = 90;
  cue.autoFollow = true;
  cue.followCueRow = 0;
  cue.followDelayMs = 250;
  input.cues.push_back(cue);

  OutputCalibration calibration;
  calibration.edgeBlendPx = 24;
  calibration.keystoneHorizontal = 6;
  calibration.keystoneVertical = -5;
  input.calibrations.insert(2, calibration);

  input.config.oscPort = 9100;
  input.config.transitionDurationMs = 750;
  input.config.transitionStyle = TransitionStyle::DipToBlack;
  input.config.fallbackSlatePath = "/tmp/slate.png";
  input.config.useRelativeMediaPaths = true;
  input.config.midiEnabled = true;
  input.config.ndiEnabled = true;
  input.config.backupTriggerEnabled = true;
  input.config.backupTriggerUrl = "https://backup.local/trigger";
  input.config.backupTriggerToken = "token-abc";
  input.config.backupTriggerTimeoutMs = 1750;

  const QString projectPath = tempDir.filePath("roundtrip.show");
  QString error;
  if (!ProjectSerializer::save(projectPath, input, &error)) {
    std::cerr << "Save failed: " << error.toStdString() << '\n';
    return 1;
  }

  ProjectData output;
  if (!ProjectSerializer::load(projectPath, &output, &error)) {
    std::cerr << "Load failed: " << error.toStdString() << '\n';
    return 1;
  }

  if (!require(output.cues.size() == 1, "Expected exactly one cue after load.")) {
    return 1;
  }

  const Cue loadedCue = output.cues.first();
  if (!require(loadedCue.id == cue.id, "Cue id mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.name == cue.name, "Cue name mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.filePath == cue.filePath, "Cue filePath mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.targetScreen == cue.targetScreen, "Cue targetScreen mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.targetSetId == cue.targetSetId, "Cue targetSetId mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.layer == cue.layer, "Cue layer mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.loop == cue.loop, "Cue loop mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.hotkey == cue.hotkey, "Cue hotkey mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.timecodeTrigger == cue.timecodeTrigger, "Cue timecodeTrigger mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.midiNote == cue.midiNote, "Cue midiNote mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.dmxChannel == cue.dmxChannel, "Cue dmxChannel mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.dmxValue == cue.dmxValue, "Cue dmxValue mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.preload == cue.preload, "Cue preload mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.isLiveInput == cue.isLiveInput, "Cue isLiveInput mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.liveInputUrl == cue.liveInputUrl, "Cue liveInputUrl mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.videoFilter == cue.videoFilter, "Cue videoFilter mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.useTransitionOverride == cue.useTransitionOverride, "Cue transition override mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.transitionStyle == cue.transitionStyle, "Cue transitionStyle mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.transitionDurationMs == cue.transitionDurationMs, "Cue transitionDurationMs mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.autoFollow == cue.autoFollow, "Cue autoFollow mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.followCueRow == cue.followCueRow, "Cue followCueRow mismatch.")) {
    return 1;
  }
  if (!require(loadedCue.followDelayMs == cue.followDelayMs, "Cue followDelayMs mismatch.")) {
    return 1;
  }

  if (!require(output.calibrations.contains(2), "Expected calibration for screen 2.")) {
    return 1;
  }

  const OutputCalibration loadedCalibration = output.calibrations.value(2);
  if (!require(loadedCalibration.edgeBlendPx == calibration.edgeBlendPx, "Calibration edgeBlendPx mismatch.")) {
    return 1;
  }
  if (!require(loadedCalibration.keystoneHorizontal == calibration.keystoneHorizontal,
               "Calibration keystoneHorizontal mismatch.")) {
    return 1;
  }
  if (!require(loadedCalibration.keystoneVertical == calibration.keystoneVertical,
               "Calibration keystoneVertical mismatch.")) {
    return 1;
  }

  if (!require(output.config.oscPort == input.config.oscPort, "Config oscPort mismatch.")) {
    return 1;
  }
  if (!require(output.config.transitionDurationMs == input.config.transitionDurationMs,
               "Config transitionDurationMs mismatch.")) {
    return 1;
  }
  if (!require(output.config.transitionStyle == input.config.transitionStyle, "Config transitionStyle mismatch.")) {
    return 1;
  }
  if (!require(output.config.fallbackSlatePath == input.config.fallbackSlatePath,
               "Config fallbackSlatePath mismatch.")) {
    return 1;
  }
  if (!require(output.config.useRelativeMediaPaths == input.config.useRelativeMediaPaths,
               "Config useRelativeMediaPaths mismatch.")) {
    return 1;
  }
  if (!require(output.config.midiEnabled == input.config.midiEnabled, "Config midiEnabled mismatch.")) {
    return 1;
  }
  if (!require(output.config.ndiEnabled == input.config.ndiEnabled, "Config ndiEnabled mismatch.")) {
    return 1;
  }
  if (!require(output.config.backupTriggerEnabled == input.config.backupTriggerEnabled,
               "Config backupTriggerEnabled mismatch.")) {
    return 1;
  }
  if (!require(output.config.backupTriggerUrl == input.config.backupTriggerUrl, "Config backupTriggerUrl mismatch.")) {
    return 1;
  }
  if (!require(output.config.backupTriggerToken == input.config.backupTriggerToken,
               "Config backupTriggerToken mismatch.")) {
    return 1;
  }
  if (!require(output.config.backupTriggerTimeoutMs == input.config.backupTriggerTimeoutMs,
               "Config backupTriggerTimeoutMs mismatch.")) {
    return 1;
  }

  std::cout << "project_serializer_smoke passed\n";
  return 0;
}
