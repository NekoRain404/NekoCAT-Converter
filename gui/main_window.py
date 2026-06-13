"""
Main Window — orchestrates all pages and the conversion worker.

Signal flow:
  Page1.parse_requested → main_window._on_parse → worker → Page1/4/5 updates
  Page6.generate_all   → main_window._on_generate_all → worker → Page6 updates

Only depends on: gui.widgets, gui.worker, nekoecat.core.
"""
from PyQt5.QtWidgets import (
    QMainWindow, QTabWidget, QTextEdit, QSplitter,
    QStatusBar, QMenuBar, QAction, QWidget, QVBoxLayout,
)
from PyQt5.QtCore import Qt

from gui.widgets.file_select_page import FileSelectPage
from gui.widgets.identity_page import IdentityPage
from gui.widgets.rule_page import RulePage
from gui.widgets.object_table_page import ObjectTablePage
from gui.widgets.pdo_page import PdoPage
from gui.widgets.generate_page import GeneratePage
from gui.worker import ConvertWorker
from nekoecat.config import ConvertConfig
from nekoecat.parser import parse_esi, parse_sdo
from nekoecat.engine import classify_objects


class MainWindow(QMainWindow):
    """Main application window with tabbed pages and log area."""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("NekoECAT Converter")
        self.setMinimumSize(960, 720)

        # ── Pages ──────────────────────────────
        self._file_page = FileSelectPage()
        self._identity_page = IdentityPage()
        self._rule_page = RulePage()
        self._obj_page = ObjectTablePage()
        self._pdo_page = PdoPage()
        self._gen_page = GeneratePage()

        # ── Tab widget ─────────────────────────
        tabs = QTabWidget()
        tabs.addTab(self._file_page, "1. File Import")
        tabs.addTab(self._identity_page, "2. Identity")
        tabs.addTab(self._rule_page, "3. Rules")
        tabs.addTab(self._obj_page, "4. Objects")
        tabs.addTab(self._pdo_page, "5. PDO/SM")
        tabs.addTab(self._gen_page, "6. Generate")
        self._tabs = tabs

        # ── Log area ───────────────────────────
        self._log = QTextEdit()
        self._log.setReadOnly(True)
        self._log.setMaximumHeight(150)

        # ── Splitter: tabs on top, log on bottom
        splitter = QSplitter(Qt.Vertical)
        splitter.addWidget(tabs)
        splitter.addWidget(self._log)
        splitter.setSizes([550, 150])
        self.setCentralWidget(splitter)

        # ── Status bar ─────────────────────────
        self.statusBar().showMessage("Ready")

        # ── Connect signals ────────────────────
        self._file_page.parse_requested.connect(self._on_parse)
        self._gen_page.generate_all.connect(self._on_generate_all)

        self._worker = None
        self._parsed_device = None

    # ── Signal handlers ────────────────────────

    def _on_parse(self, paths: dict):
        """Handle file parse request from Page 1."""
        self._log_message("Parsing files...")
        try:
            esi_device = parse_esi(paths["esi_path"]) if paths["esi_path"] else None
            sdo_device = parse_sdo(paths["sdo_path"]) if paths["sdo_path"] else None

            # Pick whichever device has data
            device = esi_device or sdo_device
            if esi_device and sdo_device:
                from nekoecat.engine import merge
                device = merge(esi_device, sdo_device)

            if device is None:
                self._log_message("Error: no device parsed")
                return

            classify_objects(device)
            self._parsed_device = device

            # Update Page 1 result display
            ident = device.identity
            self._file_page.show_parse_result({
                "Vendor ID": f"0x{ident.vendor_id:08X}",
                "Product Code": f"0x{ident.product_code:08X}",
                "Revision": f"0x{ident.revision_number:08X}",
                "Device Name": ident.device_name,
                "Objects": len(device.objects),
                "SM configs": len(device.sm_configs),
                "RxPDO": len(device.rxpdo_assign),
                "TxPDO": len(device.txpdo_assign),
            })

            # Update Page 2 identity
            self._identity_page.set_original_identity({
                "Vendor ID": f"0x{ident.vendor_id:08X}",
                "Product Code": f"0x{ident.product_code:08X}",
                "Revision": f"0x{ident.revision_number:08X}",
                "Device Name": ident.device_name,
            })

            # Update Page 4 object table
            obj_rows = []
            for obj in device.sorted_objects():
                obj_rows.append({
                    "Index": f"0x{obj.index:04X}",
                    "index_int": obj.index,
                    "Name": obj.name,
                    "SubCount": len(obj.subindices),
                    "Category": obj.category,
                    "pdo_map": obj.is_pdo_mapping_object,
                    "SM Obj": obj.is_sm_object,
                    "Risk": ", ".join(obj.risk_flags) if obj.risk_flags else "",
                    "CoeRead": "TRUE" if obj.coe_read else "",
                    "CoeWrite": "TRUE" if obj.coe_write else "",
                    "skipped": obj.skipped,
                    "risk": bool(obj.risk_flags),
                })
            self._obj_page.set_objects(obj_rows)

            # Update Page 5 PDO/SM
            sm_info = []
            for sm in device.sm_configs:
                calc = sm.total_byte_length
                status = "OK" if sm.default_size == calc or calc == 0 else "MISMATCH"
                sm_info.append({
                    "index": f"SM{sm.index}",
                    "type": sm.pdo_type,
                    "default_size": sm.default_size,
                    "calculated_size": calc,
                    "status": status,
                })
            self._pdo_page.set_sm_info(sm_info)

            self._log_message(f"Parsed: {len(device.objects)} objects")
            self.statusBar().showMessage("Parse complete")
            self._tabs.setCurrentIndex(0)  # stay on file page

        except Exception as e:
            self._log_message(f"Parse error: {e}")
            self.statusBar().showMessage("Parse failed")

    def _on_generate_all(self):
        """Handle Generate All button from Page 6."""
        if self._parsed_device is None:
            self._log_message("Error: parse files first")
            return

        self._log_message("Starting conversion...")

        # Build ConvertConfig from all pages
        paths = self._file_page.get_paths()
        identity = self._identity_page.get_config()
        rules = self._rule_page.get_config()

        config = ConvertConfig(
            esi_path=paths["esi_path"],
            sdo_path=paths["sdo_path"],
            template_xlsx_path=paths["template_path"],
            output_dir=paths["output_dir"],
            device_name=identity.get("device_name") or None,
            mode=identity.get("mode", "learning"),
            **rules,
        )

        # Parse vendor/product overrides
        try:
            if identity.get("vendor_id"):
                config.vendor_id = int(identity["vendor_id"], 0)
            if identity.get("product_code"):
                config.product_code = int(identity["product_code"], 0)
            if identity.get("revision"):
                config.revision_number = int(identity["revision"], 0)
        except ValueError as e:
            self._log_message(f"Invalid identity value: {e}")
            return

        # Run conversion in background thread
        self._worker = ConvertWorker(config)
        self._worker.log.connect(self._log_message)
        self._worker.progress.connect(self._gen_page.set_progress)
        self._worker.finished_ok.connect(self._on_conversion_done)
        self._worker.failed.connect(self._on_conversion_failed)
        self._worker.start()

        self.statusBar().showMessage("Converting...")
        self._gen_page.set_status("Conversion in progress...")

    def _on_conversion_done(self, result):
        """Handle successful conversion."""
        self._log_message(f"Done: {result.output_dir}")
        self._gen_page.set_status("Conversion complete!")
        self._gen_page.set_progress(100)

        # Collect all output files
        files = {}
        if result.xlsx_path:
            files["SSC xlsx"] = result.xlsx_path
        files.update(result.report_files)
        self._gen_page.set_output_files(files)

        self.statusBar().showMessage("Conversion complete")
        self._tabs.setCurrentIndex(5)  # switch to Generate page

    def _on_conversion_failed(self, error: str):
        """Handle failed conversion."""
        self._log_message(f"FAILED: {error}")
        self._gen_page.set_status(f"Failed: {error}")
        self.statusBar().showMessage("Conversion failed")

    # ── Helpers ────────────────────────────────

    def _log_message(self, msg: str):
        """Append a message to the log area."""
        self._log.append(msg)
