#include "project/ProjectSerializer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QJsonObject cueToJson(const Cue& cue) {
  QJsonObject object;
  object.insert("id", cue.id);
  object.insert("name", cue.name);
  object.insert("filePath", cue.filePath);
  object.insert("targetScreen", cue.targetScreen);
  object.insert("layer", cue.layer);
  object.insert("loop", cue.loop);
  object.insert("hotkey", cue.hotkey);
  object.insert("timecodeTrigger", cue.timecodeTrigger);
  object.insert("midiNote", cue.midiNote);
  object.insert("preload", cue.preload);
  return object;
}

Cue cueFromJson(const QJsonObject& object) {
  Cue cue;
  cue.id = object.value("id").toString();
  cue.name = object.value("name").toString();
  cue.filePath = object.value("filePath").toString();
  cue.targetScreen = object.value("targetScreen").toInt();
  cue.layer = object.value("layer").toInt();
  cue.loop = object.value("loop").toBool();
  cue.hotkey = object.value("hotkey").toString();
  cue.timecodeTrigger = object.value("timecodeTrigger").toString();
  cue.midiNote = object.value("midiNote").toInt(-1);
  cue.preload = object.value("preload").toBool(false);
  return cue;
}

QJsonObject calibrationToJson(int screenIndex, const OutputCalibration& calibration) {
  QJsonObject object;
  object.insert("screen", screenIndex);
  object.insert("edgeBlendPx", calibration.edgeBlendPx);
  object.insert("keystoneHorizontal", calibration.keystoneHorizontal);
  object.insert("keystoneVertical", calibration.keystoneVertical);
  return object;
}

OutputCalibration calibrationFromJson(const QJsonObject& object) {
  OutputCalibration calibration;
  calibration.edgeBlendPx = object.value("edgeBlendPx").toInt(0);
  calibration.keystoneHorizontal = object.value("keystoneHorizontal").toInt(0);
  calibration.keystoneVertical = object.value("keystoneVertical").toInt(0);
  return calibration;
}

QJsonObject configToJson(const AppConfig& config) {
  QJsonObject object;
  object.insert("oscPort", config.oscPort);
  object.insert("transitionDurationMs", config.transitionDurationMs);
  object.insert("transitionStyle", transitionStyleToString(config.transitionStyle));
  object.insert("fallbackSlatePath", config.fallbackSlatePath);
  object.insert("midiEnabled", config.midiEnabled);
  object.insert("ndiEnabled", config.ndiEnabled);
  return object;
}

AppConfig configFromJson(const QJsonObject& object) {
  AppConfig config;
  config.oscPort = object.value("oscPort").toInt(9000);
  config.transitionDurationMs = object.value("transitionDurationMs").toInt(600);
  config.transitionStyle = transitionStyleFromString(object.value("transitionStyle").toString("fade"));
  config.fallbackSlatePath = object.value("fallbackSlatePath").toString();
  config.midiEnabled = object.value("midiEnabled").toBool(true);
  config.ndiEnabled = object.value("ndiEnabled").toBool(false);
  return config;
}

}  // namespace

bool ProjectSerializer::save(const QString& filePath, const ProjectData& project, QString* errorMessage) {
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (errorMessage != nullptr) {
      *errorMessage = QString("Failed to open project file for writing: %1").arg(file.errorString());
    }
    return false;
  }

  QJsonObject root;
  root.insert("version", 1);
  root.insert("config", configToJson(project.config));

  QJsonArray cues;
  for (const Cue& cue : project.cues) {
    cues.push_back(cueToJson(cue));
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

  ProjectData loaded;
  loaded.config = configFromJson(root.value("config").toObject());

  const QJsonArray cuesArray = root.value("cues").toArray();
  loaded.cues.reserve(cuesArray.size());
  for (const QJsonValue& value : cuesArray) {
    if (!value.isObject()) {
      continue;
    }
    loaded.cues.push_back(cueFromJson(value.toObject()));
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
