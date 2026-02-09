#include "app/MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QTableView>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QUuid>
#include <QWidget>
#include <QtGlobal>

#include "control/ArtnetInputService.h"
#include "control/FailoverSyncService.h"
#include "control/MidiInputService.h"
#include "control/OscServer.h"
#include "controllers/OutputRouter.h"
#include "controllers/PlaybackController.h"
#include "core/Cue.h"
#include "core/CueListModel.h"
#include "display/DisplayManager.h"
#include "ndi/NdiBridge.h"
#include "output/DeckLinkBridge.h"
#include "output/SyphonBridge.h"
#include "project/ProjectSerializer.h"

namespace {

QMap<QString, QString> parseFilterPresets(const QString& encoded) {
  QMap<QString, QString> presets;
  const QStringList pairs = encoded.split(';', Qt::SkipEmptyParts);
  for (const QString& pair : pairs) {
    const int sep = pair.indexOf('=');
    if (sep <= 0 || sep >= pair.size() - 1) {
      continue;
    }

    const QString name = pair.left(sep).trimmed();
    const QString filter = pair.mid(sep + 1).trimmed();
    if (name.isEmpty() || filter.isEmpty()) {
      continue;
    }

    presets.insert(name, filter);
  }
  return presets;
}

QString serializeFilterPresets(const QMap<QString, QString>& presets) {
  QStringList pairs;
  pairs.reserve(presets.size());
  for (auto it = presets.constBegin(); it != presets.constEnd(); ++it) {
    pairs.push_back(QString("%1=%2").arg(it.key(), it.value()));
  }
  return pairs.join(';');
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      cueModel_(new CueListModel(this)),
      displayManager_(new DisplayManager(this)),
      outputRouter_(new OutputRouter(displayManager_, this)),
      playbackController_(new PlaybackController(cueModel_, outputRouter_, this)),
      oscServer_(new OscServer(this)),
      artnetService_(new ArtnetInputService(this)),
      failoverSync_(new FailoverSyncService(this)),
      midiService_(new MidiInputService(this)),
      ndiBridge_(new NdiBridge(this)),
      syphonBridge_(new SyphonBridge(this)),
      deckLinkBridge_(new DeckLinkBridge(this)),
      cueTable_(new QTableView(this)),
      screenCombo_(new QComboBox(this)),
      targetSetCombo_(new QComboBox(this)),
      layerSpin_(new QSpinBox(this)),
      loopCheck_(new QCheckBox("Loop", this)),
      hotkeyEdit_(new QLineEdit(this)),
      timecodeEdit_(new QLineEdit(this)),
      midiNoteSpin_(new QSpinBox(this)),
      dmxChannelSpin_(new QSpinBox(this)),
      dmxValueSpin_(new QSpinBox(this)),
      preloadCheck_(new QCheckBox("Preload", this)),
      filterPresetCombo_(new QComboBox(this)),
      cueFilterEdit_(new QLineEdit(this)),
      liveInputCheck_(new QCheckBox("Live Input", this)),
      liveInputUrlEdit_(new QLineEdit(this)),
      cueTransitionOverrideCheck_(new QCheckBox("Use Cue Transition", this)),
      cueTransitionCombo_(new QComboBox(this)),
      cueTransitionDurationSpin_(new QSpinBox(this)),
      playlistIdEdit_(new QLineEdit(this)),
      playlistAdvanceCheck_(new QCheckBox("Playlist Auto Advance", this)),
      playlistLoopCheck_(new QCheckBox("Playlist Loop", this)),
      playlistDelaySpin_(new QSpinBox(this)),
      autoStopSpin_(new QSpinBox(this)),
      autoFollowCheck_(new QCheckBox("Auto Follow", this)),
      followCueRowSpin_(new QSpinBox(this)),
      followDelaySpin_(new QSpinBox(this)),
      transitionCombo_(new QComboBox(this)),
      transitionDurationSpin_(new QSpinBox(this)),
      calibrationScreenCombo_(new QComboBox(this)),
      edgeBlendSpin_(new QSpinBox(this)),
      keystoneXSpin_(new QSpinBox(this)),
      keystoneYSpin_(new QSpinBox(this)),
      maskEnableCheck_(new QCheckBox("Enable Output Mask", this)),
      maskLeftSpin_(new QSpinBox(this)),
      maskTopSpin_(new QSpinBox(this)),
      maskRightSpin_(new QSpinBox(this)),
      maskBottomSpin_(new QSpinBox(this)),
      slatePathEdit_(new QLineEdit(this)),
      oscPortSpin_(new QSpinBox(this)),
      relativePathCheck_(new QCheckBox("Use Relative Media Paths", this)),
      midiEnableCheck_(new QCheckBox("Enable MIDI", this)),
      ndiEnableCheck_(new QCheckBox("Enable NDI", this)),
      syphonEnableCheck_(new QCheckBox("Enable Syphon", this)),
      deckLinkEnableCheck_(new QCheckBox("Enable SDI (DeckLink)", this)),
      filterPresetsEdit_(new QLineEdit(this)),
      artnetEnableCheck_(new QCheckBox("Enable Art-Net DMX", this)),
      artnetPortSpin_(new QSpinBox(this)),
      artnetUniverseSpin_(new QSpinBox(this)),
      backupTriggerCheck_(new QCheckBox("Enable Backup Trigger", this)),
      backupUrlEdit_(new QLineEdit(this)),
      backupTokenEdit_(new QLineEdit(this)),
      failoverSyncCheck_(new QCheckBox("Enable Failover Sync", this)),
      failoverHostEdit_(new QLineEdit(this)),
      failoverPeerPortSpin_(new QSpinBox(this)),
      failoverListenPortSpin_(new QSpinBox(this)),
      failoverKeyEdit_(new QLineEdit(this)),
      statusLabel_(new QLabel(this)),
      backupNetwork_(new QNetworkAccessManager(this)) {
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
  dmxChannelSpin_->setRange(-1, 512);
  dmxChannelSpin_->setSpecialValueText("None");
  dmxValueSpin_->setRange(0, 255);
  dmxValueSpin_->setValue(255);
  filterPresetCombo_->setMinimumContentsLength(18);
  cueFilterEdit_->setPlaceholderText("Optional mpv vf chain, e.g. hflip");
  liveInputUrlEdit_->setPlaceholderText("e.g. av://dshow:video=Camera");
  playlistIdEdit_->setPlaceholderText("Optional playlist/group id");
  playlistDelaySpin_->setRange(0, 300000);
  playlistDelaySpin_->setSuffix(" ms");
  autoStopSpin_->setRange(0, 600000);
  autoStopSpin_->setSuffix(" ms");
  autoStopSpin_->setSpecialValueText("Disabled");
  followCueRowSpin_->setRange(-1, 9999);
  followCueRowSpin_->setSpecialValueText("Next Cue");
  followDelaySpin_->setRange(0, 300000);
  followDelaySpin_->setSuffix(" ms");

  transitionCombo_->addItem("Cut", static_cast<int>(TransitionStyle::Cut));
  transitionCombo_->addItem("Fade", static_cast<int>(TransitionStyle::Fade));
  transitionCombo_->addItem("Dip To Black", static_cast<int>(TransitionStyle::DipToBlack));
  transitionCombo_->addItem("Wipe Left", static_cast<int>(TransitionStyle::WipeLeft));
  transitionCombo_->addItem("Dip To White", static_cast<int>(TransitionStyle::DipToWhite));
  transitionDurationSpin_->setRange(60, 6000);
  transitionDurationSpin_->setSuffix(" ms");
  transitionDurationSpin_->setValue(config_.transitionDurationMs);
  transitionCombo_->setCurrentIndex(1);

  cueTransitionCombo_->addItem("Cut", static_cast<int>(TransitionStyle::Cut));
  cueTransitionCombo_->addItem("Fade", static_cast<int>(TransitionStyle::Fade));
  cueTransitionCombo_->addItem("Dip To Black", static_cast<int>(TransitionStyle::DipToBlack));
  cueTransitionCombo_->addItem("Wipe Left", static_cast<int>(TransitionStyle::WipeLeft));
  cueTransitionCombo_->addItem("Dip To White", static_cast<int>(TransitionStyle::DipToWhite));
  cueTransitionDurationSpin_->setRange(60, 6000);
  cueTransitionDurationSpin_->setSuffix(" ms");
  cueTransitionDurationSpin_->setValue(config_.transitionDurationMs);

  edgeBlendSpin_->setRange(0, 400);
  edgeBlendSpin_->setSuffix(" px");
  keystoneXSpin_->setRange(-60, 60);
  keystoneYSpin_->setRange(-60, 60);
  maskLeftSpin_->setRange(0, 1000);
  maskTopSpin_->setRange(0, 1000);
  maskRightSpin_->setRange(0, 1000);
  maskBottomSpin_->setRange(0, 1000);
  maskLeftSpin_->setSuffix(" px");
  maskTopSpin_->setSuffix(" px");
  maskRightSpin_->setSuffix(" px");
  maskBottomSpin_->setSuffix(" px");

  oscPortSpin_->setRange(1024, 65535);
  oscPortSpin_->setValue(config_.oscPort);
  relativePathCheck_->setChecked(config_.useRelativeMediaPaths);
  midiEnableCheck_->setChecked(config_.midiEnabled);
  ndiEnableCheck_->setChecked(config_.ndiEnabled);
  syphonEnableCheck_->setChecked(config_.syphonEnabled);
  deckLinkEnableCheck_->setChecked(config_.deckLinkEnabled);
  filterPresetsEdit_->setText(serializeFilterPresets(config_.filterPresets));
  filterPresetsEdit_->setPlaceholderText("name=vf_chain;name2=vf_chain");
  artnetEnableCheck_->setChecked(config_.artnetEnabled);
  artnetPortSpin_->setRange(1024, 65535);
  artnetPortSpin_->setValue(config_.artnetPort);
  artnetUniverseSpin_->setRange(0, 32767);
  artnetUniverseSpin_->setValue(config_.artnetUniverse);
  backupTriggerCheck_->setChecked(config_.backupTriggerEnabled);
  backupUrlEdit_->setText(config_.backupTriggerUrl);
  backupUrlEdit_->setPlaceholderText("https://backup.local/api/trigger");
  backupTokenEdit_->setText(config_.backupTriggerToken);
  backupTokenEdit_->setPlaceholderText("Bearer token (optional)");
  backupTokenEdit_->setEchoMode(QLineEdit::Password);
  failoverSyncCheck_->setChecked(config_.failoverSyncEnabled);
  failoverHostEdit_->setText(config_.failoverPeerHost);
  failoverHostEdit_->setPlaceholderText("Backup node host/IP");
  failoverPeerPortSpin_->setRange(1024, 65535);
  failoverPeerPortSpin_->setValue(config_.failoverPeerPort);
  failoverListenPortSpin_->setRange(1024, 65535);
  failoverListenPortSpin_->setValue(config_.failoverListenPort);
  failoverKeyEdit_->setText(config_.failoverSharedKey);
  failoverKeyEdit_->setPlaceholderText("Shared key");
  failoverKeyEdit_->setEchoMode(QLineEdit::Password);

  auto* addCueButton = new QPushButton("Add Cue", this);
  auto* addPatternButton = new QPushButton("Add Test Pattern", this);
  auto* removeCueButton = new QPushButton("Remove Cue", this);
  auto* previewButton = new QPushButton("Preview", this);
  auto* takeButton = new QPushButton("Take", this);
  auto* playLiveButton = new QPushButton("Play Live", this);
  auto* preloadButton = new QPushButton("Preload", this);
  auto* stopButton = new QPushButton("Stop Layer", this);
  auto* stopAllButton = new QPushButton("Stop All", this);

  auto* openProjectButton = new QPushButton("Open Project", this);
  auto* relinkButton = new QPushButton("Relink Missing", this);
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
  transportControls->addWidget(addPatternButton);
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
  sessionControls->addWidget(relinkButton);
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
  cueForm->addRow("Target Set", targetSetCombo_);
  cueForm->addRow("Layer", layerSpin_);
  cueForm->addRow("Loop", loopCheck_);
  cueForm->addRow("Preload", preloadCheck_);
  cueForm->addRow("Live Input", liveInputCheck_);
  cueForm->addRow("Live Source", liveInputUrlEdit_);
  cueForm->addRow("Filter Preset", filterPresetCombo_);
  cueForm->addRow("Media Filter", cueFilterEdit_);
  cueForm->addRow("Cue Transition", cueTransitionOverrideCheck_);
  cueForm->addRow("Cue Style", cueTransitionCombo_);
  cueForm->addRow("Cue Duration", cueTransitionDurationSpin_);
  cueForm->addRow("Playlist ID", playlistIdEdit_);
  cueForm->addRow("Playlist Auto", playlistAdvanceCheck_);
  cueForm->addRow("Playlist Loop", playlistLoopCheck_);
  cueForm->addRow("Playlist Delay", playlistDelaySpin_);
  cueForm->addRow("Auto Follow", autoFollowCheck_);
  cueForm->addRow("Follow Row", followCueRowSpin_);
  cueForm->addRow("Follow Delay", followDelaySpin_);
  cueForm->addRow("Auto Stop", autoStopSpin_);
  cueForm->addRow("Hotkey", hotkeyEdit_);
  cueForm->addRow("Timecode", timecodeEdit_);
  cueForm->addRow("MIDI Note", midiNoteSpin_);
  cueForm->addRow("DMX Channel", dmxChannelSpin_);
  cueForm->addRow("DMX Min Value", dmxValueSpin_);

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
  calibrationForm->addRow("Mask Enabled", maskEnableCheck_);
  calibrationForm->addRow("Mask Left", maskLeftSpin_);
  calibrationForm->addRow("Mask Top", maskTopSpin_);
  calibrationForm->addRow("Mask Right", maskRightSpin_);
  calibrationForm->addRow("Mask Bottom", maskBottomSpin_);
  calibrationForm->addRow("Fallback Slate", slatePathEdit_);
  calibrationForm->addRow("", browseSlateButton);

  auto* calibrationGroup = new QGroupBox("Output Calibration", this);
  calibrationGroup->setLayout(calibrationForm);

  auto* controlForm = new QFormLayout();
  controlForm->addRow("OSC Port", oscPortSpin_);
  controlForm->addRow("Path Mode", relativePathCheck_);
  controlForm->addRow("MIDI", midiEnableCheck_);
  controlForm->addRow("NDI", ndiEnableCheck_);
  controlForm->addRow("Syphon", syphonEnableCheck_);
  controlForm->addRow("SDI", deckLinkEnableCheck_);
  controlForm->addRow("Filter Presets", filterPresetsEdit_);
  controlForm->addRow("Art-Net", artnetEnableCheck_);
  controlForm->addRow("Art-Net Port", artnetPortSpin_);
  controlForm->addRow("Art-Net Universe", artnetUniverseSpin_);
  controlForm->addRow("Backup Trigger", backupTriggerCheck_);
  controlForm->addRow("Backup URL", backupUrlEdit_);
  controlForm->addRow("Backup Token", backupTokenEdit_);
  controlForm->addRow("Failover Sync", failoverSyncCheck_);
  controlForm->addRow("Failover Host", failoverHostEdit_);
  controlForm->addRow("Failover Peer Port", failoverPeerPortSpin_);
  controlForm->addRow("Failover Listen Port", failoverListenPortSpin_);
  controlForm->addRow("Failover Key", failoverKeyEdit_);

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
  connect(addPatternButton, &QPushButton::clicked, this, &MainWindow::appendTestPatternCue);
  connect(removeCueButton, &QPushButton::clicked, this, &MainWindow::removeSelectedCue);
  connect(previewButton, &QPushButton::clicked, this, &MainWindow::previewSelectedCue);
  connect(takeButton, &QPushButton::clicked, this, &MainWindow::takePreviewCue);
  connect(playLiveButton, &QPushButton::clicked, this, &MainWindow::playSelectedCue);
  connect(preloadButton, &QPushButton::clicked, this, &MainWindow::preloadSelectedCue);
  connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopSelectedCue);
  connect(stopAllButton, &QPushButton::clicked, this, &MainWindow::stopAllCues);

  connect(openProjectButton, &QPushButton::clicked, this, &MainWindow::openProject);
  connect(relinkButton, &QPushButton::clicked, this, &MainWindow::relinkMissingMedia);
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
  connect(targetSetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(layerSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(loopCheck_, &QCheckBox::toggled, this, [this](bool) { applyEditorsToSelection(); });
  connect(preloadCheck_, &QCheckBox::toggled, this, [this](bool) { applyEditorsToSelection(); });
  connect(liveInputCheck_, &QCheckBox::toggled, this, [this](bool) { applyEditorsToSelection(); });
  connect(liveInputUrlEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyEditorsToSelection);
  connect(filterPresetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(cueFilterEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyEditorsToSelection);
  connect(cueTransitionOverrideCheck_, &QCheckBox::toggled, this, [this](bool) { applyEditorsToSelection(); });
  connect(cueTransitionCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(cueTransitionDurationSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(playlistIdEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyEditorsToSelection);
  connect(playlistAdvanceCheck_, &QCheckBox::toggled, this, [this](bool) { applyEditorsToSelection(); });
  connect(playlistLoopCheck_, &QCheckBox::toggled, this, [this](bool) { applyEditorsToSelection(); });
  connect(playlistDelaySpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(autoFollowCheck_, &QCheckBox::toggled, this, [this](bool) { applyEditorsToSelection(); });
  connect(followCueRowSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(followDelaySpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(autoStopSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { applyEditorsToSelection(); });
  connect(hotkeyEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyEditorsToSelection);
  connect(timecodeEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyEditorsToSelection);
  connect(midiNoteSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(dmxChannelSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyEditorsToSelection(); });
  connect(dmxValueSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
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
  connect(maskEnableCheck_, &QCheckBox::toggled, this, [this](bool) { applyCalibrationToScreen(); });
  connect(maskLeftSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { applyCalibrationToScreen(); });
  connect(maskTopSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { applyCalibrationToScreen(); });
  connect(maskRightSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { applyCalibrationToScreen(); });
  connect(maskBottomSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { applyCalibrationToScreen(); });
  connect(slatePathEdit_, &QLineEdit::editingFinished, this, [this]() {
    config_.fallbackSlatePath = slatePathEdit_->text().trimmed();
    outputRouter_->setFallbackSlatePath(config_.fallbackSlatePath);
  });

  connect(oscPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
    config_.oscPort = value;
  });
  connect(relativePathCheck_, &QCheckBox::toggled, this, [this](bool checked) { config_.useRelativeMediaPaths = checked; });
  connect(midiEnableCheck_, &QCheckBox::toggled, this, [this](bool) { applyControlConfig(); });
  connect(ndiEnableCheck_, &QCheckBox::toggled, this, [this](bool) { applyControlConfig(); });
  connect(syphonEnableCheck_, &QCheckBox::toggled, this, [this](bool) { applyControlConfig(); });
  connect(deckLinkEnableCheck_, &QCheckBox::toggled, this, [this](bool) { applyControlConfig(); });
  connect(filterPresetsEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyControlConfig);
  connect(artnetEnableCheck_, &QCheckBox::toggled, this, [this](bool) { applyControlConfig(); });
  connect(artnetPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { applyControlConfig(); });
  connect(artnetUniverseSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { applyControlConfig(); });
  connect(backupTriggerCheck_, &QCheckBox::toggled, this, [this](bool) { applyControlConfig(); });
  connect(backupUrlEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyControlConfig);
  connect(backupTokenEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyControlConfig);
  connect(failoverSyncCheck_, &QCheckBox::toggled, this, [this](bool) { applyControlConfig(); });
  connect(failoverHostEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyControlConfig);
  connect(failoverPeerPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { applyControlConfig(); });
  connect(failoverListenPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int) { applyControlConfig(); });
  connect(failoverKeyEdit_, &QLineEdit::editingFinished, this, &MainWindow::applyControlConfig);

  connect(displayManager_, &DisplayManager::displaysChanged, this, &MainWindow::refreshScreenChoices);

  connect(outputRouter_, &OutputRouter::routingError, this, &MainWindow::showStatus);
  connect(outputRouter_, &OutputRouter::routingStatus, this, &MainWindow::showStatus);
  connect(playbackController_, &PlaybackController::playbackError, this, &MainWindow::showStatus);
  connect(playbackController_, &PlaybackController::playbackStatus, this, &MainWindow::showStatus);
  connect(playbackController_, &PlaybackController::cueWentLive, this, &MainWindow::forwardCueToBackup);

  connect(oscServer_, &OscServer::statusMessage, this, &MainWindow::showStatus);
  connect(oscServer_, &OscServer::playRowRequested, this, &MainWindow::handleExternalPlayRow);
  connect(oscServer_, &OscServer::previewRowRequested, this, &MainWindow::handleExternalPreviewRow);
  connect(oscServer_, &OscServer::preloadRowRequested, this, &MainWindow::handleExternalPreloadRow);
  connect(oscServer_, &OscServer::takeRequested, this, &MainWindow::handleExternalTake);
  connect(oscServer_, &OscServer::stopAllRequested, this, &MainWindow::stopAllCues);
  connect(oscServer_, &OscServer::timecodeReceived, this, &MainWindow::handleTimecode);
  connect(oscServer_, &OscServer::dmxValueReceived, this, &MainWindow::handleExternalDmx);
  connect(oscServer_, &OscServer::overlayTextReceived, this, &MainWindow::handleExternalOverlayText);

  connect(artnetService_, &ArtnetInputService::statusMessage, this, &MainWindow::showStatus);
  connect(artnetService_, &ArtnetInputService::dmxValueReceived, this, &MainWindow::handleExternalDmx);

  connect(failoverSync_, &FailoverSyncService::statusMessage, this, &MainWindow::showStatus);
  connect(failoverSync_, &FailoverSyncService::remoteCueLiveRequested, this, &MainWindow::handleRemoteCueLive);
  connect(failoverSync_, &FailoverSyncService::remoteStopAllRequested, this, &MainWindow::handleRemoteStopAll);
  connect(failoverSync_, &FailoverSyncService::remoteOverlayTextReceived, this, &MainWindow::handleRemoteOverlayText);

  connect(midiService_, &MidiInputService::statusMessage, this, &MainWindow::showStatus);
  connect(midiService_, &MidiInputService::cueNoteRequested, this, &MainWindow::handleExternalMidiNote);
  connect(midiService_, &MidiInputService::timecodeReceived, this, &MainWindow::handleTimecode);

  connect(ndiBridge_, &NdiBridge::statusMessage, this, &MainWindow::showStatus);
  connect(syphonBridge_, &SyphonBridge::statusMessage, this, &MainWindow::showStatus);
  connect(deckLinkBridge_, &DeckLinkBridge::statusMessage, this, &MainWindow::showStatus);

  connect(cueModel_, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex&, int, int) { rebuildCueHotkeys(); });
  connect(cueModel_, &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex&, int, int) { rebuildCueHotkeys(); });
  connect(cueModel_, &QAbstractItemModel::modelReset, this, [this]() { rebuildCueHotkeys(); });
  connect(cueModel_, &QAbstractItemModel::dataChanged, this,
          [this](const QModelIndex&, const QModelIndex&, const QList<int>&) { rebuildCueHotkeys(); });

  refreshScreenChoices();
  refreshFilterPresetChoices();
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
  cue.targetSetId = targetSetCombo_->currentData().toString();
  cue.layer = layerSpin_->value();
  cue.loop = loopCheck_->isChecked();
  cue.preload = preloadCheck_->isChecked();
  cue.isLiveInput = liveInputCheck_->isChecked();
  cue.liveInputUrl = liveInputUrlEdit_->text().trimmed();
  cue.filterPresetId = filterPresetCombo_->currentData().toString();
  cue.videoFilter = cueFilterEdit_->text().trimmed();
  cue.useTransitionOverride = cueTransitionOverrideCheck_->isChecked();
  cue.transitionStyle = static_cast<TransitionStyle>(cueTransitionCombo_->currentData().toInt());
  cue.transitionDurationMs = cueTransitionDurationSpin_->value();
  cue.playlistId = playlistIdEdit_->text().trimmed();
  cue.playlistAutoAdvance = playlistAdvanceCheck_->isChecked();
  cue.playlistLoop = playlistLoopCheck_->isChecked();
  cue.playlistAdvanceDelayMs = playlistDelaySpin_->value();
  cue.autoFollow = autoFollowCheck_->isChecked();
  cue.followCueRow = followCueRowSpin_->value();
  cue.followDelayMs = followDelaySpin_->value();
  cue.autoStopMs = autoStopSpin_->value();
  cue.hotkey = hotkeyEdit_->text().trimmed();
  cue.timecodeTrigger = timecodeEdit_->text().trimmed();
  cue.midiNote = midiNoteSpin_->value();
  cue.dmxChannel = dmxChannelSpin_->value();
  cue.dmxValue = dmxValueSpin_->value();

  cueModel_->addCue(cue);

  const int row = cueModel_->rowCount() - 1;
  cueTable_->selectRow(row);

  if (cue.preload) {
    playbackController_->preloadCueAtRow(row);
  }

  showStatus(QString("Added cue '%1'.").arg(cue.name));
}

void MainWindow::appendTestPatternCue() {
  const QString patternPath = ensureColorBarsPatternPath();
  if (patternPath.isEmpty()) {
    showStatus("Failed to create color bars test pattern.");
    return;
  }

  Cue cue;
  cue.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  cue.filePath = patternPath;
  cue.name = QString("Test Pattern - Color Bars");
  cue.targetScreen = screenCombo_->currentData().toInt();
  cue.targetSetId = targetSetCombo_->currentData().toString();
  cue.layer = layerSpin_->value();
  cue.loop = loopCheck_->isChecked();
  cue.preload = preloadCheck_->isChecked();
  cue.isLiveInput = false;
  cue.liveInputUrl.clear();
  cue.filterPresetId = filterPresetCombo_->currentData().toString();
  cue.videoFilter = cueFilterEdit_->text().trimmed();
  cue.useTransitionOverride = cueTransitionOverrideCheck_->isChecked();
  cue.transitionStyle = static_cast<TransitionStyle>(cueTransitionCombo_->currentData().toInt());
  cue.transitionDurationMs = cueTransitionDurationSpin_->value();
  cue.playlistId = playlistIdEdit_->text().trimmed();
  cue.playlistAutoAdvance = playlistAdvanceCheck_->isChecked();
  cue.playlistLoop = playlistLoopCheck_->isChecked();
  cue.playlistAdvanceDelayMs = playlistDelaySpin_->value();
  cue.autoFollow = autoFollowCheck_->isChecked();
  cue.followCueRow = followCueRowSpin_->value();
  cue.followDelayMs = followDelaySpin_->value();
  cue.autoStopMs = autoStopSpin_->value();
  cue.hotkey = hotkeyEdit_->text().trimmed();
  cue.timecodeTrigger = timecodeEdit_->text().trimmed();
  cue.midiNote = midiNoteSpin_->value();
  cue.dmxChannel = dmxChannelSpin_->value();
  cue.dmxValue = dmxValueSpin_->value();

  cueModel_->addCue(cue);
  const int row = cueModel_->rowCount() - 1;
  cueTable_->selectRow(row);
  if (cue.preload) {
    playbackController_->preloadCueAtRow(row);
  }

  showStatus("Added color bars test pattern cue.");
}

void MainWindow::relinkMissingMedia() {
  int missingCount = 0;
  int relinkedCount = 0;
  const QString startDir = currentProjectPath_.isEmpty() ? QDir::homePath() : QFileInfo(currentProjectPath_).absolutePath();

  for (int row = 0; row < cueModel_->rowCount(); ++row) {
    Cue cue = cueModel_->cueAt(row);
    if (cue.filePath.trimmed().isEmpty() || cue.filePath.contains("://")) {
      continue;
    }

    if (QFileInfo::exists(cue.filePath)) {
      continue;
    }

    ++missingCount;
    const QString replacement = QFileDialog::getOpenFileName(
        this, QString("Relink media for cue '%1'").arg(cue.name), startDir,
        "Media files (*.mp4 *.mov *.mkv *.m4v *.avi *.webm *.mpg *.mpeg *.mxf *.ts *.wav *.mp3 *.png *.jpg *.jpeg *.gif);;All files (*)");
    if (replacement.isEmpty()) {
      continue;
    }

    cue.filePath = replacement;
    cue.isLiveInput = false;
    cue.liveInputUrl.clear();
    cueModel_->updateCue(row, cue);
    ++relinkedCount;
  }

  showStatus(QString("Relink complete: %1/%2 missing cues relinked.").arg(relinkedCount).arg(missingCount));
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

  if (config_.failoverSyncEnabled) {
    if (suppressFailoverStopPublish_) {
      suppressFailoverStopPublish_ = false;
    } else {
      failoverSync_->publishStopAll();
    }
  } else {
    suppressFailoverStopPublish_ = false;
  }

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

  int missingCount = 0;
  for (const Cue& cue : cueModel_->cues()) {
    if (cue.filePath.isEmpty() || cue.filePath.contains("://")) {
      continue;
    }
    if (!QFileInfo::exists(cue.filePath)) {
      ++missingCount;
    }
  }

  if (missingCount > 0) {
    showStatus(QString("Loaded project: %1 (%2 missing media file(s), run Relink Missing)").arg(filePath).arg(missingCount));
  } else {
    showStatus(QString("Loaded project: %1").arg(filePath));
  }
}

void MainWindow::syncEditorsFromSelection() {
  const int row = selectedRow();
  updatingEditors_ = true;

  QSignalBlocker blockScreen(screenCombo_);
  QSignalBlocker blockTargetSet(targetSetCombo_);
  QSignalBlocker blockLayer(layerSpin_);
  QSignalBlocker blockLoop(loopCheck_);
  QSignalBlocker blockPreload(preloadCheck_);
  QSignalBlocker blockLiveInput(liveInputCheck_);
  QSignalBlocker blockLiveUrl(liveInputUrlEdit_);
  QSignalBlocker blockPreset(filterPresetCombo_);
  QSignalBlocker blockFilter(cueFilterEdit_);
  QSignalBlocker blockCueTransitionOverride(cueTransitionOverrideCheck_);
  QSignalBlocker blockCueTransition(cueTransitionCombo_);
  QSignalBlocker blockCueDuration(cueTransitionDurationSpin_);
  QSignalBlocker blockPlaylistId(playlistIdEdit_);
  QSignalBlocker blockPlaylistAdvance(playlistAdvanceCheck_);
  QSignalBlocker blockPlaylistLoop(playlistLoopCheck_);
  QSignalBlocker blockPlaylistDelay(playlistDelaySpin_);
  QSignalBlocker blockAutoFollow(autoFollowCheck_);
  QSignalBlocker blockFollowRow(followCueRowSpin_);
  QSignalBlocker blockFollowDelay(followDelaySpin_);
  QSignalBlocker blockAutoStop(autoStopSpin_);
  QSignalBlocker blockHotkey(hotkeyEdit_);
  QSignalBlocker blockTimecode(timecodeEdit_);
  QSignalBlocker blockMidi(midiNoteSpin_);
  QSignalBlocker blockDmxChannel(dmxChannelSpin_);
  QSignalBlocker blockDmxValue(dmxValueSpin_);

  if (!cueModel_->isValidRow(row)) {
    if (targetSetCombo_->count() > 0) {
      targetSetCombo_->setCurrentIndex(0);
    }
    layerSpin_->setValue(0);
    loopCheck_->setChecked(false);
    preloadCheck_->setChecked(false);
    liveInputCheck_->setChecked(false);
    liveInputUrlEdit_->setText({});
    filterPresetCombo_->setCurrentIndex(0);
    cueFilterEdit_->setText({});
    cueTransitionOverrideCheck_->setChecked(false);
    cueTransitionCombo_->setCurrentIndex(1);
    cueTransitionDurationSpin_->setValue(config_.transitionDurationMs);
    playlistIdEdit_->setText({});
    playlistAdvanceCheck_->setChecked(false);
    playlistLoopCheck_->setChecked(false);
    playlistDelaySpin_->setValue(0);
    autoFollowCheck_->setChecked(false);
    followCueRowSpin_->setValue(-1);
    followDelaySpin_->setValue(0);
    autoStopSpin_->setValue(0);
    hotkeyEdit_->setText({});
    timecodeEdit_->setText({});
    midiNoteSpin_->setValue(-1);
    dmxChannelSpin_->setValue(-1);
    dmxValueSpin_->setValue(255);
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
  const int targetSetIndex = targetSetCombo_->findData(cue.targetSetId);
  if (targetSetIndex >= 0) {
    targetSetCombo_->setCurrentIndex(targetSetIndex);
  } else if (targetSetCombo_->count() > 0) {
    targetSetCombo_->setCurrentIndex(0);
  }

  layerSpin_->setValue(cue.layer);
  loopCheck_->setChecked(cue.loop);
  preloadCheck_->setChecked(cue.preload);
  liveInputCheck_->setChecked(cue.isLiveInput);
  liveInputUrlEdit_->setText(cue.liveInputUrl);
  const int presetIndex = filterPresetCombo_->findData(cue.filterPresetId);
  if (presetIndex >= 0) {
    filterPresetCombo_->setCurrentIndex(presetIndex);
  } else {
    filterPresetCombo_->setCurrentIndex(0);
  }
  cueFilterEdit_->setText(cue.videoFilter);
  cueTransitionOverrideCheck_->setChecked(cue.useTransitionOverride);
  const int cueTransitionIndex = cueTransitionCombo_->findData(static_cast<int>(cue.transitionStyle));
  if (cueTransitionIndex >= 0) {
    cueTransitionCombo_->setCurrentIndex(cueTransitionIndex);
  }
  cueTransitionDurationSpin_->setValue(cue.transitionDurationMs);
  playlistIdEdit_->setText(cue.playlistId);
  playlistAdvanceCheck_->setChecked(cue.playlistAutoAdvance);
  playlistLoopCheck_->setChecked(cue.playlistLoop);
  playlistDelaySpin_->setValue(cue.playlistAdvanceDelayMs);
  autoFollowCheck_->setChecked(cue.autoFollow);
  followCueRowSpin_->setValue(cue.followCueRow);
  followDelaySpin_->setValue(cue.followDelayMs);
  autoStopSpin_->setValue(cue.autoStopMs);
  hotkeyEdit_->setText(cue.hotkey);
  timecodeEdit_->setText(cue.timecodeTrigger);
  midiNoteSpin_->setValue(cue.midiNote < 0 ? -1 : cue.midiNote);
  dmxChannelSpin_->setValue(cue.dmxChannel < 0 ? -1 : cue.dmxChannel);
  dmxValueSpin_->setValue(cue.dmxValue);

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
  cue.targetSetId = targetSetCombo_->currentData().toString();
  cue.layer = layerSpin_->value();
  cue.loop = loopCheck_->isChecked();
  cue.preload = preloadCheck_->isChecked();
  cue.isLiveInput = liveInputCheck_->isChecked();
  cue.liveInputUrl = liveInputUrlEdit_->text().trimmed();
  cue.filterPresetId = filterPresetCombo_->currentData().toString();
  cue.videoFilter = cueFilterEdit_->text().trimmed();
  cue.useTransitionOverride = cueTransitionOverrideCheck_->isChecked();
  cue.transitionStyle = static_cast<TransitionStyle>(cueTransitionCombo_->currentData().toInt());
  cue.transitionDurationMs = cueTransitionDurationSpin_->value();
  cue.playlistId = playlistIdEdit_->text().trimmed();
  cue.playlistAutoAdvance = playlistAdvanceCheck_->isChecked();
  cue.playlistLoop = playlistLoopCheck_->isChecked();
  cue.playlistAdvanceDelayMs = playlistDelaySpin_->value();
  cue.autoFollow = autoFollowCheck_->isChecked();
  cue.followCueRow = followCueRowSpin_->value();
  cue.followDelayMs = followDelaySpin_->value();
  cue.autoStopMs = autoStopSpin_->value();
  cue.hotkey = hotkeyEdit_->text().trimmed();
  cue.timecodeTrigger = timecodeEdit_->text().trimmed();
  cue.midiNote = midiNoteSpin_->value();
  cue.dmxChannel = dmxChannelSpin_->value();
  cue.dmxValue = dmxValueSpin_->value();

  cueModel_->updateCue(row, cue);
}

void MainWindow::refreshScreenChoices() {
  const int currentScreen = screenCombo_->currentData().toInt();
  const int currentCalibrationScreen = calibrationScreenCombo_->currentData().toInt();
  const QString currentTargetSet = targetSetCombo_->currentData().toString();

  QSignalBlocker blockScreen(screenCombo_);
  QSignalBlocker blockCalibration(calibrationScreenCombo_);
  QSignalBlocker blockTargetSet(targetSetCombo_);

  screenCombo_->clear();
  calibrationScreenCombo_->clear();
  targetSetCombo_->clear();
  targetSetCombo_->addItem("Selected Screen", QString());

  const QVector<DisplayTarget> targets = displayManager_->displays();
  for (const DisplayTarget& target : targets) {
    const QString label = QString("%1 | %2x%3").arg(target.name).arg(target.size.width()).arg(target.size.height());
    screenCombo_->addItem(label, target.index);
    calibrationScreenCombo_->addItem(label, target.index);
    targetSetCombo_->addItem(QString("Screen %1 (%2)").arg(target.index).arg(target.name),
                             QString("screen-%1").arg(target.index));
  }

  if (screenCombo_->count() == 0) {
    screenCombo_->addItem("Primary Screen", 0);
    calibrationScreenCombo_->addItem("Primary Screen", 0);
    targetSetCombo_->addItem("Primary Screen", "screen-0");
  } else if (targets.size() > 1) {
    targetSetCombo_->addItem("All Screens", "all-screens");
  }

  const int restoredScreenIndex = screenCombo_->findData(currentScreen);
  if (restoredScreenIndex >= 0) {
    screenCombo_->setCurrentIndex(restoredScreenIndex);
  }

  const int restoredCalibrationIndex = calibrationScreenCombo_->findData(currentCalibrationScreen);
  if (restoredCalibrationIndex >= 0) {
    calibrationScreenCombo_->setCurrentIndex(restoredCalibrationIndex);
  }

  const int restoredTargetSetIndex = targetSetCombo_->findData(currentTargetSet);
  if (restoredTargetSetIndex >= 0) {
    targetSetCombo_->setCurrentIndex(restoredTargetSetIndex);
  } else if (targetSetCombo_->count() > 0) {
    targetSetCombo_->setCurrentIndex(0);
  }

  syncCalibrationEditors();
}

void MainWindow::refreshFilterPresetChoices() {
  const QString currentPreset = filterPresetCombo_->currentData().toString();

  QSignalBlocker blocker(filterPresetCombo_);
  filterPresetCombo_->clear();
  filterPresetCombo_->addItem("None", QString());

  for (auto it = config_.filterPresets.constBegin(); it != config_.filterPresets.constEnd(); ++it) {
    filterPresetCombo_->addItem(it.key(), it.key());
  }

  const int presetIndex = filterPresetCombo_->findData(currentPreset);
  filterPresetCombo_->setCurrentIndex(presetIndex >= 0 ? presetIndex : 0);
}

void MainWindow::syncCalibrationEditors() {
  if (updatingCalibration_) {
    return;
  }

  updatingCalibration_ = true;
  QSignalBlocker blockBlend(edgeBlendSpin_);
  QSignalBlocker blockKx(keystoneXSpin_);
  QSignalBlocker blockKy(keystoneYSpin_);
  QSignalBlocker blockMaskEnabled(maskEnableCheck_);
  QSignalBlocker blockMaskLeft(maskLeftSpin_);
  QSignalBlocker blockMaskTop(maskTopSpin_);
  QSignalBlocker blockMaskRight(maskRightSpin_);
  QSignalBlocker blockMaskBottom(maskBottomSpin_);

  const int screenIndex = calibrationScreenCombo_->currentData().toInt();
  const OutputCalibration calibration = outputRouter_->calibrationForScreen(screenIndex);

  edgeBlendSpin_->setValue(calibration.edgeBlendPx);
  keystoneXSpin_->setValue(calibration.keystoneHorizontal);
  keystoneYSpin_->setValue(calibration.keystoneVertical);
  maskEnableCheck_->setChecked(calibration.maskEnabled);
  maskLeftSpin_->setValue(calibration.maskLeftPx);
  maskTopSpin_->setValue(calibration.maskTopPx);
  maskRightSpin_->setValue(calibration.maskRightPx);
  maskBottomSpin_->setValue(calibration.maskBottomPx);

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
  calibration.maskEnabled = maskEnableCheck_->isChecked();
  calibration.maskLeftPx = maskLeftSpin_->value();
  calibration.maskTopPx = maskTopSpin_->value();
  calibration.maskRightPx = maskRightSpin_->value();
  calibration.maskBottomPx = maskBottomSpin_->value();

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
  config_.useRelativeMediaPaths = relativePathCheck_->isChecked();
  config_.midiEnabled = midiEnableCheck_->isChecked();
  config_.ndiEnabled = ndiEnableCheck_->isChecked();
  config_.syphonEnabled = syphonEnableCheck_->isChecked();
  config_.deckLinkEnabled = deckLinkEnableCheck_->isChecked();
  config_.filterPresets = parseFilterPresets(filterPresetsEdit_->text());
  config_.artnetEnabled = artnetEnableCheck_->isChecked();
  config_.artnetPort = artnetPortSpin_->value();
  config_.artnetUniverse = artnetUniverseSpin_->value();
  config_.backupTriggerEnabled = backupTriggerCheck_->isChecked();
  config_.backupTriggerUrl = backupUrlEdit_->text().trimmed();
  config_.backupTriggerToken = backupTokenEdit_->text().trimmed();
  config_.failoverSyncEnabled = failoverSyncCheck_->isChecked();
  config_.failoverPeerHost = failoverHostEdit_->text().trimmed();
  config_.failoverPeerPort = failoverPeerPortSpin_->value();
  config_.failoverListenPort = failoverListenPortSpin_->value();
  config_.failoverSharedKey = failoverKeyEdit_->text().trimmed();

  refreshFilterPresetChoices();
  outputRouter_->setFilterPresets(config_.filterPresets);

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

  if (config_.syphonEnabled && !syphonBridge_->setEnabled(true)) {
    QSignalBlocker blockSyphon(syphonEnableCheck_);
    syphonEnableCheck_->setChecked(false);
    config_.syphonEnabled = false;
  }
  if (!config_.syphonEnabled) {
    syphonBridge_->setEnabled(false);
  }

  if (config_.deckLinkEnabled && !deckLinkBridge_->setEnabled(true)) {
    QSignalBlocker blockDeckLink(deckLinkEnableCheck_);
    deckLinkEnableCheck_->setChecked(false);
    config_.deckLinkEnabled = false;
  }
  if (!config_.deckLinkEnabled) {
    deckLinkBridge_->setEnabled(false);
  }

  if (config_.artnetEnabled) {
    if (!artnetService_->start(static_cast<quint16>(config_.artnetPort), config_.artnetUniverse)) {
      QSignalBlocker blockArtnet(artnetEnableCheck_);
      artnetEnableCheck_->setChecked(false);
      config_.artnetEnabled = false;
      artnetService_->stop();
    }
  } else {
    artnetService_->stop();
  }

  failoverSync_->setPeer(config_.failoverPeerHost, static_cast<quint16>(config_.failoverPeerPort));
  if (config_.failoverSyncEnabled) {
    if (!failoverSync_->start(static_cast<quint16>(config_.failoverListenPort), config_.failoverSharedKey)) {
      QSignalBlocker blockFailover(failoverSyncCheck_);
      failoverSyncCheck_->setChecked(false);
      config_.failoverSyncEnabled = false;
      failoverSync_->stop();
    } else {
      failoverSync_->setPeer(config_.failoverPeerHost, static_cast<quint16>(config_.failoverPeerPort));
    }
  } else {
    failoverSync_->stop();
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

void MainWindow::handleExternalDmx(int channel, int value) {
  const int resolvedRow = resolveCueRowFromDmx(channel, value);
  selectRowIfValid(resolvedRow);
  playbackController_->playCueAtRow(resolvedRow, selectedTransitionStyle(), selectedTransitionDuration());
}

void MainWindow::handleExternalOverlayText(const QString& text) {
  const QString trimmed = text.trimmed();
  outputRouter_->setOverlayText(trimmed);

  if (!config_.failoverSyncEnabled) {
    suppressFailoverOverlayPublish_ = false;
    return;
  }

  if (suppressFailoverOverlayPublish_) {
    suppressFailoverOverlayPublish_ = false;
    return;
  }

  failoverSync_->publishOverlayText(trimmed);
}

void MainWindow::handleExternalTake() { playbackController_->takePreviewCue(selectedTransitionStyle(), selectedTransitionDuration()); }

void MainWindow::handleTimecode(const QString& timecode) {
  playbackController_->triggerByTimecode(timecode.trimmed(), selectedTransitionStyle(), selectedTransitionDuration());
}

void MainWindow::handleRemoteCueLive(const QString& cueId) {
  selectRowIfValid(cueModel_->rowForCueId(cueId));
  suppressFailoverCuePublish_ = true;
  const bool ok = playbackController_->playCueById(cueId, selectedTransitionStyle(), selectedTransitionDuration(), false);
  if (!ok) {
    suppressFailoverCuePublish_ = false;
  }
}

void MainWindow::handleRemoteStopAll() {
  suppressFailoverStopPublish_ = true;
  stopAllCues();
}

void MainWindow::handleRemoteOverlayText(const QString& text) {
  suppressFailoverOverlayPublish_ = true;
  handleExternalOverlayText(text);
}

void MainWindow::forwardCueToBackup(const Cue& cue) {
  if (config_.backupTriggerEnabled && !config_.backupTriggerUrl.trimmed().isEmpty()) {
    const QUrl url(config_.backupTriggerUrl.trimmed());
    if (!url.isValid()) {
      showStatus("Backup trigger URL is invalid.");
    } else {
      QNetworkRequest request(url);
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
      if (!config_.backupTriggerToken.trimmed().isEmpty()) {
        request.setRawHeader("Authorization", QByteArray("Bearer ") + config_.backupTriggerToken.toUtf8());
      }

      QJsonObject payload;
      payload.insert("cueId", cue.id);
      payload.insert("cueName", cue.name);
      payload.insert("targetScreen", cue.targetScreen);
      payload.insert("targetSetId", cue.targetSetId);
      payload.insert("layer", cue.layer);
      payload.insert("timestampUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

      QNetworkReply* reply = backupNetwork_->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
      QTimer::singleShot(qMax(300, config_.backupTriggerTimeoutMs), reply, [reply]() {
        if (reply->isRunning()) {
          reply->abort();
        }
      });

      connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
          showStatus(QString("Backup trigger failed: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
      });
    }
  }

  if (!config_.failoverSyncEnabled) {
    suppressFailoverCuePublish_ = false;
    return;
  }

  if (suppressFailoverCuePublish_) {
    suppressFailoverCuePublish_ = false;
    return;
  }

  failoverSync_->publishCueLive(cue);
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

int MainWindow::resolveCueRowFromDmx(int channel, int value) const {
  if (channel < 0) {
    return -1;
  }

  const QVector<Cue> cues = cueModel_->cues();
  for (int i = 0; i < cues.size(); ++i) {
    const Cue& cue = cues.at(i);
    if (cue.dmxChannel < 0) {
      continue;
    }
    if (cue.dmxChannel == channel && value >= cue.dmxValue) {
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

QString MainWindow::ensureColorBarsPatternPath() const {
  const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (appData.isEmpty()) {
    return {};
  }

  const QDir base(appData);
  if (!base.exists() && !QDir().mkpath(base.path())) {
    return {};
  }

  const QString patternsDirPath = base.filePath("patterns");
  if (!QDir().mkpath(patternsDirPath)) {
    return {};
  }

  const QString patternPath = QDir(patternsDirPath).filePath("color-bars-1920x1080.png");
  if (QFileInfo::exists(patternPath)) {
    return patternPath;
  }

  QImage image(1920, 1080, QImage::Format_RGB32);
  image.fill(Qt::black);

  QPainter painter(&image);
  const QVector<QColor> colors = {
      QColor(255, 255, 255), QColor(255, 255, 0),  QColor(0, 255, 255), QColor(0, 255, 0),
      QColor(255, 0, 255),   QColor(255, 0, 0),    QColor(0, 0, 255),   QColor(32, 32, 32)};
  const int barWidth = image.width() / colors.size();
  for (int i = 0; i < colors.size(); ++i) {
    painter.fillRect(i * barWidth, 0, barWidth, image.height(), colors.at(i));
  }

  painter.setPen(QPen(Qt::black, 3));
  painter.drawRect(1, 1, image.width() - 2, image.height() - 2);
  painter.end();

  if (!image.save(patternPath, "PNG")) {
    return {};
  }

  return patternPath;
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
    QSignalBlocker blockRelative(relativePathCheck_);
    QSignalBlocker blockMidi(midiEnableCheck_);
    QSignalBlocker blockNdi(ndiEnableCheck_);
    QSignalBlocker blockSyphon(syphonEnableCheck_);
    QSignalBlocker blockDeckLink(deckLinkEnableCheck_);
    QSignalBlocker blockFilterPresets(filterPresetsEdit_);
    QSignalBlocker blockArtnetEnabled(artnetEnableCheck_);
    QSignalBlocker blockArtnetPort(artnetPortSpin_);
    QSignalBlocker blockArtnetUniverse(artnetUniverseSpin_);
    QSignalBlocker blockBackupCheck(backupTriggerCheck_);
    QSignalBlocker blockBackupUrl(backupUrlEdit_);
    QSignalBlocker blockBackupToken(backupTokenEdit_);
    QSignalBlocker blockFailoverEnabled(failoverSyncCheck_);
    QSignalBlocker blockFailoverHost(failoverHostEdit_);
    QSignalBlocker blockFailoverPeerPort(failoverPeerPortSpin_);
    QSignalBlocker blockFailoverListenPort(failoverListenPortSpin_);
    QSignalBlocker blockFailoverKey(failoverKeyEdit_);

    const int styleIndex = transitionCombo_->findData(static_cast<int>(config_.transitionStyle));
    if (styleIndex >= 0) {
      transitionCombo_->setCurrentIndex(styleIndex);
    }
    transitionDurationSpin_->setValue(config_.transitionDurationMs);
    oscPortSpin_->setValue(config_.oscPort);
    relativePathCheck_->setChecked(config_.useRelativeMediaPaths);
    midiEnableCheck_->setChecked(config_.midiEnabled);
    ndiEnableCheck_->setChecked(config_.ndiEnabled);
    syphonEnableCheck_->setChecked(config_.syphonEnabled);
    deckLinkEnableCheck_->setChecked(config_.deckLinkEnabled);
    filterPresetsEdit_->setText(serializeFilterPresets(config_.filterPresets));
    artnetEnableCheck_->setChecked(config_.artnetEnabled);
    artnetPortSpin_->setValue(config_.artnetPort);
    artnetUniverseSpin_->setValue(config_.artnetUniverse);
    backupTriggerCheck_->setChecked(config_.backupTriggerEnabled);
    backupUrlEdit_->setText(config_.backupTriggerUrl);
    backupTokenEdit_->setText(config_.backupTriggerToken);
    failoverSyncCheck_->setChecked(config_.failoverSyncEnabled);
    failoverHostEdit_->setText(config_.failoverPeerHost);
    failoverPeerPortSpin_->setValue(config_.failoverPeerPort);
    failoverListenPortSpin_->setValue(config_.failoverListenPort);
    failoverKeyEdit_->setText(config_.failoverSharedKey);
  }

  slatePathEdit_->setText(config_.fallbackSlatePath);
  refreshFilterPresetChoices();
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
