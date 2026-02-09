#pragma once

#include <QWidget>

class EdgeBlendOverlay : public QWidget {
  Q_OBJECT

 public:
  explicit EdgeBlendOverlay(QWidget* parent = nullptr);

  void setBlendSize(int pixels);
  int blendSize() const;
  void setMask(bool enabled, int leftPx, int topPx, int rightPx, int bottomPx);

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  int blendSize_ = 0;
  bool maskEnabled_ = false;
  int maskLeftPx_ = 0;
  int maskTopPx_ = 0;
  int maskRightPx_ = 0;
  int maskBottomPx_ = 0;
};
