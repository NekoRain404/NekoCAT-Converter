"""
页1: 文件导入 (dev doc §12.1)

功能：
  - 选择 ESI.XML、SDO.XML、模板 xlsx、输出目录
  - 点击 "解析" 后通过信号通知 MainWindow
  - 显示解析结果摘要

信号：
  parse_requested(dict) — 用户点击 "解析"，携带文件路径
"""
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
    QLabel, QLineEdit, QPushButton, QFileDialog,
    QGroupBox, QTextEdit,
)
from PyQt5.QtCore import pyqtSignal


class FileSelectPage(QWidget):
    """文件选择和初始解析页面。"""

    parse_requested = pyqtSignal(dict)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ── 文件选择组 ─────────────────────────
        file_group = QGroupBox("输入文件")
        grid = QGridLayout(file_group)

        self._esi_edit = self._make_file_row(grid, 0, "ESI.XML:", "XML files (*.xml);;All (*)")
        self._sdo_edit = self._make_file_row(grid, 1, "SDO 文件:", "XML/TXT files (*.xml *.txt);;All (*)")
        self._tpl_edit = self._make_file_row(grid, 2, "模板 xlsx:", "Excel files (*.xlsx);;All (*)")
        self._out_edit = self._make_dir_row(grid, 3, "输出目录:")

        layout.addWidget(file_group)

        # ── 解析按钮 ───────────────────────────
        btn_row = QHBoxLayout()
        btn_row.addStretch()
        self._parse_btn = QPushButton("解析")
        self._parse_btn.setMinimumWidth(120)
        self._parse_btn.clicked.connect(self._on_parse)
        btn_row.addWidget(self._parse_btn)
        layout.addLayout(btn_row)

        # ── 解析结果展示 ───────────────────────
        result_group = QGroupBox("解析结果")
        result_layout = QVBoxLayout(result_group)
        self._result_text = QTextEdit()
        self._result_text.setReadOnly(True)
        self._result_text.setMaximumHeight(180)
        result_layout.addWidget(self._result_text)
        layout.addWidget(result_group)

        layout.addStretch()

    # ── UI 构建辅助 ────────────────────────────

    def _make_file_row(self, grid, row, label, filter_str):
        """创建一行：标签 + 输入框 + 浏览按钮。"""
        lbl = QLabel(label)
        edit = QLineEdit()
        btn = QPushButton("浏览...")
        btn.setMaximumWidth(80)
        btn.clicked.connect(lambda: self._browse_file(edit, filter_str))
        grid.addWidget(lbl, row, 0)
        grid.addWidget(edit, row, 1)
        grid.addWidget(btn, row, 2)
        return edit

    def _make_dir_row(self, grid, row, label):
        """创建一行：标签 + 目录输入框 + 浏览按钮。"""
        lbl = QLabel(label)
        edit = QLineEdit("out")
        btn = QPushButton("浏览...")
        btn.setMaximumWidth(80)
        btn.clicked.connect(lambda: self._browse_dir(edit))
        grid.addWidget(lbl, row, 0)
        grid.addWidget(edit, row, 1)
        grid.addWidget(btn, row, 2)
        return edit

    def _browse_file(self, edit, filter_str):
        path, _ = QFileDialog.getOpenFileName(self, "选择文件", "", filter_str)
        if path:
            edit.setText(path)

    def _browse_dir(self, edit):
        path = QFileDialog.getExistingDirectory(self, "选择输出目录")
        if path:
            edit.setText(path)

    # ── 动作 ───────────────────────────────────

    def _on_parse(self):
        """收集文件路径，发出 parse_requested 信号。"""
        paths = {
            "esi_path": self._esi_edit.text().strip() or None,
            "sdo_path": self._sdo_edit.text().strip() or None,
            "template_path": self._tpl_edit.text().strip() or None,
            "output_dir": self._out_edit.text().strip() or "out",
        }
        if not paths["esi_path"] and not paths["sdo_path"]:
            self._result_text.setPlainText("错误: 请至少提供一个 ESI 或 SDO 文件。")
            return
        self.parse_requested.emit(paths)

    def show_parse_result(self, info: dict):
        """显示解析结果摘要（由 MainWindow 调用）。"""
        lines = [f"{key}: {val}" for key, val in info.items()]
        self._result_text.setPlainText("\n".join(lines))

    def get_paths(self) -> dict:
        """返回当前文件路径。"""
        return {
            "esi_path": self._esi_edit.text().strip() or None,
            "sdo_path": self._sdo_edit.text().strip() or None,
            "template_path": self._tpl_edit.text().strip() or None,
            "output_dir": self._out_edit.text().strip() or "out",
        }
