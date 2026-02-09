#pragma once

#include <QObject>
#include <QSize>
#include <QString>
#include <QVector>

class QScreen;

struct DisplayTarget {
  int index = 0;
  QString name;
  QSize size;
};

class DisplayManager : public QObject {
  Q_OBJECT

 public:
  explicit DisplayManager(QObject* parent = nullptr);

  QVector<DisplayTarget> displays() const;
  QScreen* screenAt(int index) const;

 public slots:
  void refresh();

 signals:
  void displaysChanged();

 private:
  QVector<DisplayTarget> displays_;
};
