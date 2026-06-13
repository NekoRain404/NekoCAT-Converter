"""
ConvertWorker — QThread for non-blocking conversion.

Per dev doc §13: runs the full pipeline in a background thread,
emits progress/log/result signals back to the GUI.

Only depends on: nekoecat.core (facade).  No direct engine/parser imports.
"""
from PyQt5.QtCore import QThread, pyqtSignal
from nekoecat.config import ConvertConfig
from nekoecat.core import convert, ConvertResult


class ConvertWorker(QThread):
    """Background worker that runs the conversion pipeline.

    Signals:
        log(str)         — human-readable log messages
        progress(int)    — 0-100 progress percentage
        finished_ok(dict) — ConvertResult as dict on success
        failed(str)      — error message on failure
    """
    log = pyqtSignal(str)
    progress = pyqtSignal(int)
    finished_ok = pyqtSignal(object)   # ConvertResult
    failed = pyqtSignal(str)

    def __init__(self, config: ConvertConfig, parent=None):
        super().__init__(parent)
        self.config = config

    def run(self):
        """Execute the conversion pipeline.  Called by QThread.start()."""
        try:
            self.log.emit("Starting conversion...")
            self.progress.emit(5)

            self.log.emit(f"ESI: {self.config.esi_path or '(none)'}")
            self.log.emit(f"SDO: {self.config.sdo_path or '(none)'}")
            self.progress.emit(10)

            # Call the core facade — the only entry point
            result = convert(self.config)

            self.progress.emit(100)

            if result.success:
                self.log.emit(f"Conversion complete: {result.output_dir}")
                self.finished_ok.emit(result)
            else:
                self.log.emit(f"Conversion failed: {result.error}")
                self.failed.emit(result.error or "Unknown error")

        except Exception as e:
            self.log.emit(f"Exception: {e}")
            self.failed.emit(str(e))
