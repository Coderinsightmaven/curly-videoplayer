#include "project/ProjectSerializer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

bool isLikelyUrl(const QString& path) { return path.contains("://"); }

QString toPortablePath(const QString& path, const QDir& baseDir, bool useRelative) {
  if (path.isEmpty() || isLikelyUrl(path)) {
    return path;
  }

  QFileInfo fileInfo(path);
  const QString absolute = fileInfo.isAbsolute() ? fileInfo.absoluteFilePath() : baseDir.absoluteFilePath(path);
  if (!useRelative) {
    return QDir::cleanPath(absolute);
  }

  return QDir::cleanPath(baseDir.relativeFilePath(absolute));
}

QString toAbsolutePath(const QString& path, const QDir& baseDir) {
  if (path.isEmpty() || isLikelyUrl(path)) {
    return path;
  }

  QFileInfo fileInfo(path);
  if (fileInfo.isAbsolute()) {
    return QDir::cleanPath(fileInfo.absoluteFilePath());
  }

  return QDir::cleanPath(baseDir.absoluteFilePath(path));
}

QJsonObject cueToJson(const Cue& cue, const QDir& baseDir, bool useRelativeMediaPaths) {
  QJsonObject object;
  object.insert("id", cue.id);
  object.insert("name", cue.name);
  object.insert("filePath", toPortablePath(cue.filePath, baseDir, useRelativeMediaPaths));
  object.insert("targetScreen", cue.targetScreen);
  object.insert("targetSetId", cue.targetSetId);
  object.insert("layer", cue.layer);
  object.insert("loop", cue.loop);
  object.insert("hotkey", cue.hotkey);
  object.insert("timecodeTrigger", cue.timecodeTrigger);
  object.insert("midiNote", cue.midiNote);
  object.insert("dmxChannel", cue.dmxChannel);
  object.insert("dmxValue", cue.dmxValue);
  object.insert("preload", cue.preload);
  object.insert("isLiveInput", cue.isLiveInput);
  object.insert("liveInputUrl", cue.liveInputUrl);
  object.insert("filterPresetId", cue.filterPresetId);
  object.insert("videoFilter", cue.videoFilter);
  object.insert("useTransitionOverride", cue.useTransitionOverride);
  object.insert("transitionStyle", transitionStyleToString(cue.transitionStyle));
  object.insert("transitionDurationMs", cue.transitionDurationMs);
  object.insert("autoFollow", cue.autoFollow);
  object.insert("followCueRow", cue.followCueRow);
  object.insert("followDelayMs", cue.followDelayMs);
  object.insert("playlistId", cue.playlistId);
  object.insert("playlistAutoAdvance", cue.playlistAutoAdvance);
  object.insert("playlistLoop", cue.playlistLoop);
  object.insert("playlistAdvanceDelayMs", cue.playlistAdvanceDelayMs);
  object.insert("autoStopMs", cue.autoStopMs);
  return object;
}

Cue cueFromJson(const QJsonObject& object, const QDir& baseDir) {
  Cue cue;
  cue.id = object.value("id").toString();
  cue.name = object.value("name").toString();
  cue.filePath = toAbsolutePath(object.value("filePath").toString(), baseDir);
  cue.targetScreen = object.value("targetScreen").toInt();
  cue.targetSetId = object.value("targetSetId").toString();
  cue.layer = object.value("layer").toInt();
  cue.loop = object.value("loop").toBool();
  cue.hotkey = object.value("hotkey").toString();
  cue.timecodeTrigger = object.value("timecodeTrigger").toString();
  cue.midiNote = object.value("midiNote").toInt(-1);
  cue.dmxChannel = object.value("dmxChannel").toInt(-1);
  cue.dmxValue = object.value("dmxValue").toInt(255);
  cue.preload = object.value("preload").toBool(false);
  cue.isLiveInput = object.value("isLiveInput").toBool(false);
  cue.liveInputUrl = object.value("liveInputUrl").toString();
  cue.filterPresetId = object.value("filterPresetId").toString();
  cue.videoFilter = object.value("videoFilter").toString();
  cue.useTransitionOverride = object.value("useTransitionOverride").toBool(false);
  cue.transitionStyle = transitionStyleFromString(object.value("transitionStyle").toString("fade"));
  cue.transitionDurationMs = object.value("transitionDurationMs").toInt(600);
  cue.autoFollow = object.value("autoFollow").toBool(false);
  cue.followCueRow = object.value("followCueRow").toInt(-1);
  cue.followDelayMs = object.value("followDelayMs").toInt(0);
  cue.playlistId = object.value("playlistId").toString();
  cue.playlistAutoAdvance = object.value("playlistAutoAdvance").toBool(false);
  cue.playlistLoop = object.value("playlistLoop").toBool(false);
  cue.playlistAdvanceDelayMs = object.value("playlistAdvanceDelayMs").toInt(0);
  cue.autoStopMs = object.value("autoStopMs").toInt(0);
  return cue;
}

QJsonObject calibrationToJson(int screenIndex, const OutputCalibration& calibration) {
  QJsonObject object;
  object.insert("screen", screenIndex);
  object.insert("edgeBlendPx", calibration.edgeBlendPx);
  object.insert("keystoneHorizontal", calibration.keystoneHorizontal);
  object.insert("keystoneVertical", calibration.keystoneVertical);
  object.insert("maskEnabled", calibration.maskEnabled);
  object.insert("maskLeftPx", calibration.maskLeftPx);
  object.insert("maskTopPx", calibration.maskTopPx);
  object.insert("maskRightPx", calibration.maskRightPx);
  object.insert("maskBottomPx", calibration.maskBottomPx);
  return object;
}

OutputCalibration calibrationFromJson(const QJsonObject& object) {
  OutputCalibration calibration;
  calibration.edgeBlendPx = object.value("edgeBlendPx").toInt(0);
  calibration.keystoneHorizontal = object.value("keystoneHorizontal").toInt(0);
  calibration.keystoneVertical = object.value("keystoneVertical").toInt(0);
  calibration.maskEnabled = object.value("maskEnabled").toBool(false);
  calibration.maskLeftPx = object.value("maskLeftPx").toInt(0);
  calibration.maskTopPx = object.value("maskTopPx").toInt(0);
  calibration.maskRightPx = object.value("maskRightPx").toInt(0);
  calibration.maskBottomPx = object.value("maskBottomPx").toInt(0);
  return calibration;
}

QJsonObject configToJson(const AppConfig& config, const QDir& baseDir) {
  QJsonObject object;
  object.insert("oscPort", config.oscPort);
  object.insert("transitionDurationMs", config.transitionDurationMs);
  object.insert("transitionStyle", transitionStyleToString(config.transitionStyle));
  object.insert("fallbackSlatePath", toPortablePath(config.fallbackSlatePath, baseDir, config.useRelativeMediaPaths));
  object.insert("useRelativeMediaPaths", config.useRelativeMediaPaths);
  object.insert("midiEnabled", config.midiEnabled);
  object.insert("ndiEnabled", config.ndiEnabled);
  object.insert("syphonEnabled", config.syphonEnabled);
  object.insert("deckLinkEnabled", config.deckLinkEnabled);
  object.insert("backupTriggerEnabled", config.backupTriggerEnabled);
  object.insert("backupTriggerUrl", config.backupTriggerUrl);
  object.insert("backupTriggerToken", config.backupTriggerToken);
  object.insert("backupTriggerTimeoutMs", config.backupTriggerTimeoutMs);
  QJsonObject filterPresets;
  for (auto it = config.filterPresets.constBegin(); it != config.filterPresets.constEnd(); ++it) {
    filterPresets.insert(it.key(), it.value());
  }
  object.insert("filterPresets", filterPresets);
  object.insert("artnetEnabled", config.artnetEnabled);
  object.insert("artnetPort", config.artnetPort);
  object.insert("artnetUniverse", config.artnetUniverse);
  object.insert("failoverSyncEnabled", config.failoverSyncEnabled);
  object.insert("failoverPeerHost", config.failoverPeerHost);
  object.insert("failoverPeerPort", config.failoverPeerPort);
  object.insert("failoverListenPort", config.failoverListenPort);
  object.insert("failoverSharedKey", config.failoverSharedKey);
  return object;
}

AppConfig configFromJson(const QJsonObject& object, const QDir& baseDir) {
  AppConfig config;
  config.oscPort = object.value("oscPort").toInt(9000);
  config.transitionDurationMs = object.value("transitionDurationMs").toInt(600);
  config.transitionStyle = transitionStyleFromString(object.value("transitionStyle").toString("fade"));
  config.useRelativeMediaPaths = object.value("useRelativeMediaPaths").toBool(true);
  config.fallbackSlatePath = toAbsolutePath(object.value("fallbackSlatePath").toString(), baseDir);
  config.midiEnabled = object.value("midiEnabled").toBool(true);
  config.ndiEnabled = object.value("ndiEnabled").toBool(false);
  config.syphonEnabled = object.value("syphonEnabled").toBool(false);
  config.deckLinkEnabled = object.value("deckLinkEnabled").toBool(false);
  config.backupTriggerEnabled = object.value("backupTriggerEnabled").toBool(false);
  config.backupTriggerUrl = object.value("backupTriggerUrl").toString();
  config.backupTriggerToken = object.value("backupTriggerToken").toString();
  config.backupTriggerTimeoutMs = object.value("backupTriggerTimeoutMs").toInt(1500);
  config.filterPresets.clear();
  const QJsonObject filterPresetsObject = object.value("filterPresets").toObject();
  for (auto it = filterPresetsObject.constBegin(); it != filterPresetsObject.constEnd(); ++it) {
    config.filterPresets.insert(it.key(), it.value().toString());
  }
  config.artnetEnabled = object.value("artnetEnabled").toBool(false);
  config.artnetPort = object.value("artnetPort").toInt(6454);
  config.artnetUniverse = object.value("artnetUniverse").toInt(0);
  config.failoverSyncEnabled = object.value("failoverSyncEnabled").toBool(false);
  config.failoverPeerHost = object.value("failoverPeerHost").toString();
  config.failoverPeerPort = object.value("failoverPeerPort").toInt(9101);
  config.failoverListenPort = object.value("failoverListenPort").toInt(9101);
  config.failoverSharedKey = object.value("failoverSharedKey").toString();
  return config;
}

}  // namespace

bool ProjectSerializer::save(const QString& filePath, const ProjectData& project, QString* errorMessage) {
  const QDir baseDir = QFileInfo(filePath).absoluteDir();

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (errorMessage != nullptr) {
      *errorMessage = QString("Failed to open project file for writing: %1").arg(file.errorString());
    }
    return false;
  }

  QJsonObject root;
  root.insert("version", 1);
  root.insert("config", configToJson(project.config, baseDir));

  QJsonArray cues;
  for (const Cue& cue : project.cues) {
    cues.push_back(cueToJson(cue, baseDir, project.config.useRelativeMediaPaths));
  }
  root.insert("cues", cues);

  QJsonArray calibrations;
  for (auto it = project.calibrations.constBegin(); it != project.calibrations.constEnd(); ++it) {
    calibrations.push_back(calibrationToJson(it.key(), it.value()));
  }
  root.insert("calibrations", calibrations);

  const QJsonDocument document(root);
  const QByteArray payload = document.toJson(QJsonDocument::Indented);
  const qint64 written = file.write(payload);
  if (written < 0 || written != payload.size()) {
    if (errorMessage != nullptr) {
      *errorMessage = QString("Failed to write project file (%1 of %2 bytes): %3")
                          .arg(written)
                          .arg(payload.size())
                          .arg(file.errorString());
    }
    return false;
  }

  if (!file.flush()) {
    if (errorMessage != nullptr) {
      *errorMessage = QString("Failed to flush project file: %1").arg(file.errorString());
    }
    return false;
  }

  return true;
}

bool ProjectSerializer::load(const QString& filePath, ProjectData* project, QString* errorMessage) {
  if (project == nullptr) {
    if (errorMessage != nullptr) {
      *errorMessage = "Project output was null.";
    }
    return false;
  }

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    if (errorMessage != nullptr) {
      *errorMessage = QString("Failed to open project file: %1").arg(file.errorString());
    }
    return false;
  }

  const QByteArray data = file.readAll();
  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
    if (errorMessage != nullptr) {
      *errorMessage = QString("Invalid project JSON: %1").arg(parseError.errorString());
    }
    return false;
  }

  const QJsonObject root = document.object();
  const QDir baseDir = QFileInfo(filePath).absoluteDir();

  ProjectData loaded;
  loaded.config = configFromJson(root.value("config").toObject(), baseDir);

  const QJsonArray cuesArray = root.value("cues").toArray();
  loaded.cues.reserve(cuesArray.size());
  for (const QJsonValue& value : cuesArray) {
    if (!value.isObject()) {
      continue;
    }
    loaded.cues.push_back(cueFromJson(value.toObject(), baseDir));
  }

  const QJsonArray calibrationArray = root.value("calibrations").toArray();
  for (const QJsonValue& value : calibrationArray) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject object = value.toObject();
    const int screen = object.value("screen").toInt(0);
    loaded.calibrations.insert(screen, calibrationFromJson(object));
  }

  *project = loaded;
  return true;
}
