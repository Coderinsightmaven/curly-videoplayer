#include "output/DeckLinkBridge.h"

DeckLinkBridge::DeckLinkBridge(QObject* parent) : QObject(parent) {}

bool DeckLinkBridge::isAvailable() const {
#ifdef HAVE_DECKLINK_SDK
  return true;
#else
  return false;
#endif
}

bool DeckLinkBridge::isEnabled() const { return enabled_; }

bool DeckLinkBridge::setEnabled(bool enabled) {
#ifdef HAVE_DECKLINK_SDK
  if (enabled == enabled_) {
    return true;
  }

  enabled_ = enabled;
  emit statusMessage(enabled_ ? "DeckLink/SDI bridge enabled." : "DeckLink/SDI bridge disabled.");
  return true;
#else
  if (enabled) {
    emit statusMessage("DeckLink bridge unavailable in this build (SDK not found).");
  }
  enabled_ = false;
  return false;
#endif
}

QString DeckLinkBridge::stateDescription() const {
#ifdef HAVE_DECKLINK_SDK
  return enabled_ ? "DeckLink enabled" : "DeckLink disabled";
#else
  return "DeckLink unavailable";
#endif
}
