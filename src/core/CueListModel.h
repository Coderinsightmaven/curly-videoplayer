#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "core/Cue.h"

class CueListModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  enum Column {
    NameColumn = 0,
    FileColumn,
    ScreenColumn,
    LayerColumn,
    LoopColumn,
    HotkeyColumn,
    TimecodeColumn,
    MidiColumn,
    ColumnCount
  };

  explicit CueListModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  void addCue(const Cue& cue);
  void removeCue(int row);
  void updateCue(int row, const Cue& cue);
  void setCues(const QVector<Cue>& cues);

  Cue cueAt(int row) const;
  QVector<Cue> cues() const;
  bool isValidRow(int row) const;
  int rowForCueId(const QString& cueId) const;

 private:
  QVector<Cue> cues_;
};
