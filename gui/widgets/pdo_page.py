"""
Page 5: PDO/SM Validation (dev doc §12.5)

Shows SM sizes, PDO calculated sizes, status (OK/mismatch),
and PDO entry layout with bit offsets.
"""
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QGroupBox, QGridLayout,
    QLabel, QTextEdit, QHBoxLayout,
)
from PyQt5.QtGui import QFont


class PdoPage(QWidget):
    """PDO/SM validation display page."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ── SM size comparison ─────────────────
        sm_group = QGroupBox("Sync Manager Validation")
        self._sm_grid = QGridLayout(sm_group)
        self._sm_labels = {}
        layout.addWidget(sm_group)

        # ── PDO layout display ─────────────────
        pdo_group = QGroupBox("PDO Layout")
        pdo_layout = QVBoxLayout(pdo_group)
        self._pdo_text = QTextEdit()
        self._pdo_text.setReadOnly(True)
        self._pdo_text.setFont(QFont("Monospace", 9))
        pdo_layout.addWidget(self._pdo_text)
        layout.addWidget(pdo_group)

    # ── Public API ─────────────────────────────

    def set_sm_info(self, sm_info: list[dict]):
        """Display SM validation results.

        Each dict: {index, type, default_size, calculated_size, status}
        """
        # Clear old content
        for i in reversed(range(self._sm_grid.count())):
            self._sm_grid.itemAt(i).widget().setParent(None)
        self._sm_labels.clear()

        headers = ["SM", "Type", "Default Size", "Calculated", "Status"]
        for col, h in enumerate(headers):
            self._sm_grid.addWidget(QLabel(f"<b>{h}</b>"), 0, col)

        for row, info in enumerate(sm_info, start=1):
            status = info.get("status", "OK")
            color = "green" if status == "OK" else "red"
            self._sm_grid.addWidget(QLabel(str(info.get("index", ""))), row, 0)
            self._sm_grid.addWidget(QLabel(info.get("type", "")), row, 1)
            self._sm_grid.addWidget(QLabel(f"{info.get('default_size', 0)} bytes"), row, 2)
            self._sm_grid.addWidget(QLabel(f"{info.get('calculated_size', 0)} bytes"), row, 3)
            self._sm_grid.addWidget(QLabel(f"<font color='{color}'>{status}</font>"), row, 4)

    def set_pdo_layout(self, layout_text: str):
        """Display PDO entry layout with bit offsets."""
        self._pdo_text.setPlainText(layout_text)
