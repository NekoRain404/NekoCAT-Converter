"""
Page 4: Object Dictionary Table (dev doc §12.4)

Shows all objects in a filterable table with color coding:
  green=auto, yellow=callback, red=error, gray=skipped
"""
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QTableWidget, QTableWidgetItem,
    QComboBox, QLabel, QHeaderView, QGroupBox,
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QColor, QBrush


# Color scheme per dev doc §12.4
_COLORS = {
    "auto":     QColor(200, 255, 200),   # green
    "callback": QColor(255, 255, 180),   # yellow
    "error":    QColor(255, 200, 200),   # red
    "skipped":  QColor(220, 220, 220),   # gray
    "normal":   QColor(255, 255, 255),   # white
}

_TABLE_HEADERS = [
    "Index", "Name", "SubCount", "Category",
    "PDO Map", "SM Obj", "Risk", "CoeRead", "CoeWrite",
]


class ObjectTablePage(QWidget):
    """Object dictionary table with filtering."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._objects = []  # cached object list
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)

        # ── Filter bar ─────────────────────────
        filter_row = QHBoxLayout()
        filter_row.addWidget(QLabel("Filter:"))
        self._filter_combo = QComboBox()
        self._filter_combo.addItems([
            "All", "Errors only", "Callback only",
            "PDO mapping only", "Fxxx only", "Skipped",
        ])
        self._filter_combo.currentIndexChanged.connect(self._apply_filter)
        filter_row.addWidget(self._filter_combo)
        filter_row.addStretch()
        self._count_label = QLabel("0 objects")
        filter_row.addWidget(self._count_label)
        layout.addLayout(filter_row)

        # ── Table ──────────────────────────────
        self._table = QTableWidget()
        self._table.setColumnCount(len(_TABLE_HEADERS))
        self._table.setHorizontalHeaderLabels(_TABLE_HEADERS)
        self._table.setEditTriggers(QTableWidget.NoEditTriggers)
        self._table.setSelectionBehavior(QTableWidget.SelectRows)
        self._table.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        layout.addWidget(self._table)

    # ── Public API ─────────────────────────────

    def set_objects(self, objects: list[dict]):
        """Load objects into the table. Each dict has keys matching _TABLE_HEADERS."""
        self._objects = objects
        self._apply_filter()

    def _apply_filter(self):
        """Filter and refresh the table."""
        filter_idx = self._filter_combo.currentIndex()
        filtered = self._objects

        if filter_idx == 1:   # Errors only
            filtered = [o for o in filtered if o.get("risk")]
        elif filter_idx == 2: # Callback only
            filtered = [o for o in filtered if o.get("CoeRead") or o.get("CoeWrite")]
        elif filter_idx == 3: # PDO mapping
            filtered = [o for o in filtered if o.get("pdo_map")]
        elif filter_idx == 4: # Fxxx
            filtered = [o for o in filtered if o.get("index_int", 0) >= 0xF000]
        elif filter_idx == 5: # Skipped
            filtered = [o for o in filtered if o.get("skipped")]

        self._populate_table(filtered)

    def _populate_table(self, objects: list[dict]):
        """Fill the QTableWidget with object data."""
        self._table.setRowCount(len(objects))
        for row, obj in enumerate(objects):
            color = _determine_color(obj)
            brush = QBrush(color)

            for col, key in enumerate(_TABLE_HEADERS):
                val = str(obj.get(key, ""))
                item = QTableWidgetItem(val)
                item.setBackground(brush)
                self._table.setItem(row, col, item)

        self._count_label.setText(f"{len(objects)} objects")


def _determine_color(obj: dict) -> QColor:
    """Determine row color based on object state."""
    if obj.get("skipped"):
        return _COLORS["skipped"]
    if obj.get("risk"):
        return _COLORS["error"]
    if obj.get("CoeRead") or obj.get("CoeWrite"):
        return _COLORS["callback"]
    if obj.get("category") == "comm":
        return _COLORS["auto"]
    return _COLORS["normal"]
