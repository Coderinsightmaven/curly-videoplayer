#pragma once

#include <QWidget>

class EdgeBlendOverlay : public QWidget {
  Q_OBJECT

 public:
  explicit EdgeBlendOverlay(QWidget* parent = nullptr);

  void setBlendSize(int pixels);
  int blendSize() const;

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  int blendSize_ = 0;
};
