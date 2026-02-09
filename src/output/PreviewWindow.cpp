#include "output/PreviewWindow.h"

#include <QLabel>
#include <QVBoxLayout>

#include "output/LayerSurface.h"

PreviewWindow::PreviewWindow(QWidget* parent) : QWidget(parent), surface_(new LayerSurface(this)) {
  setWindowTitle("Preview");
  setMinimumSize(640, 360);
  resize(960, 540);
  setStyleSheet("background: #101010;");

  auto* badge = new QLabel("PREVIEW", this);
  badge->setStyleSheet(
      "background: #f4c542; color: black; font-weight: 700; padding: 4px 10px; border-radius: 4px;");

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->addWidget(badge, 0, Qt::AlignLeft);
  layout->addWidget(surface_, 1);

  connect(surface_, &LayerSurface::playbackError, this, &PreviewWindow::previewError);
}

bool PreviewWindow::previewCue(const Cue& cue) {
  const bool ok = surface_->playCue(cue);
  if (ok) {
    lastCue_ = cue;
  }
  return ok;
}

bool PreviewWindow::preloadCue(const Cue& cue) {
  const bool ok = surface_->preloadCue(cue);
  if (ok) {
    lastCue_ = cue;
  }
  return ok;
}

void PreviewWindow::stopAll() { surface_->stopAll(); }

Cue PreviewWindow::lastCue() const { return lastCue_; }
