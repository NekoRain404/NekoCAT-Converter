"""
Page 2: Identity Configuration (dev doc §12.2)

Shows original device identity, lets user override for output.
Mode selection: Learning / Lab Clone / Product.
"""
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QGridLayout,
    QLabel, QLineEdit, QGroupBox, QRadioButton,
    QButtonGroup, QHBoxLayout,
)


class IdentityPage(QWidget):
    """Device identity configuration page."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ── Original identity (read-only) ──────
        orig_group = QGroupBox("Original Identity")
        orig_grid = QGridLayout(orig_group)
        self._orig_labels = {}
        for i, key in enumerate(["Vendor ID", "Product Code", "Revision", "Device Name"]):
            orig_grid.addWidget(QLabel(f"{key}:"), i, 0)
            lbl = QLabel("-")
            lbl.setStyleSheet("color: gray;")
            orig_grid.addWidget(lbl, i, 1)
            self._orig_labels[key] = lbl
        layout.addWidget(orig_group)

        # ── New identity (editable) ────────────
        new_group = QGroupBox("New Identity (override)")
        new_grid = QGridLayout(new_group)
        self._vendor_edit = self._add_row(new_grid, 0, "Vendor ID:")
        self._product_edit = self._add_row(new_grid, 1, "Product Code (hex):")
        self._revision_edit = self._add_row(new_grid, 2, "Revision (hex):")
        self._name_edit = self._add_row(new_grid, 3, "Device Name:")
        layout.addWidget(new_group)

        # ── Mode selection (§12.2) ─────────────
        mode_group = QGroupBox("Mode")
        mode_layout = QHBoxLayout(mode_group)
        self._mode_group = QButtonGroup(self)
        for i, (text, tooltip) in enumerate([
            ("Learning", "Default — safe for development"),
            ("Lab Clone", "Clone existing device identity"),
            ("Product", "Production — use real identity"),
        ]):
            rb = QRadioButton(text)
            rb.setToolTip(tooltip)
            self._mode_group.addButton(rb, i)
            mode_layout.addWidget(rb)
        self._mode_group.button(0).setChecked(True)  # default: Learning
        layout.addWidget(mode_group)

        layout.addStretch()

    def _add_row(self, grid, row, label):
        lbl = QLabel(label)
        edit = QLineEdit()
        grid.addWidget(lbl, row, 0)
        grid.addWidget(edit, row, 1)
        return edit

    # ── Public API ─────────────────────────────

    def set_original_identity(self, identity: dict):
        """Populate the original identity display."""
        for key, lbl in self._orig_labels.items():
            lbl.setText(str(identity.get(key, "-")))

    def get_config(self) -> dict:
        """Return user's identity overrides."""
        mode_map = {0: "learning", 1: "lab", 2: "product"}
        mode_id = self._mode_group.checkedId()
        return {
            "vendor_id": self._vendor_edit.text().strip(),
            "product_code": self._product_edit.text().strip(),
            "revision": self._revision_edit.text().strip(),
            "device_name": self._name_edit.text().strip(),
            "mode": mode_map.get(mode_id, "learning"),
        }
