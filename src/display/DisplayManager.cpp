#include "display/DisplayManager.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>

DisplayManager::DisplayManager(QObject* parent) : QObject(parent) {
  auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
  if (app != nullptr) {
    connect(app, &QGuiApplication::screenAdded, this, &DisplayManager::refresh);
    connect(app, &QGuiApplication::screenRemoved, this, &DisplayManager::refresh);
  }
  refresh();
}

QVector<DisplayTarget> DisplayManager::displays() const { return displays_; }

QScreen* DisplayManager::screenAt(int index) const {
  const QList<QScreen*> screens = QGuiApplication::screens();
  if (index < 0 || index >= screens.size()) {
    return nullptr;
  }
  return screens.at(index);
}

void DisplayManager::refresh() {
  const QList<QScreen*> screens = QGuiApplication::screens();
  QVector<DisplayTarget> updated;
  updated.reserve(screens.size());

  for (int i = 0; i < screens.size(); ++i) {
    QScreen* screen = screens.at(i);
    DisplayTarget target;
    target.index = i;
    target.name = screen->name();
    target.size = screen->size();
    updated.push_back(target);
  }

  displays_ = updated;
  emit displaysChanged();
}
