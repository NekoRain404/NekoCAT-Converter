"""
Page 3: Rule Strategy Configuration (dev doc §12.3)

Options for: OCTET_STRING handling, unaligned objects,
communication objects, Fxxx objects.
"""
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QGroupBox,
    QRadioButton, QButtonGroup, QCheckBox, QHBoxLayout,
)


class RulePage(QWidget):
    """Rule strategy configuration page."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ── OCTET_STRING strategy ──────────────
        octet_group = QGroupBox("OCTET_STRING Handling")
        octet_layout = QVBoxLayout(octet_group)
        self._octet_group = QButtonGroup(self)
        for i, (text, tip) in enumerate([
            ("Convert to STRING + CoE callback", "Recommended — SSC Tool compatible"),
            ("Skip", "Exclude from output"),
            ("Keep + manual callback", "Preserve type, user sets callback"),
        ]):
            rb = QRadioButton(text)
            rb.setToolTip(tip)
            self._octet_group.addButton(rb, i)
            octet_layout.addWidget(rb)
        self._octet_group.button(0).setChecked(True)
        layout.addWidget(octet_group)

        # ── Unaligned objects ──────────────────
        align_group = QGroupBox("Unaligned Objects (word offset)")
        align_layout = QVBoxLayout(align_group)
        self._align_group = QButtonGroup(self)
        for i, (text, tip) in enumerate([
            ("Object-level CoE callback", "Recommended — no layout change"),
            ("Insert padding", "Changes object layout"),
            ("Skip object", "Exclude unaligned objects"),
        ]):
            rb = QRadioButton(text)
            rb.setToolTip(tip)
            self._align_group.addButton(rb, i)
            align_layout.addWidget(rb)
        self._align_group.button(0).setChecked(True)
        layout.addWidget(align_group)

        # ── Communication objects ──────────────
        comm_group = QGroupBox("Communication Objects (0x1000-0x1FFF)")
        comm_layout = QVBoxLayout(comm_group)
        self._comm_group = QButtonGroup(self)
        for i, (text, tip) in enumerate([
            ("Let SSC auto-generate", "Recommended — SSC handles comm objects"),
            ("Force import", "Include in xlsx despite SSC warnings"),
            ("Skip", "Exclude all comm objects"),
        ]):
            rb = QRadioButton(text)
            rb.setToolTip(tip)
            self._comm_group.addButton(rb, i)
            comm_layout.addWidget(rb)
        self._comm_group.button(0).setChecked(True)
        layout.addWidget(comm_group)

        # ── Fxxx objects ───────────────────────
        fxxx_group = QGroupBox("High-profile Objects (0xF000+)")
        fxxx_layout = QVBoxLayout(fxxx_group)
        self._include_fxxx = QCheckBox("Include Fxxx objects in output")
        self._include_fxxx.setChecked(True)
        fxxx_layout.addWidget(self._include_fxxx)
        layout.addWidget(fxxx_group)

        layout.addStretch()

    # ── Public API ─────────────────────────────

    def get_config(self) -> dict:
        """Return strategy selections."""
        octet_map = {0: "string_with_callback", 1: "skip", 2: "manual_callback"}
        align_map = {0: "coe_callback", 1: "insert_padding", 2: "skip"}
        comm_map = {0: "ssc_auto", 1: "force_import", 2: "skip"}
        return {
            "octet_string_strategy": octet_map.get(self._octet_group.checkedId(), "string_with_callback"),
            "unaligned_strategy": align_map.get(self._align_group.checkedId(), "coe_callback"),
            "communication_object_strategy": comm_map.get(self._comm_group.checkedId(), "ssc_auto"),
            "include_fxxx": self._include_fxxx.isChecked(),
        }
