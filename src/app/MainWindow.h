#pragma once

#include <QMainWindow>
#include <QMap>
#include <QString>

#include "core/AppConfig.h"

class CueListModel;
class DisplayManager;
class MidiInputService;
class NdiBridge;
class OscServer;
class OutputRouter;
class PlaybackController;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QLabel;
class QShortcut;
class QSpinBox;
class QTableView;

struct ProjectData;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);

 private slots:
  void appendCueFromFile();
  void removeSelectedCue();
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
  void handleExternalTake();
  void handleTimecode(const QString& timecode);

  void rebuildCueHotkeys();
  void showOutputs();
  void hideOutputs();
  void showPreview();
  void hidePreview();
  void showStatus(const QString& message);

 private:
  int selectedRow() const;
  int resolveCueRowFromIndex(int row) const;
  int resolveCueRowFromMidiNote(int note) const;
  void selectRowIfValid(int row);
  TransitionStyle selectedTransitionStyle() const;
  int selectedTransitionDuration() const;
  void applyLoadedProject(const ProjectData& project);
  void connectCoreShortcuts();

  CueListModel* cueModel_;
  DisplayManager* displayManager_;
  OutputRouter* outputRouter_;
  PlaybackController* playbackController_;
  OscServer* oscServer_;
  MidiInputService* midiService_;
  NdiBridge* ndiBridge_;

  QTableView* cueTable_;
  QComboBox* screenCombo_;
  QSpinBox* layerSpin_;
  QCheckBox* loopCheck_;
  QLineEdit* hotkeyEdit_;
  QLineEdit* timecodeEdit_;
  QSpinBox* midiNoteSpin_;
  QCheckBox* preloadCheck_;

  QComboBox* transitionCombo_;
  QSpinBox* transitionDurationSpin_;

  QComboBox* calibrationScreenCombo_;
  QSpinBox* edgeBlendSpin_;
  QSpinBox* keystoneXSpin_;
  QSpinBox* keystoneYSpin_;
  QLineEdit* slatePathEdit_;

  QSpinBox* oscPortSpin_;
  QCheckBox* midiEnableCheck_;
  QCheckBox* ndiEnableCheck_;
  QLabel* statusLabel_;

  bool updatingEditors_ = false;
  bool updatingCalibration_ = false;
  QString currentProjectPath_;
  AppConfig config_;
  QMap<QString, QShortcut*> cueHotkeys_;
};
