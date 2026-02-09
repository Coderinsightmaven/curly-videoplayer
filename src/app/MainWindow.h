#pragma once

#include <QMainWindow>
#include <QMap>
#include <QString>

#include "core/AppConfig.h"

class CueListModel;
class DisplayManager;
class ArtnetInputService;
class FailoverSyncService;
class DeckLinkBridge;
class MidiInputService;
class NdiBridge;
class OscServer;
class OutputRouter;
class PlaybackController;
class SyphonBridge;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QLabel;
class QNetworkAccessManager;
class QShortcut;
class QSpinBox;
class QTableView;

struct Cue;
struct ProjectData;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);

 private slots:
  void appendCueFromFile();
  void appendTestPatternCue();
  void removeSelectedCue();
  void relinkMissingMedia();
  void previewSelectedCue();
  void takePreviewCue();
  void playSelectedCue();
  void preloadSelectedCue();
  void stopSelectedCue();
  void stopAllCues();
  void nextCue();
  void previousCue();

  void saveProject();
  void saveProjectAs();
  void openProject();

  void syncEditorsFromSelection();
  void applyEditorsToSelection();
  void refreshScreenChoices();

  void syncCalibrationEditors();
  void applyCalibrationToScreen();
  void browseSlatePath();

  void restartOscServer();
  void applyControlConfig();

  void handleExternalPlayRow(int row);
  void handleExternalPreviewRow(int row);
  void handleExternalPreloadRow(int row);
  void handleExternalMidiNote(int note);
  void handleExternalDmx(int channel, int value);
  void handleExternalOverlayText(const QString& text);
  void handleExternalTake();
  void handleTimecode(const QString& timecode);
  void handleRemoteCueLive(const QString& cueId);
  void handleRemoteStopAll();
  void handleRemoteOverlayText(const QString& text);
  void forwardCueToBackup(const Cue& cue);

  void rebuildCueHotkeys();
  void refreshFilterPresetChoices();
  void showOutputs();
  void hideOutputs();
  void showPreview();
  void hidePreview();
  void showStatus(const QString& message);

 private:
  int selectedRow() const;
  int resolveCueRowFromIndex(int row) const;
  int resolveCueRowFromMidiNote(int note) const;
  int resolveCueRowFromDmx(int channel, int value) const;
  void selectRowIfValid(int row);
  QString ensureColorBarsPatternPath() const;
  TransitionStyle selectedTransitionStyle() const;
  int selectedTransitionDuration() const;
  void applyLoadedProject(const ProjectData& project);
  void connectCoreShortcuts();

  CueListModel* cueModel_;
  DisplayManager* displayManager_;
  OutputRouter* outputRouter_;
  PlaybackController* playbackController_;
  OscServer* oscServer_;
  ArtnetInputService* artnetService_;
  FailoverSyncService* failoverSync_;
  MidiInputService* midiService_;
  NdiBridge* ndiBridge_;
  SyphonBridge* syphonBridge_;
  DeckLinkBridge* deckLinkBridge_;

  QTableView* cueTable_;
  QComboBox* screenCombo_;
  QComboBox* targetSetCombo_;
  QSpinBox* layerSpin_;
  QCheckBox* loopCheck_;
  QLineEdit* hotkeyEdit_;
  QLineEdit* timecodeEdit_;
  QSpinBox* midiNoteSpin_;
  QSpinBox* dmxChannelSpin_;
  QSpinBox* dmxValueSpin_;
  QCheckBox* preloadCheck_;
  QComboBox* filterPresetCombo_;
  QLineEdit* cueFilterEdit_;
  QCheckBox* liveInputCheck_;
  QLineEdit* liveInputUrlEdit_;
  QCheckBox* cueTransitionOverrideCheck_;
  QComboBox* cueTransitionCombo_;
  QSpinBox* cueTransitionDurationSpin_;
  QLineEdit* playlistIdEdit_;
  QCheckBox* playlistAdvanceCheck_;
  QCheckBox* playlistLoopCheck_;
  QSpinBox* playlistDelaySpin_;
  QSpinBox* autoStopSpin_;
  QCheckBox* autoFollowCheck_;
  QSpinBox* followCueRowSpin_;
  QSpinBox* followDelaySpin_;

  QComboBox* transitionCombo_;
  QSpinBox* transitionDurationSpin_;

  QComboBox* calibrationScreenCombo_;
  QSpinBox* edgeBlendSpin_;
  QSpinBox* keystoneXSpin_;
  QSpinBox* keystoneYSpin_;
  QCheckBox* maskEnableCheck_;
  QSpinBox* maskLeftSpin_;
  QSpinBox* maskTopSpin_;
  QSpinBox* maskRightSpin_;
  QSpinBox* maskBottomSpin_;
  QLineEdit* slatePathEdit_;

  QSpinBox* oscPortSpin_;
  QCheckBox* relativePathCheck_;
  QCheckBox* midiEnableCheck_;
  QCheckBox* ndiEnableCheck_;
  QCheckBox* syphonEnableCheck_;
  QCheckBox* deckLinkEnableCheck_;
  QLineEdit* filterPresetsEdit_;
  QCheckBox* artnetEnableCheck_;
  QSpinBox* artnetPortSpin_;
  QSpinBox* artnetUniverseSpin_;
  QCheckBox* backupTriggerCheck_;
  QLineEdit* backupUrlEdit_;
  QLineEdit* backupTokenEdit_;
  QCheckBox* failoverSyncCheck_;
  QLineEdit* failoverHostEdit_;
  QSpinBox* failoverPeerPortSpin_;
  QSpinBox* failoverListenPortSpin_;
  QLineEdit* failoverKeyEdit_;
  QLabel* statusLabel_;
  QNetworkAccessManager* backupNetwork_;

  bool updatingEditors_ = false;
  bool updatingCalibration_ = false;
  bool suppressFailoverCuePublish_ = false;
  bool suppressFailoverStopPublish_ = false;
  bool suppressFailoverOverlayPublish_ = false;
  QString currentProjectPath_;
  AppConfig config_;
  QMap<QString, QShortcut*> cueHotkeys_;
};
