#include "output/SyphonBridge.h"

SyphonBridge::SyphonBridge(QObject* parent) : QObject(parent) {}

bool SyphonBridge::isAvailable() const {
#ifdef HAVE_SYPHON_SDK
  return true;
#else
  return false;
#endif
}

bool SyphonBridge::isEnabled() const { return enabled_; }

bool SyphonBridge::setEnabled(bool enabled) {
#ifdef HAVE_SYPHON_SDK
  if (enabled == enabled_) {
    return true;
  }

  enabled_ = enabled;
  emit statusMessage(enabled_ ? "Syphon output bridge enabled." : "Syphon output bridge disabled.");
  return true;
#else
  if (enabled) {
    emit statusMessage("Syphon bridge unavailable in this build (SDK not found).");
  }
  enabled_ = false;
  return false;
#endif
}

QString SyphonBridge::stateDescription() const {
#ifdef HAVE_SYPHON_SDK
  return enabled_ ? "Syphon enabled" : "Syphon disabled";
#else
  return "Syphon unavailable";
#endif
}
