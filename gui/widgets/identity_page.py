"""
页2: 身份配置 (dev doc §12.2)

功能：
  - 显示原始设备身份（只读）
  - 用户可覆盖身份用于输出
  - 模式选择：Learning / Lab Clone / Product
"""
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QGridLayout,
    QLabel, QLineEdit, QGroupBox, QRadioButton,
    QButtonGroup, QHBoxLayout,
)


class IdentityPage(QWidget):
    """设备身份配置页面。"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ── 原始身份（只读） ──────────────────
        orig_group = QGroupBox("原始身份")
        orig_grid = QGridLayout(orig_group)
        self._orig_labels = {}
        for i, key in enumerate(["Vendor ID", "Product Code", "Revision", "Device Name"]):
            orig_grid.addWidget(QLabel(f"{key}:"), i, 0)
            lbl = QLabel("-")
            lbl.setStyleSheet("color: #888;")
            orig_grid.addWidget(lbl, i, 1)
            self._orig_labels[key] = lbl
        layout.addWidget(orig_group)

        # ── 新身份（可编辑） ──────────────────
        new_group = QGroupBox("新身份 (覆盖)")
        new_grid = QGridLayout(new_group)
        self._vendor_edit = self._add_row(new_grid, 0, "Vendor ID:")
        self._product_edit = self._add_row(new_grid, 1, "Product Code (hex):")
        self._revision_edit = self._add_row(new_grid, 2, "Revision (hex):")
        self._name_edit = self._add_row(new_grid, 3, "Device Name:")
        layout.addWidget(new_group)

        # ── 模式选择 (dev doc §12.2) ──────────
        mode_group = QGroupBox("模式")
        mode_layout = QHBoxLayout(mode_group)
        self._mode_group = QButtonGroup(self)
        for i, (text, tip) in enumerate([
            ("学习模式", "默认 — 开发阶段安全使用"),
            ("实验室克隆", "克隆已有设备身份"),
            ("产品模式", "生产环境 — 使用真实身份"),
        ]):
            rb = QRadioButton(text)
            rb.setToolTip(tip)
            self._mode_group.addButton(rb, i)
            mode_layout.addWidget(rb)
        self._mode_group.button(0).setChecked(True)
        layout.addWidget(mode_group)

        layout.addStretch()

    def _add_row(self, grid, row, label):
        grid.addWidget(QLabel(label), row, 0)
        edit = QLineEdit()
        grid.addWidget(edit, row, 1)
        return edit

    def set_original_identity(self, identity: dict):
        """填充原始身份信息。"""
        for key, lbl in self._orig_labels.items():
            lbl.setText(str(identity.get(key, "-")))

    def get_config(self) -> dict:
        """返回用户的身份覆盖配置。"""
        mode_map = {0: "learning", 1: "lab", 2: "product"}
        return {
            "vendor_id": self._vendor_edit.text().strip(),
            "product_code": self._product_edit.text().strip(),
            "revision": self._revision_edit.text().strip(),
            "device_name": self._name_edit.text().strip(),
            "mode": mode_map.get(self._mode_group.checkedId(), "learning"),
        }
