"""
MainWindow — 主窗口：左侧边导航 + 右侧内容区。

布局参考 docs/ui/ 参考图：
  ┌────────────┬──────────────────────────┐
  │ 侧边栏     │  QStackedWidget (内容页)  │
  │ Step 1     │                          │
  │ Step 2     │                          │
  │ Step 3     ├──────────────────────────┤
  │            │  底部：日志 + 进度 + 生成  │
  └────────────┴──────────────────────────┘

解耦原则：只导入 nekoecat.core（门面）。
"""
from PyQt5.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QTextEdit, QProgressBar, QLabel,
    QSplitter, QStackedWidget, QFrame, QSizePolicy,
)
from PyQt5.QtCore import Qt, QSize
from PyQt5.QtGui import QFont, QIcon, QColor, QPalette

from gui.widgets.file_select_page import FileSelectPage
from gui.widgets.identity_page import IdentityPage
from gui.widgets.rule_page import RulePage
from gui.widgets.object_table_page import ObjectTablePage
from gui.widgets.pdo_page import PdoPage
from gui.widgets.generate_page import GeneratePage
from gui.worker import ConvertWorker
from nekoecat.config import ConvertConfig
from nekoecat.core import parse_only

# ── 深色主题样式 (参考 UI 图) ─────────────────
DARK_STYLE = """
QMainWindow, QWidget {
    background-color: #2b2b2b;
    color: #e0e0e0;
    font-size: 13px;
}
QGroupBox {
    border: 1px solid #555;
    border-radius: 4px;
    margin-top: 10px;
    padding-top: 14px;
    font-weight: bold;
    color: #aaa;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
}
QLineEdit {
    background-color: #3c3c3c;
    border: 1px solid #555;
    border-radius: 3px;
    padding: 5px;
    color: #e0e0e0;
}
QPushButton {
    background-color: #0d6efd;
    color: white;
    border: none;
    border-radius: 4px;
    padding: 6px 16px;
    font-weight: bold;
}
QPushButton:hover { background-color: #0b5ed7; }
QPushButton:pressed { background-color: #0a58ca; }
QPushButton:disabled { background-color: #555; color: #888; }
QPushButton#sidebar-btn {
    background-color: transparent;
    text-align: left;
    padding: 10px 14px;
    border-radius: 6px;
    font-size: 13px;
    color: #ccc;
}
QPushButton#sidebar-btn:hover { background-color: #3c3c3c; }
QPushButton#sidebar-btn[active="true"] {
    background-color: #0d6efd;
    color: white;
    font-weight: bold;
}
QRadioButton { color: #e0e0e0; spacing: 6px; }
QRadioButton::indicator { width: 16px; height: 16px; }
QCheckBox { color: #e0e0e0; spacing: 6px; }
QTextEdit {
    background-color: #1e1e1e;
    border: 1px solid #444;
    color: #d4d4d4;
    font-family: Consolas, monospace;
    font-size: 12px;
}
QTableWidget {
    background-color: #1e1e1e;
    alternate-background-color: #252525;
    color: #e0e0e0;
    gridline-color: #444;
    border: 1px solid #444;
}
QTableWidget::item { padding: 4px; }
QHeaderView::section {
    background-color: #333;
    color: #e0e0e0;
    border: 1px solid #444;
    padding: 5px;
    font-weight: bold;
}
QProgressBar {
    border: 1px solid #555;
    border-radius: 4px;
    text-align: center;
    color: white;
}
QProgressBar::chunk { background-color: #0d6efd; border-radius: 3px; }
QLabel#sidebar-title { font-size: 16px; font-weight: bold; color: #0d6efd; }
QLabel#sidebar-subtitle { font-size: 11px; color: #888; }
QLabel#step-label { font-size: 11px; font-weight: bold; color: #0d6efd; }
"""


class MainWindow(QMainWindow):
    """主窗口：左侧导航 + 右侧内容区 + 底部日志/进度。"""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("NekoECAT Converter")
        self.setMinimumSize(1100, 700)
        self.resize(1200, 750)

        # 应用深色主题
        self.setStyleSheet(DARK_STYLE)

        # ── 创建各页面 ─────────────────────────
        self._file_page = FileSelectPage()
        self._identity_page = IdentityPage()
        self._rule_page = RulePage()
        self._obj_page = ObjectTablePage()
        self._pdo_page = PdoPage()
        self._gen_page = GeneratePage()

        self._pages = [
            self._file_page,
            self._identity_page,
            self._rule_page,
            self._obj_page,
            self._pdo_page,
            self._gen_page,
        ]

        # ── 内容区 (QStackedWidget) ────────────
        self._stack = QStackedWidget()
        for page in self._pages:
            self._stack.addWidget(page)

        # ── 侧边栏 ────────────────────────────
        sidebar = self._build_sidebar()

        # ── 底部区域：日志 + 进度条 ────────────
        self._log = QTextEdit()
        self._log.setReadOnly(True)
        self._log.setMaximumHeight(120)
        self._log.setPlaceholderText("日志输出...")

        self._progress = QProgressBar()
        self._progress.setRange(0, 100)
        self._progress.setValue(0)
        self._progress.setMaximumHeight(20)

        bottom = QWidget()
        bottom_layout = QVBoxLayout(bottom)
        bottom_layout.setContentsMargins(0, 0, 0, 0)
        bottom_layout.addWidget(self._log)
        bottom_layout.addWidget(self._progress)

        # ── 右侧主区域：内容 + 底部 ───────────
        right = QWidget()
        right_layout = QVBoxLayout(right)
        right_layout.setContentsMargins(0, 0, 0, 0)
        right_layout.addWidget(self._stack, stretch=1)
        right_layout.addWidget(bottom, stretch=0)

        # ── 水平分割：侧边栏 + 内容 ───────────
        main_widget = QWidget()
        main_layout = QHBoxLayout(main_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        main_layout.addWidget(sidebar, stretch=0)
        main_layout.addWidget(right, stretch=1)

        self.setCentralWidget(main_widget)

        # ── 状态栏 ─────────────────────────────
        self.statusBar().setStyleSheet("background-color: #333; color: #aaa;")
        self.statusBar().showMessage("就绪")

        # ── 信号连接 ───────────────────────────
        self._file_page.parse_requested.connect(self._on_parse)
        self._gen_page.generate_all.connect(self._on_generate_all)

        self._worker = None
        self._parsed_device = None

        # 默认选中第一页
        self._set_active_page(0)

    # ═══════════════════════════════════════════
    # 侧边栏构建
    # ═══════════════════════════════════════════

    def _build_sidebar(self) -> QWidget:
        """构建左侧导航栏，参考 UI 图的样式。"""
        sidebar = QWidget()
        sidebar.setFixedWidth(240)
        sidebar.setStyleSheet("background-color: #1e1e1e;")
        layout = QVBoxLayout(sidebar)
        layout.setContentsMargins(12, 16, 12, 16)
        layout.setSpacing(4)

        # ── 标题 ───────────────────────────────
        title = QLabel("NekoECAT Converter")
        title.setObjectName("sidebar-title")
        subtitle = QLabel("ECAT → SSC Tool")
        subtitle.setObjectName("sidebar-subtitle")
        layout.addWidget(title)
        layout.addWidget(subtitle)
        layout.addSpacing(16)

        # ── 导航按钮 ──────────────────────────
        self._sidebar_buttons = []
        step_labels = [
            ("Step 1", "文件导入"),
            ("Step 2", "身份配置"),
            ("Step 3", "规则策略"),
            ("Step 4", "对象字典"),
            ("Step 5", "PDO/SM 校验"),
            ("Step 6", "生成输出"),
        ]
        for i, (step, name) in enumerate(step_labels):
            btn = QPushButton(f"{step}: {name}")
            btn.setObjectName("sidebar-btn")
            btn.setCursor(Qt.PointingHandCursor)
            idx = i
            btn.clicked.connect(lambda checked, x=idx: self._set_active_page(x))
            self._sidebar_buttons.append(btn)
            layout.addWidget(btn)

        layout.addStretch()

        # ── 底部版本信息 ───────────────────────
        ver = QLabel("v0.1.0")
        ver.setStyleSheet("color: #555; font-size: 11px;")
        layout.addWidget(ver)

        return sidebar

    def _set_active_page(self, index: int):
        """切换到指定页面，更新侧边栏高亮。"""
        self._stack.setCurrentIndex(index)
        for i, btn in enumerate(self._sidebar_buttons):
            btn.setProperty("active", i == index)
            btn.style().unpolish(btn)
            btn.style().polish(btn)

    # ═══════════════════════════════════════════
    # 信号处理：解析
    # ═══════════════════════════════════════════

    def _on_parse(self, paths: dict):
        """处理页1的 "解析" 按钮点击。"""
        self._log_message("正在解析文件...")
        self._progress.setValue(10)
        try:
            device = parse_only(
                esi_path=paths.get("esi_path"),
                sdo_path=paths.get("sdo_path"),
            )
            self._parsed_device = device
            self._progress.setValue(50)

            # ── 更新页1：解析结果 ──────────────
            ident = device.identity
            self._file_page.show_parse_result({
                "厂商 ID": f"0x{ident.vendor_id:08X}",
                "产品代码": f"0x{ident.product_code:08X}",
                "修订号": f"0x{ident.revision_number:08X}",
                "设备名称": ident.device_name,
                "对象数量": len(device.objects),
                "SM 配置": len(device.sm_configs),
                "RxPDO": len(device.rxpdo_assign),
                "TxPDO": len(device.txpdo_assign),
            })

            # ── 更新页2：原始身份 ──────────────
            self._identity_page.set_original_identity({
                "Vendor ID": f"0x{ident.vendor_id:08X}",
                "Product Code": f"0x{ident.product_code:08X}",
                "Revision": f"0x{ident.revision_number:08X}",
                "Device Name": ident.device_name,
            })

            # ── 更新页4：对象字典表 ────────────
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

            # ── 更新页5：PDO/SM 校验 ───────────
            self._update_pdo_page(device)

            self._progress.setValue(60)
            self._log_message(f"解析完成: {len(device.objects)} 个对象")
            self.statusBar().showMessage("解析完成")

        except Exception as e:
            self._log_message(f"解析失败: {e}")
            self.statusBar().showMessage("解析失败")
            self._progress.setValue(0)

    def _update_pdo_page(self, device):
        """更新页5的 SM 校验和 PDO 布局。"""
        sm_info = []
        for sm in device.sm_configs:
            calc = sm.total_byte_length
            status = "OK" if (sm.default_size == calc or calc == 0) else "MISMATCH"
            sm_info.append({
                "index": f"SM{sm.index}",
                "type": sm.pdo_type,
                "default_size": sm.default_size,
                "calculated_size": calc,
                "status": status,
            })
        self._pdo_page.set_sm_info(sm_info)

        lines = []
        for sm in device.sm_configs:
            for pdo in sm.pdos:
                lines.append(f"{pdo.name} (0x{pdo.index:04X})")
                bit_offset = 0
                for entry in pdo.entries:
                    lines.append(
                        f"  0x{entry.index:04X}:{entry.subindex:02X} "
                        f"{entry.bit_length}bit  offset {bit_offset}  {entry.name}"
                    )
                    bit_offset += entry.bit_length
                lines.append("")
        self._pdo_page.set_pdo_layout("\n".join(lines) if lines else "(无 PDO 布局信息)")

    # ═══════════════════════════════════════════
    # 信号处理：生成
    # ═══════════════════════════════════════════

    def _on_generate_all(self):
        """处理页6的 "全部生成" 按钮。"""
        if self._parsed_device is None:
            self._log_message("错误: 请先解析文件")
            return

        self._log_message("开始转换...")
        self._progress.setValue(10)

        # ── 从各页面收集配置 ──────────────────
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

        try:
            if identity.get("vendor_id"):
                config.vendor_id = int(identity["vendor_id"], 0)
            if identity.get("product_code"):
                config.product_code = int(identity["product_code"], 0)
            if identity.get("revision"):
                config.revision_number = int(identity["revision"], 0)
        except ValueError as e:
            self._log_message(f"身份值格式错误: {e}")
            return

        # ── 启动后台工作线程 ──────────────────
        self._worker = ConvertWorker(config)
        self._worker.log.connect(self._log_message)
        self._worker.progress.connect(self._progress.setValue)
        self._worker.finished_ok.connect(self._on_conversion_done)
        self._worker.failed.connect(self._on_conversion_failed)
        self._worker.start()

        self.statusBar().showMessage("转换中...")
        self._gen_page.set_status("转换进行中...")

    def _on_conversion_done(self, result):
        """转换成功完成。"""
        self._log_message(f"完成: {result.output_dir}")
        self._gen_page.set_status("转换完成!")
        self._progress.setValue(100)

        files = {}
        if result.xlsx_path:
            files["SSC xlsx"] = result.xlsx_path
        files.update(result.report_files)
        self._gen_page.set_output_files(files)
        self.statusBar().showMessage("转换完成")

    def _on_conversion_failed(self, error: str):
        """转换失败。"""
        self._log_message(f"失败: {error}")
        self._gen_page.set_status(f"失败: {error}")
        self._progress.setValue(0)
        self.statusBar().showMessage("转换失败")

    # ═══════════════════════════════════════════
    # 辅助
    # ═══════════════════════════════════════════

    def _log_message(self, msg: str):
        """向日志区追加一条消息。"""
        self._log.append(msg)
