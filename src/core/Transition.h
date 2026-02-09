#pragma once

#include <QString>

// Transition modes used for taking cues to program output.
enum class TransitionStyle {
  Cut = 0,
  Fade = 1,
  DipToBlack = 2,
  WipeLeft = 3,
  DipToWhite = 4,
};

inline QString transitionStyleToString(TransitionStyle style) {
  switch (style) {
    case TransitionStyle::Cut:
      return "cut";
    case TransitionStyle::Fade:
      return "fade";
    case TransitionStyle::DipToBlack:
      return "dip_to_black";
    case TransitionStyle::WipeLeft:
      return "wipe_left";
    case TransitionStyle::DipToWhite:
      return "dip_to_white";
    default:
      return "cut";
  }
}

inline TransitionStyle transitionStyleFromString(const QString& value) {
  const QString normalized = value.trimmed().toLower();
  if (normalized == "fade") {
    return TransitionStyle::Fade;
  }
  if (normalized == "dip_to_black") {
    return TransitionStyle::DipToBlack;
  }
  if (normalized == "wipe_left") {
    return TransitionStyle::WipeLeft;
  }
  if (normalized == "dip_to_white") {
    return TransitionStyle::DipToWhite;
  }
  return TransitionStyle::Cut;
}
