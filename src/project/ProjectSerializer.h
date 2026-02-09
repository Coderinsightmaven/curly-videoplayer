#pragma once

#include <QMap>
#include <QString>
#include <QVector>

#include "core/AppConfig.h"
#include "core/Cue.h"
#include "output/OutputCalibration.h"

struct ProjectData {
  QVector<Cue> cues;
  QMap<int, OutputCalibration> calibrations;
  AppConfig config;
};

class ProjectSerializer {
 public:
  static bool save(const QString& filePath, const ProjectData& project, QString* errorMessage);
  static bool load(const QString& filePath, ProjectData* project, QString* errorMessage);
};
