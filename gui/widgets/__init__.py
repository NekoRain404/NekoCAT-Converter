"""
gui.widgets — 各页面组件。

每个页面是一个独立的 QWidget 子类，
通过信号与 MainWindow 通信，页面之间不直接耦合。
"""
from .file_select_page import FileSelectPage
from .identity_page import IdentityPage
from .rule_page import RulePage
from .object_table_page import ObjectTablePage
from .pdo_page import PdoPage
from .generate_page import GeneratePage

__all__ = [
    "FileSelectPage",
    "IdentityPage",
    "RulePage",
    "ObjectTablePage",
    "PdoPage",
    "GeneratePage",
]
