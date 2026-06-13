"""
ConvertWorker — 后台转换工作线程。

Per dev doc §13: 在 QThread 中运行完整转换管线，
通过信号将进度/日志/结果发回 GUI。

解耦原则：
  只依赖 nekoecat.core（门面），不直接导入 parser/engine/generator。
"""
from PyQt5.QtCore import QThread, pyqtSignal
from nekoecat.config import ConvertConfig
from nekoecat.core import convert, ConvertResult


class ConvertWorker(QThread):
    """后台转换工作线程。

    信号：
        log(str)           — 日志消息（显示在 GUI 日志区）
        progress(int)      — 进度百分比 0-100
        finished_ok(object) — ConvertResult 对象（成功时）
        failed(str)        — 错误消息（失败时）
    """
    log = pyqtSignal(str)
    progress = pyqtSignal(int)
    finished_ok = pyqtSignal(object)
    failed = pyqtSignal(str)

    def __init__(self, config: ConvertConfig, parent=None):
        super().__init__(parent)
        self.config = config

    def run(self):
        """执行转换管线。由 QThread.start() 调用。"""
        try:
            self.log.emit("开始转换...")
            self.progress.emit(5)

            self.log.emit(f"ESI: {self.config.esi_path or '(无)'}")
            self.log.emit(f"SDO: {self.config.sdo_path or '(无)'}")
            self.progress.emit(10)

            # 调用 core 门面 —— 唯一入口
            result = convert(self.config)

            self.progress.emit(100)

            if result.success:
                self.log.emit(f"转换完成: {result.output_dir}")
                self.finished_ok.emit(result)
            else:
                self.log.emit(f"转换失败: {result.error}")
                self.failed.emit(result.error or "未知错误")

        except Exception as e:
            self.log.emit(f"异常: {e}")
            self.failed.emit(str(e))
