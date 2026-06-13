"""
NekoECAT Converter — GUI entry point.

Launches PyQt5 main window.  Falls back to CLI if PyQt5 unavailable.

Usage:
    python main.py
"""
import sys


def main():
    try:
        from PyQt5.QtWidgets import QApplication
        from gui import MainWindow

        app = QApplication(sys.argv)
        app.setApplicationName("NekoECAT Converter")
        app.setStyle("Fusion")  # consistent cross-platform look

        window = MainWindow()
        window.show()

        sys.exit(app.exec_())

    except ImportError:
        print("PyQt5 not installed. Use CLI: python cli.py convert --help")
        sys.exit(1)


if __name__ == "__main__":
    main()
