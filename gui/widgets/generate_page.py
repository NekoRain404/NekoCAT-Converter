"""
Page 6: Generate Output (dev doc §12.6)

Buttons: Generate SSC xlsx, Generate TwinCAT ESI, Generate Report, Generate All
Shows: output file list after generation
"""
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QPushButton,
    QGroupBox, QListWidget, QListWidgetItem, QLabel,
    QProgressBar,
)
from PyQt5.QtCore import pyqtSignal, Qt


class GeneratePage(QWidget):
    """Generate output files page."""

    # Signals to trigger generation
    generate_all = pyqtSignal()
    generate_xlsx = pyqtSignal()
    generate_esi = pyqtSignal()
    generate_report = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ── Generate buttons ───────────────────
        btn_group = QGroupBox("Generate")
        btn_layout = QHBoxLayout(btn_group)

        for text, signal in [
            ("Generate All", self.generate_all),
            ("SSC xlsx only", self.generate_xlsx),
            ("TwinCAT ESI only", self.generate_esi),
            ("Report only", self.generate_report),
        ]:
            btn = QPushButton(text)
            btn.setMinimumWidth(140)
            btn.clicked.connect(signal.emit)
            btn_layout.addWidget(btn)

        layout.addWidget(btn_group)

        # ── Progress bar ───────────────────────
        self._progress = QProgressBar()
        self._progress.setRange(0, 100)
        self._progress.setValue(0)
        layout.addWidget(self._progress)

        # ── Output file list ───────────────────
        output_group = QGroupBox("Output Files")
        output_layout = QVBoxLayout(output_group)
        self._file_list = QListWidget()
        output_layout.addWidget(self._file_list)
        layout.addWidget(output_group)

        # ── Status label ───────────────────────
        self._status = QLabel("Ready")
        layout.addWidget(self._status)

    # ── Public API ─────────────────────────────

    def set_progress(self, value: int):
        """Update progress bar (0-100)."""
        self._progress.setValue(value)

    def set_status(self, text: str):
        """Update status message."""
        self._status.setText(text)

    def set_output_files(self, files: dict[str, str]):
        """Show generated file list."""
        self._file_list.clear()
        for name, path in files.items():
            self._file_list.addItem(f"{name}: {path}")

    def append_log(self, message: str):
        """Append a log message to the status area."""
        current = self._status.text()
        self._status.setText(f"{current}\n{message}")
