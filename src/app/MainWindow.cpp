#include "app/MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>
#include <QUuid>
#include <QWidget>
#include <QtGlobal>

#include "control/MidiInputService.h"
#include "control/OscServer.h"
#include "controllers/OutputRouter.h"
#include "controllers/PlaybackController.h"
#include "core/Cue.h"
#include "core/CueListModel.h"
#include "display/DisplayManager.h"
#include "ndi/NdiBridge.h"
#include "project/ProjectSerializer.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      cueModel_(new CueListModel(this)),
      displayManager_(new DisplayManager(this)),
      outputRouter_(new OutputRouter(displayManager_, this)),
      playbackController_(new PlaybackController(cueModel_, outputRouter_, this)),
      oscServer_(new OscServer(this)),
      midiService_(new MidiInputService(this)),
      ndiBridge_(new NdiBridge(this)),
      cueTable_(new QTableView(this)),
      screenCombo_(new QComboBox(this)),
      layerSpin_(new QSpinBox(this)),
      loopCheck_(new QCheckBox("Loop", this)),
      hotkeyEdit_(new QLineEdit(this)),
      timecodeEdit_(new QLineEdit(this)),
      midiNoteSpin_(new QSpinBox(this)),
      preloadCheck_(new QCheckBox("Preload", this)),
      transitionCombo_(new QComboBox(this)),
      transitionDurationSpin_(new QSpinBox(this)),
      calibrationScreenCombo_(new QComboBox(this)),
      edgeBlendSpin_(new QSpinBox(this)),
      keystoneXSpin_(new QSpinBox(this)),
      keystoneYSpin_(new QSpinBox(this)),
      slatePathEdit_(new QLineEdit(this)),
      oscPortSpin_(new QSpinBox(this)),
      midiEnableCheck_(new QCheckBox("Enable MIDI", this)),
      ndiEnableCheck_(new QCheckBox("Enable NDI", this)),
      statusLabel_(new QLabel(this)) {
  setWindowTitle("VideoPlayerForMe (v1.5 show control)");
  resize(1460, 900);

  cueTable_->setModel(cueModel_);
  cueTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  cueTable_->setSelectionMode(QAbstractItemView::SingleSelection);
  cueTable_->setAlternatingRowColors(true);
  cueTable_->horizontalHeader()->setStretchLastSection(true);
  cueTable_->verticalHeader()->setVisible(false);

  layerSpin_->setRange(0, 32);
  midiNoteSpin_->setRange(-1, 127);
  midiNoteSpin_->setSpecialValueText("None");

  transitionCombo_->addItem("Cut", static_cast<int>(TransitionStyle::Cut));
  transitionCombo_->addItem("Fade", static_cast<int>(TransitionStyle::Fade));
  transitionCombo_->addItem("Dip To Black", static_cast<int>(TransitionStyle::DipToBlack));
  transitionDurationSpin_->setRange(60, 6000);
  transitionDurationSpin_->setSuffix(" ms");
  transitionDurationSpin_->setValue(config_.transitionDurationMs);
  transitionCombo_->setCurrentIndex(1);

  edgeBlendSpin_->setRange(0, 400);
  edgeBlendSpin_->setSuffix(" px");
  keystoneXSpin_->setRange(-60, 60);
  keystoneYSpin_->setRange(-60, 60);

  oscPortSpin_->setRange(1024, 65535);
  oscPortSpin_->setValue(config_.oscPort);
  midiEnableCheck_->setChecked(config_.midiEnabled);
  ndiEnableCheck_->setChecked(config_.ndiEnabled);

  auto* addCueButton = new QPushButton("Add Cue", this);
  auto* removeCueButton = new QPushButton("Remove Cue", this);
  auto* previewButton = new QPushButton("Preview", this);
  auto* takeButton = new QPushButton("Take", this);
  auto* playLiveButton = new QPushButton("Play Live", this);
  auto* preloadButton = new QPushButton("Preload", this);
  auto* stopButton = new QPushButton("Stop Layer", this);
  auto* stopAllButton = new QPushButton("Stop All", this);

  auto* openProjectButton = new QPushButton("Open Project", this);
  auto* saveProjectButton = new QPushButton("Save", this);
  auto* saveAsProjectButton = new QPushButton("Save As", this);
  auto* showOutputsButton = new QPushButton("Show Outputs", this);
  auto* hideOutputsButton = new QPushButton("Hide Outputs", this);
  auto* showPreviewButton = new QPushButton("Show Preview", this);
  auto* hidePreviewButton = new QPushButton("Hide Preview", this);
  auto* refreshDisplaysButton = new QPushButton("Refresh Displays", this);
  auto* startOscButton = new QPushButton("Restart OSC", this);
  auto* browseSlateButton = new QPushButton("Browse Slate", this);

  auto* transportControls = new QHBoxLayout();
  transportControls->addWidget(addCueButton);
  transportControls->addWidget(removeCueButton);
  transportControls->addSpacing(12);
  transportControls->addWidget(previewButton);
  transportControls->addWidget(takeButton);
  transportControls->addWidget(playLiveButton);
  transportControls->addWidget(preloadButton);
  transportControls->addWidget(stopButton);
  transportControls->addWidget(stopAllButton);
  transportControls->addStretch();

  auto* sessionControls = new QHBoxLayout();
  sessionControls->addWidget(openProjectButton);
  sessionControls->addWidget(saveProjectButton);
  sessionControls->addWidget(saveAsProjectButton);
  sessionControls->addSpacing(12);
  sessionControls->addWidget(showOutputsButton);
  sessionControls->addWidget(hideOutputsButton);
  sessionControls->addWidget(showPreviewButton);
  sessionControls->addWidget(hidePreviewButton);
  sessionControls->addWidget(refreshDisplaysButton);
  sessionControls->addSpacing(12);
  sessionControls->addWidget(startOscButton);
  sessionControls->addStretch();

  auto* cueForm = new QFormLayout();
  cueForm->addRow("Target Screen", screenCombo_);
  cueForm->addRow("Layer", layerSpin_);
  cueForm->addRow("Loop", loopCheck_);
  cueForm->addRow("Preload", preloadCheck_);
  cueForm->addRow("Hotkey", hotkeyEdit_);
  cueForm->addRow("Timecode", timecodeEdit_);
  cueForm->addRow("MIDI Note", midiNoteSpin_);

  auto* cueGroup = new QGroupBox("Cue Properties", this);
  cueGroup->setLayout(cueForm);

  auto* transitionForm = new QFormLayout();
  transitionForm->addRow("Style", transitionCombo_);
  transitionForm->addRow("Duration", transitionDurationSpin_);

  auto* transitionGroup = new QGroupBox("Transition", this);
  transitionGroup->setLayout(transitionForm);

  auto* calibrationForm = new QFormLayout();
  calibrationForm->addRow("Screen", calibrationScreenCombo_);
  calibrationForm->addRow("Edge Blend", edgeBlendSpin_);
  calibrationForm->addRow("Keystone X", keystoneXSpin_);
  calibrationForm->addRow("Keystone Y", keystoneYSpin_);
  calibrationForm->addRow("Fallback Slate", slatePathEdit_);
  calibrationForm->addRow("", browseSlateButton);

  auto* calibrationGroup = new QGroupBox("Output Calibration", this);
  calibrationGroup->setLayout(calibrationForm);

  auto* controlForm = new QFormLayout();
  controlForm->addRow("OSC Port", oscPortSpin_);
  controlForm->addRow("MIDI", midiEnableCheck_);
  controlForm->addRow("NDI", ndiEnableCheck_);

  auto* controlGroup = new QGroupBox("Control Inputs", this);
  controlGroup->setLayout(controlForm);

  auto* rightPanel = new QWidget(this);
  auto* rightLayout = new QVBoxLayout(rightPanel);
  rightLayout->addWidget(cueGroup);
  rightLayout->addWidget(transitionGroup);
  rightLayout->addWidget(calibrationGroup);
  rightLayout->addWidget(controlGroup);
  rightLayout->addStretch();

  auto* splitter = new QSplitter(this);
  splitter->addWidget(cueTable_);
  splitter->addWidget(rightPanel);
  splitter->setStretchFactor(0, 4);
  splitter->setStretchFactor(1, 3);

  statusLabel_->setStyleSheet("color: #303030; font-weight: 500;");

  auto* root = new QWidget(this);
  auto* rootLayout = new QVBoxLayout(root);
  rootLayout->addLayout(transportControls);
  rootLayout->addLayout(sessionControls);
  rootLayout->addWidget(splitter, 1);
  rootLayout->addWidget(statusLabel_);
  setCentralWidget(root);

  connect(addCueButton, &QPushButton::clicked, this, &MainWindow::appendCueFromFile);
  connect(removeCueButton, &QPushButton::clicked, this, &MainWindow::removeSelectedCue);
  connect(previewButton, &QPushButton::clicked, this, &MainWindow::previewSelectedCue);
  connect(takeButton, &QPushButton::clicked, this, &MainWindow::takePreviewCue);
  connect(playLiveButton, &QPushButton::clicked, this, &MainWindow::playSelectedCue);
  connect(preloadButton, &QPushButton::clicked, this, &MainWindow::preloadSelectedCue);
  connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopSelectedCue);
  connect(stopAllButton, &QPushButton::clicked, this, &MainWindow::stopAllCues);

  connect(openProjectButton, &QPushButton::clicked, this, &MainWindow::openProject);
  connect(saveProjectButton, &QPushButton::clicked, this, &MainWindow::saveProject);
  connect(saveAsProjectButton, &QPushButton::clicked, this, &MainWindow::saveProjectAs);

  connect(showOutputsButton, &QPushButton::clicked, this, &MainWindow::showOutputs);
  connect(hideOutputsButton, &QPushButton::clicked, this, &MainWindow::hideOutputs);
  connect(showPreviewButton, &QPushButton::clicked, this, &MainWindow::showPreview);
  connect(hidePreviewButton, &QPushButton::clicked, this, &MainWindow::hidePreview);
  connect(refreshDisplaysButton, &QPushButton::clicked, displayManager_, &DisplayManager::refresh);

  connect(startOscButton, &QPushButton::clicked, this, &MainWindow::restartOscServer);
  connect(browseSlateButton, &QPushButton::clicked, this, &MainWindow::browseSlatePath);

  connect(cueTable_->selectionModel(), &QItemSelectionModel::currentChanged, this,
          [this](const QModelIndex&, const QModelIndex&) { syncEditorsFromSelection(); });

  connect(screenCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(layerSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(loopCheck_, &QCheckBox::toggled, this, [this](bool) { applyEditorsToSelection(); });
  connect(preloadCheck_, &QCheckBox::toggled, this, [this](bool) { applyEditorsToSelection(); });
  connect(hotkeyEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyEditorsToSelection);
  connect(timecodeEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyEditorsToSelection);
  connect(midiNoteSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyEditorsToSelection(); });

  connect(transitionCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
    config_.transitionStyle = selectedTransitionStyle();
  });
  connect(transitionDurationSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
    config_.transitionDurationMs = value;
  });

  connect(calibrationScreenCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { syncCalibrationEditors(); });
  connect(edgeBlendSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyCalibrationToScreen(); });
  connect(keystoneXSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyCalibrationToScreen(); });
  connect(keystoneYSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyCalibrationToScreen(); });
  connect(slatePathEdit_, &QLineEdit::editingFinished, this, [this]() {
    config_.fallbackSlatePath = slatePathEdit_->text().trimmed();
    outputRouter_->setFallbackSlatePath(config_.fallbackSlatePath);
  });

  connect(oscPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
    config_.oscPort = value;
  });
  connect(midiEnableCheck_, &QCheckBox::toggled, this, [this](bool) { applyControlConfig(); });
  connect(ndiEnableCheck_, &QCheckBox::toggled, this, [this](bool) { applyControlConfig(); });

  connect(displayManager_, &DisplayManager::displaysChanged, this, &MainWindow::refreshScreenChoices);

  connect(outputRouter_, &OutputRouter::routingError, this, &MainWindow::showStatus);
  connect(outputRouter_, &OutputRouter::routingStatus, this, &MainWindow::showStatus);
  connect(playbackController_, &PlaybackController::playbackError, this, &MainWindow::showStatus);
  connect(playbackController_, &PlaybackController::playbackStatus, this, &MainWindow::showStatus);

  connect(oscServer_, &OscServer::statusMessage, this, &MainWindow::showStatus);
  connect(oscServer_, &OscServer::playRowRequested, this, &MainWindow::handleExternalPlayRow);
  connect(oscServer_, &OscServer::previewRowRequested, this, &MainWindow::handleExternalPreviewRow);
  connect(oscServer_, &OscServer::preloadRowRequested, this, &MainWindow::handleExternalPreloadRow);
  connect(oscServer_, &OscServer::takeRequested, this, &MainWindow::handleExternalTake);
  connect(oscServer_, &OscServer::stopAllRequested, this, &MainWindow::stopAllCues);
  connect(oscServer_, &OscServer::timecodeReceived, this, &MainWindow::handleTimecode);

  connect(midiService_, &MidiInputService::statusMessage, this, &MainWindow::showStatus);
  connect(midiService_, &MidiInputService::cueNoteRequested, this, &MainWindow::handleExternalMidiNote);
  connect(midiService_, &MidiInputService::timecodeReceived, this, &MainWindow::handleTimecode);

  connect(ndiBridge_, &NdiBridge::statusMessage, this, &MainWindow::showStatus);

  connect(cueModel_, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex&, int, int) { rebuildCueHotkeys(); });
  connect(cueModel_, &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex&, int, int) { rebuildCueHotkeys(); });
  connect(cueModel_, &QAbstractItemModel::modelReset, this, [this]() { rebuildCueHotkeys(); });
  connect(cueModel_, &QAbstractItemModel::dataChanged, this,
          [this](const QModelIndex&, const QModelIndex&, const QList<int>&) { rebuildCueHotkeys(); });

  refreshScreenChoices();
  syncEditorsFromSelection();
  syncCalibrationEditors();
  outputRouter_->setFallbackSlatePath(config_.fallbackSlatePath);

  connectCoreShortcuts();
  rebuildCueHotkeys();
  restartOscServer();
  applyControlConfig();

  showStatus("Ready.");
}

void MainWindow::appendCueFromFile() {
  const QString filePath = QFileDialog::getOpenFileName(
      this, "Select media file", QString(),
      "Media files (*.mp4 *.mov *.mkv *.m4v *.avi *.webm *.mpg *.mpeg *.mxf *.ts *.wav *.mp3 *.png *.jpg *.jpeg *.gif);;All files (*)");
  if (filePath.isEmpty()) {
    return;
  }

  Cue cue;
  cue.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  cue.filePath = filePath;
  cue.name = QFileInfo(filePath).completeBaseName();
  if (cue.name.isEmpty()) {
    cue.name = QFileInfo(filePath).fileName();
  }

  cue.targetScreen = screenCombo_->currentData().toInt();
  cue.layer = layerSpin_->value();
  cue.loop = loopCheck_->isChecked();
  cue.preload = preloadCheck_->isChecked();
  cue.hotkey = hotkeyEdit_->text().trimmed();
  cue.timecodeTrigger = timecodeEdit_->text().trimmed();
  cue.midiNote = midiNoteSpin_->value();

  cueModel_->addCue(cue);

  const int row = cueModel_->rowCount() - 1;
  cueTable_->selectRow(row);

  if (cue.preload) {
    playbackController_->preloadCueAtRow(row);
  }

  showStatus(QString("Added cue '%1'.").arg(cue.name));
}

void MainWindow::removeSelectedCue() {
  const int row = selectedRow();
  if (!cueModel_->isValidRow(row)) {
    showStatus("Select a cue to remove.");
    return;
  }

  const QString name = cueModel_->cueAt(row).name;
  cueModel_->removeCue(row);
  showStatus(QString("Removed cue '%1'.").arg(name));
}

void MainWindow::previewSelectedCue() {
  const int row = selectedRow();
  if (!playbackController_->previewCueAtRow(row)) {
    return;
  }
  showPreview();
}

void MainWindow::takePreviewCue() {
  playbackController_->takePreviewCue(selectedTransitionStyle(), selectedTransitionDuration());
}

void MainWindow::playSelectedCue() {
  const int row = selectedRow();
  playbackController_->playCueAtRow(row, selectedTransitionStyle(), selectedTransitionDuration());
}

void MainWindow::preloadSelectedCue() {
  const int row = selectedRow();
  playbackController_->preloadCueAtRow(row);
}

void MainWindow::stopSelectedCue() {
  const int row = selectedRow();
  if (!cueModel_->isValidRow(row)) {
    showStatus("Select a cue to stop.");
    return;
  }

  const Cue cue = cueModel_->cueAt(row);
  playbackController_->stopCueAtRow(row);
  showStatus(QString("Stopped screen %1 layer %2.").arg(cue.targetScreen).arg(cue.layer));
}

void MainWindow::stopAllCues() {
  playbackController_->stopAll();
  showStatus("Stopped all outputs.");
}

void MainWindow::nextCue() { selectRowIfValid(selectedRow() + 1); }

void MainWindow::previousCue() { selectRowIfValid(selectedRow() - 1); }

void MainWindow::saveProject() {
  if (currentProjectPath_.isEmpty()) {
    saveProjectAs();
    return;
  }

  ProjectData project;
  project.cues = cueModel_->cues();
  project.calibrations = outputRouter_->calibrations();
  project.config = config_;

  QString error;
  if (!ProjectSerializer::save(currentProjectPath_, project, &error)) {
    showStatus(error);
    return;
  }

  showStatus(QString("Saved project: %1").arg(currentProjectPath_));
}

void MainWindow::saveProjectAs() {
  QString filePath = QFileDialog::getSaveFileName(this, "Save project", currentProjectPath_.isEmpty() ? "show.show" : currentProjectPath_,
                                                  "Show Project (*.show *.json)");
  if (filePath.isEmpty()) {
    return;
  }

  if (!filePath.endsWith(".show") && !filePath.endsWith(".json")) {
    filePath += ".show";
  }

  currentProjectPath_ = filePath;
  saveProject();
}

void MainWindow::openProject() {
  const QString filePath = QFileDialog::getOpenFileName(this, "Open project", QString(), "Show Project (*.show *.json)");
  if (filePath.isEmpty()) {
    return;
  }

  ProjectData project;
  QString error;
  if (!ProjectSerializer::load(filePath, &project, &error)) {
    showStatus(error);
    return;
  }

  currentProjectPath_ = filePath;
  applyLoadedProject(project);
  showStatus(QString("Loaded project: %1").arg(filePath));
}

void MainWindow::syncEditorsFromSelection() {
  const int row = selectedRow();
  updatingEditors_ = true;

  QSignalBlocker blockScreen(screenCombo_);
  QSignalBlocker blockLayer(layerSpin_);
  QSignalBlocker blockLoop(loopCheck_);
  QSignalBlocker blockPreload(preloadCheck_);
  QSignalBlocker blockHotkey(hotkeyEdit_);
  QSignalBlocker blockTimecode(timecodeEdit_);
  QSignalBlocker blockMidi(midiNoteSpin_);

  if (!cueModel_->isValidRow(row)) {
    layerSpin_->setValue(0);
    loopCheck_->setChecked(false);
    preloadCheck_->setChecked(false);
    hotkeyEdit_->setText({});
    timecodeEdit_->setText({});
    midiNoteSpin_->setValue(-1);
    if (screenCombo_->count() > 0) {
      screenCombo_->setCurrentIndex(0);
    }
    updatingEditors_ = false;
    return;
  }

  const Cue cue = cueModel_->cueAt(row);
  const int comboIndex = screenCombo_->findData(cue.targetScreen);
  if (comboIndex >= 0) {
    screenCombo_->setCurrentIndex(comboIndex);
  }

  layerSpin_->setValue(cue.layer);
  loopCheck_->setChecked(cue.loop);
  preloadCheck_->setChecked(cue.preload);
  hotkeyEdit_->setText(cue.hotkey);
  timecodeEdit_->setText(cue.timecodeTrigger);
  midiNoteSpin_->setValue(cue.midiNote < 0 ? -1 : cue.midiNote);

  updatingEditors_ = false;
}

void MainWindow::applyEditorsToSelection() {
  if (updatingEditors_) {
    return;
  }

  const int row = selectedRow();
  if (!cueModel_->isValidRow(row)) {
    return;
  }

  Cue cue = cueModel_->cueAt(row);
  cue.targetScreen = screenCombo_->currentData().toInt();
  cue.layer = layerSpin_->value();
  cue.loop = loopCheck_->isChecked();
  cue.preload = preloadCheck_->isChecked();
  cue.hotkey = hotkeyEdit_->text().trimmed();
  cue.timecodeTrigger = timecodeEdit_->text().trimmed();
  cue.midiNote = midiNoteSpin_->value();

  cueModel_->updateCue(row, cue);
}

void MainWindow::refreshScreenChoices() {
  const int currentScreen = screenCombo_->currentData().toInt();
  const int currentCalibrationScreen = calibrationScreenCombo_->currentData().toInt();

  QSignalBlocker blockScreen(screenCombo_);
  QSignalBlocker blockCalibration(calibrationScreenCombo_);

  screenCombo_->clear();
  calibrationScreenCombo_->clear();

  const QVector<DisplayTarget> targets = displayManager_->displays();
  for (const DisplayTarget& target : targets) {
    const QString label = QString("%1 | %2x%3").arg(target.name).arg(target.size.width()).arg(target.size.height());
    screenCombo_->addItem(label, target.index);
    calibrationScreenCombo_->addItem(label, target.index);
  }

  if (screenCombo_->count() == 0) {
    screenCombo_->addItem("Primary Screen", 0);
    calibrationScreenCombo_->addItem("Primary Screen", 0);
  }

  const int restoredScreenIndex = screenCombo_->findData(currentScreen);
  if (restoredScreenIndex >= 0) {
    screenCombo_->setCurrentIndex(restoredScreenIndex);
  }

  const int restoredCalibrationIndex = calibrationScreenCombo_->findData(currentCalibrationScreen);
  if (restoredCalibrationIndex >= 0) {
    calibrationScreenCombo_->setCurrentIndex(restoredCalibrationIndex);
  }

  syncCalibrationEditors();
}

void MainWindow::syncCalibrationEditors() {
  if (updatingCalibration_) {
    return;
  }

  updatingCalibration_ = true;
  QSignalBlocker blockBlend(edgeBlendSpin_);
  QSignalBlocker blockKx(keystoneXSpin_);
  QSignalBlocker blockKy(keystoneYSpin_);

  const int screenIndex = calibrationScreenCombo_->currentData().toInt();
  const OutputCalibration calibration = outputRouter_->calibrationForScreen(screenIndex);

  edgeBlendSpin_->setValue(calibration.edgeBlendPx);
  keystoneXSpin_->setValue(calibration.keystoneHorizontal);
  keystoneYSpin_->setValue(calibration.keystoneVertical);

  updatingCalibration_ = false;
}

void MainWindow::applyCalibrationToScreen() {
  if (updatingCalibration_) {
    return;
  }

  const int screenIndex = calibrationScreenCombo_->currentData().toInt();
  OutputCalibration calibration;
  calibration.edgeBlendPx = edgeBlendSpin_->value();
  calibration.keystoneHorizontal = keystoneXSpin_->value();
  calibration.keystoneVertical = keystoneYSpin_->value();

  outputRouter_->setOutputCalibration(screenIndex, calibration);
}

void MainWindow::browseSlatePath() {
  const QString filePath = QFileDialog::getOpenFileName(this, "Select fallback slate", config_.fallbackSlatePath,
                                                        "Images (*.png *.jpg *.jpeg *.bmp *.webp);;All files (*)");
  if (filePath.isEmpty()) {
    return;
  }

  slatePathEdit_->setText(filePath);
  config_.fallbackSlatePath = filePath;
  outputRouter_->setFallbackSlatePath(config_.fallbackSlatePath);
  showStatus("Updated fallback slate.");
}

void MainWindow::restartOscServer() {
  config_.oscPort = oscPortSpin_->value();
  oscServer_->start(static_cast<quint16>(config_.oscPort));
}

void MainWindow::applyControlConfig() {
  config_.midiEnabled = midiEnableCheck_->isChecked();
  config_.ndiEnabled = ndiEnableCheck_->isChecked();

  if (config_.midiEnabled) {
    if (!midiService_->start()) {
      QSignalBlocker blockMidi(midiEnableCheck_);
      midiEnableCheck_->setChecked(false);
      config_.midiEnabled = false;
      midiService_->stop();
    }
  } else {
    midiService_->stop();
  }

  if (config_.ndiEnabled && !ndiBridge_->setEnabled(true)) {
    ndiEnableCheck_->setChecked(false);
    config_.ndiEnabled = false;
  }

  if (!config_.ndiEnabled) {
    ndiBridge_->setEnabled(false);
  }
}

void MainWindow::handleExternalPlayRow(int row) {
  const int resolvedRow = resolveCueRowFromIndex(row);
  selectRowIfValid(resolvedRow);
  playbackController_->playCueAtRow(resolvedRow, selectedTransitionStyle(), selectedTransitionDuration());
}

void MainWindow::handleExternalPreviewRow(int row) {
  const int resolvedRow = resolveCueRowFromIndex(row);
  selectRowIfValid(resolvedRow);
  playbackController_->previewCueAtRow(resolvedRow);
}

void MainWindow::handleExternalPreloadRow(int row) {
  const int resolvedRow = resolveCueRowFromIndex(row);
  selectRowIfValid(resolvedRow);
  playbackController_->preloadCueAtRow(resolvedRow);
}

void MainWindow::handleExternalMidiNote(int note) {
  const int resolvedRow = resolveCueRowFromMidiNote(note);
  selectRowIfValid(resolvedRow);
  playbackController_->playCueAtRow(resolvedRow, selectedTransitionStyle(), selectedTransitionDuration());
}

void MainWindow::handleExternalTake() { playbackController_->takePreviewCue(selectedTransitionStyle(), selectedTransitionDuration()); }

void MainWindow::handleTimecode(const QString& timecode) {
  playbackController_->triggerByTimecode(timecode.trimmed(), selectedTransitionStyle(), selectedTransitionDuration());
}

void MainWindow::rebuildCueHotkeys() {
  for (auto it = cueHotkeys_.begin(); it != cueHotkeys_.end(); ++it) {
    delete it.value();
  }
  cueHotkeys_.clear();

  const QVector<Cue> cues = cueModel_->cues();
  for (const Cue& cue : cues) {
    const QString key = cue.hotkey.trimmed();
    if (cue.id.isEmpty() || key.isEmpty()) {
      continue;
    }

    const QKeySequence sequence(key);
    if (sequence.isEmpty()) {
      continue;
    }

    auto* shortcut = new QShortcut(sequence, this);
    shortcut->setContext(Qt::ApplicationShortcut);
    connect(shortcut, &QShortcut::activated, this,
            [this, cueId = cue.id]() { playbackController_->playCueById(cueId, selectedTransitionStyle(), selectedTransitionDuration(), false); });

    cueHotkeys_.insert(cue.id, shortcut);
  }
}

void MainWindow::showOutputs() {
  outputRouter_->showOutputs();
  showStatus("Outputs shown.");
}

void MainWindow::hideOutputs() {
  outputRouter_->hideOutputs();
  showStatus("Outputs hidden.");
}

void MainWindow::showPreview() {
  outputRouter_->showPreview();
  showStatus("Preview shown.");
}

void MainWindow::hidePreview() {
  outputRouter_->hidePreview();
  showStatus("Preview hidden.");
}

void MainWindow::showStatus(const QString& message) { statusLabel_->setText(message); }

int MainWindow::selectedRow() const {
  if (cueTable_->selectionModel() == nullptr) {
    return -1;
  }
  return cueTable_->selectionModel()->currentIndex().row();
}

int MainWindow::resolveCueRowFromIndex(int row) const {
  if (cueModel_->isValidRow(row)) {
    return row;
  }
  return -1;
}

int MainWindow::resolveCueRowFromMidiNote(int note) const {
  const QVector<Cue> cues = cueModel_->cues();
  for (int i = 0; i < cues.size(); ++i) {
    if (cues.at(i).midiNote == note) {
      return i;
    }
  }

  return -1;
}

void MainWindow::selectRowIfValid(int row) {
  if (!cueModel_->isValidRow(row)) {
    return;
  }
  cueTable_->selectRow(row);
}

TransitionStyle MainWindow::selectedTransitionStyle() const {
  return static_cast<TransitionStyle>(transitionCombo_->currentData().toInt());
}

int MainWindow::selectedTransitionDuration() const { return transitionDurationSpin_->value(); }

void MainWindow::applyLoadedProject(const ProjectData& project) {
  cueModel_->setCues(project.cues);

  outputRouter_->clearCalibrations();
  for (auto it = project.calibrations.constBegin(); it != project.calibrations.constEnd(); ++it) {
    outputRouter_->setOutputCalibration(it.key(), it.value());
  }

  config_ = project.config;

  {
    QSignalBlocker blockStyle(transitionCombo_);
    QSignalBlocker blockDuration(transitionDurationSpin_);
    QSignalBlocker blockOsc(oscPortSpin_);
    QSignalBlocker blockMidi(midiEnableCheck_);
    QSignalBlocker blockNdi(ndiEnableCheck_);

    const int styleIndex = transitionCombo_->findData(static_cast<int>(config_.transitionStyle));
    if (styleIndex >= 0) {
      transitionCombo_->setCurrentIndex(styleIndex);
    }
    transitionDurationSpin_->setValue(config_.transitionDurationMs);
    oscPortSpin_->setValue(config_.oscPort);
    midiEnableCheck_->setChecked(config_.midiEnabled);
    ndiEnableCheck_->setChecked(config_.ndiEnabled);
  }

  slatePathEdit_->setText(config_.fallbackSlatePath);
  outputRouter_->setFallbackSlatePath(config_.fallbackSlatePath);

  syncEditorsFromSelection();
  syncCalibrationEditors();
  rebuildCueHotkeys();
  restartOscServer();
  applyControlConfig();
}

void MainWindow::connectCoreShortcuts() {
  auto* previewShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
  connect(previewShortcut, &QShortcut::activated, this, &MainWindow::previewSelectedCue);

  auto* takeShortcut1 = new QShortcut(QKeySequence(Qt::Key_Return), this);
  connect(takeShortcut1, &QShortcut::activated, this, &MainWindow::takePreviewCue);

  auto* takeShortcut2 = new QShortcut(QKeySequence(Qt::Key_Enter), this);
  connect(takeShortcut2, &QShortcut::activated, this, &MainWindow::takePreviewCue);

  auto* liveShortcut = new QShortcut(QKeySequence(Qt::Key_G), this);
  connect(liveShortcut, &QShortcut::activated, this, &MainWindow::playSelectedCue);

  auto* preloadShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this);
  connect(preloadShortcut, &QShortcut::activated, this, &MainWindow::preloadSelectedCue);

  auto* stopShortcut = new QShortcut(QKeySequence(Qt::Key_S), this);
  connect(stopShortcut, &QShortcut::activated, this, &MainWindow::stopSelectedCue);

  auto* stopAllShortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_S), this);
  connect(stopAllShortcut, &QShortcut::activated, this, &MainWindow::stopAllCues);

  auto* nextShortcut = new QShortcut(QKeySequence(Qt::Key_Down), this);
  connect(nextShortcut, &QShortcut::activated, this, &MainWindow::nextCue);

  auto* prevShortcut = new QShortcut(QKeySequence(Qt::Key_Up), this);
  connect(prevShortcut, &QShortcut::activated, this, &MainWindow::previousCue);

  auto* openShortcut = new QShortcut(QKeySequence::Open, this);
  connect(openShortcut, &QShortcut::activated, this, &MainWindow::openProject);

  auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
  connect(saveShortcut, &QShortcut::activated, this, &MainWindow::saveProject);

  auto* saveAsShortcut = new QShortcut(QKeySequence("Ctrl+Shift+S"), this);
  connect(saveAsShortcut, &QShortcut::activated, this, &MainWindow::saveProjectAs);

  for (int i = 0; i < 9; ++i) {
    auto* previewRowShortcut = new QShortcut(QKeySequence(QString::number(i + 1)), this);
    connect(previewRowShortcut, &QShortcut::activated, this, [this, i]() { handleExternalPreviewRow(i); });

    auto* liveRowShortcut = new QShortcut(QKeySequence(QString("Shift+%1").arg(i + 1)), this);
    connect(liveRowShortcut, &QShortcut::activated, this, [this, i]() { handleExternalPlayRow(i); });
  }
}
