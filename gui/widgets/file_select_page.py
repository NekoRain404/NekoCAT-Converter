"""
Page 1: File Import (dev doc §12.1)

Controls: ESI path, SDO path, template path, output dir
Buttons: Browse, Parse
After parse: shows device identity summary
"""
from pathlib import Path
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
    QLabel, QLineEdit, QPushButton, QFileDialog,
    QGroupBox, QTextEdit, QSizePolicy,
)
from PyQt5.QtCore import pyqtSignal


class FileSelectPage(QWidget):
    """File selection and initial parsing page."""

    # Emitted when user clicks "Parse" — carries the file paths
    parse_requested = pyqtSignal(dict)
    # Emitted after successful parse — carries parsed info dict
    parse_done = pyqtSignal(dict)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ── File selection group ───────────────
        file_group = QGroupBox("Input Files")
        grid = QGridLayout(file_group)

        self._esi_edit = self._make_file_row(grid, 0, "ESI.XML:", "XML files (*.xml);;All (*)")
        self._sdo_edit = self._make_file_row(grid, 1, "SDO.XML:", "XML/TXT files (*.xml *.txt);;All (*)")
        self._tpl_edit = self._make_file_row(grid, 2, "Template xlsx:", "Excel files (*.xlsx);;All (*)")
        self._out_edit = self._make_dir_row(grid, 3, "Output dir:")

        layout.addWidget(file_group)

        # ── Parse button ───────────────────────
        btn_row = QHBoxLayout()
        btn_row.addStretch()
        self._parse_btn = QPushButton("Parse")
        self._parse_btn.setMinimumWidth(120)
        self._parse_btn.clicked.connect(self._on_parse)
        btn_row.addWidget(self._parse_btn)
        layout.addLayout(btn_row)

        # ── Parse result display ───────────────
        result_group = QGroupBox("Parse Result")
        result_layout = QVBoxLayout(result_group)
        self._result_text = QTextEdit()
        self._result_text.setReadOnly(True)
        self._result_text.setMaximumHeight(180)
        result_layout.addWidget(self._result_text)
        layout.addWidget(result_group)

        layout.addStretch()

    # ── UI helpers ─────────────────────────────

    def _make_file_row(self, grid, row, label, filter_str):
        """Add a label + line edit + browse button row."""
        lbl = QLabel(label)
        edit = QLineEdit()
        btn = QPushButton("Browse...")
        btn.setMaximumWidth(80)
        btn.clicked.connect(lambda: self._browse_file(edit, filter_str))
        grid.addWidget(lbl, row, 0)
        grid.addWidget(edit, row, 1)
        grid.addWidget(btn, row, 2)
        return edit

    def _make_dir_row(self, grid, row, label):
        """Add a label + line edit + browse-dir button row."""
        lbl = QLabel(label)
        edit = QLineEdit("out")
        btn = QPushButton("Browse...")
        btn.setMaximumWidth(80)
        btn.clicked.connect(lambda: self._browse_dir(edit))
        grid.addWidget(lbl, row, 0)
        grid.addWidget(edit, row, 1)
        grid.addWidget(btn, row, 2)
        return edit

    def _browse_file(self, edit, filter_str):
        path, _ = QFileDialog.getOpenFileName(self, "Select file", "", filter_str)
        if path:
            edit.setText(path)

    def _browse_dir(self, edit):
        path = QFileDialog.getExistingDirectory(self, "Select output directory")
        if path:
            edit.setText(path)

    # ── Actions ────────────────────────────────

    def _on_parse(self):
        """Collect file paths and emit parse_requested signal."""
        paths = {
            "esi_path": self._esi_edit.text().strip() or None,
            "sdo_path": self._sdo_edit.text().strip() or None,
            "template_path": self._tpl_edit.text().strip() or None,
            "output_dir": self._out_edit.text().strip() or "out",
        }
        if not paths["esi_path"] and not paths["sdo_path"]:
            self._result_text.setPlainText("Error: Provide at least one of ESI or SDO file.")
            return
        self.parse_requested.emit(paths)

    def show_parse_result(self, info: dict):
        """Display parse result summary (called from main_window)."""
        lines = []
        for key, val in info.items():
            lines.append(f"{key}: {val}")
        self._result_text.setPlainText("\n".join(lines))

    def get_paths(self) -> dict:
        """Return current file paths."""
        return {
            "esi_path": self._esi_edit.text().strip() or None,
            "sdo_path": self._sdo_edit.text().strip() or None,
            "template_path": self._tpl_edit.text().strip() or None,
            "output_dir": self._out_edit.text().strip() or "out",
        }
