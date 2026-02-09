#pragma once

#include <QObject>
#include <QString>

class NdiBridge : public QObject {
  Q_OBJECT

 public:
  explicit NdiBridge(QObject* parent = nullptr);

  bool isAvailable() const;
  bool isEnabled() const;
  bool setEnabled(bool enabled);
  QString stateDescription() const;

 signals:
  void statusMessage(const QString& message);

 private:
  bool enabled_ = false;
};
