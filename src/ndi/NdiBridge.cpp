#include "ndi/NdiBridge.h"

#ifdef HAVE_NDI_SDK
#include <Processing.NDI.Lib.h>
#endif

NdiBridge::NdiBridge(QObject* parent) : QObject(parent) {}

bool NdiBridge::isAvailable() const {
#ifdef HAVE_NDI_SDK
  return true;
#else
  return false;
#endif
}

bool NdiBridge::isEnabled() const { return enabled_; }

bool NdiBridge::setEnabled(bool enabled) {
#ifdef HAVE_NDI_SDK
  if (enabled == enabled_) {
    return true;
  }

  if (enabled) {
    if (!NDIlib_initialize()) {
      emit statusMessage("NDI SDK is available but failed to initialize.");
      return false;
    }
    enabled_ = true;
    emit statusMessage("NDI initialized.");
    return true;
  }

  NDIlib_destroy();
  enabled_ = false;
  emit statusMessage("NDI stopped.");
  return true;
#else
  if (enabled) {
    emit statusMessage("NDI is unavailable in this build (SDK not found).");
  }
  enabled_ = false;
  return false;
#endif
}

QString NdiBridge::stateDescription() const {
#ifdef HAVE_NDI_SDK
  return enabled_ ? "NDI enabled" : "NDI disabled";
#else
  return "NDI unavailable";
#endif
}
