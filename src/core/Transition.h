#pragma once

#include <QString>

// Transition modes used for taking cues to program output.
enum class TransitionStyle {
  Cut = 0,
  Fade = 1,
  DipToBlack = 2,
};

inline QString transitionStyleToString(TransitionStyle style) {
  switch (style) {
    case TransitionStyle::Cut:
      return "cut";
    case TransitionStyle::Fade:
      return "fade";
    case TransitionStyle::DipToBlack:
      return "dip_to_black";
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
  return TransitionStyle::Cut;
}
