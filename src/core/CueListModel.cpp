#include "core/CueListModel.h"

#include <QFileInfo>

CueListModel::CueListModel(QObject* parent) : QAbstractTableModel(parent) {}

int CueListModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return cues_.size();
}

int CueListModel::columnCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return ColumnCount;
}

QVariant CueListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || !isValidRow(index.row())) {
    return {};
  }

  const Cue& cue = cues_.at(index.row());

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
      case NameColumn:
        return cue.name;
      case FileColumn:
        if (cue.isLiveInput && !cue.liveInputUrl.trimmed().isEmpty()) {
          return QString("[Live] %1").arg(cue.liveInputUrl);
        }
        return QFileInfo(cue.filePath).fileName();
      case ScreenColumn:
        return cue.targetScreen;
      case LayerColumn:
        return cue.layer;
      case LoopColumn:
        return cue.loop ? "Yes" : "No";
      case HotkeyColumn:
        return cue.hotkey;
      case TimecodeColumn:
        return cue.timecodeTrigger;
      case MidiColumn:
        return cue.midiNote >= 0 ? QVariant(cue.midiNote) : QVariant("-");
      default:
        return {};
    }
  }

  if (role == Qt::ToolTipRole && index.column() == FileColumn) {
    if (cue.isLiveInput && !cue.liveInputUrl.trimmed().isEmpty()) {
      return cue.liveInputUrl;
    }
    return cue.filePath;
  }

  return {};
}

QVariant CueListModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
    return QAbstractTableModel::headerData(section, orientation, role);
  }

  switch (section) {
    case NameColumn:
      return "Cue";
    case FileColumn:
      return "File";
    case ScreenColumn:
      return "Screen";
    case LayerColumn:
      return "Layer";
    case LoopColumn:
      return "Loop";
    case HotkeyColumn:
      return "Hotkey";
    case TimecodeColumn:
      return "Timecode";
    case MidiColumn:
      return "MIDI";
    default:
      return {};
  }
}

Qt::ItemFlags CueListModel::flags(const QModelIndex& index) const {
  if (!index.isValid()) {
    return Qt::NoItemFlags;
  }
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void CueListModel::addCue(const Cue& cue) {
  beginInsertRows(QModelIndex(), cues_.size(), cues_.size());
  cues_.push_back(cue);
  endInsertRows();
}

void CueListModel::removeCue(int row) {
  if (!isValidRow(row)) {
    return;
  }
  beginRemoveRows(QModelIndex(), row, row);
  cues_.removeAt(row);
  endRemoveRows();
}

void CueListModel::updateCue(int row, const Cue& cue) {
  if (!isValidRow(row)) {
    return;
  }

  cues_[row] = cue;
  const QModelIndex left = index(row, 0);
  const QModelIndex right = index(row, ColumnCount - 1);
  emit dataChanged(left, right);
}

void CueListModel::setCues(const QVector<Cue>& cues) {
  beginResetModel();
  cues_ = cues;
  endResetModel();
}

Cue CueListModel::cueAt(int row) const {
  if (!isValidRow(row)) {
    return {};
  }
  return cues_.at(row);
}

QVector<Cue> CueListModel::cues() const { return cues_; }

bool CueListModel::isValidRow(int row) const {
  return row >= 0 && row < cues_.size();
}

int CueListModel::rowForCueId(const QString& cueId) const {
  if (cueId.isEmpty()) {
    return -1;
  }

  for (int i = 0; i < cues_.size(); ++i) {
    if (cues_.at(i).id == cueId) {
      return i;
    }
  }
  return -1;
}
