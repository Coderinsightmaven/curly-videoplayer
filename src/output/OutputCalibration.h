#pragma once

struct OutputCalibration {
  int edgeBlendPx = 0;
  int keystoneHorizontal = 0;
  int keystoneVertical = 0;
  bool maskEnabled = false;
  int maskLeftPx = 0;
  int maskTopPx = 0;
  int maskRightPx = 0;
  int maskBottomPx = 0;
};
