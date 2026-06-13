"""
NekoECAT Converter — GUI entry point.

Launches PyQt5 main window with optional screenshot capture.

Usage:
    python main.py                          # just launch
    python main.py --screenshot /tmp/gui.png  # launch + capture active window
"""
import sys
import argparse


def main():
    parser = argparse.ArgumentParser(description="NekoECAT Converter GUI")
    parser.add_argument(
        "--screenshot", "-s",
        metavar="PATH",
        help="Capture active window to PNG after launch (uses xdotool + import)",
    )
    args = parser.parse_args()

    try:
        from PyQt5.QtWidgets import QApplication
        from PyQt5.QtCore import QTimer
        from gui import MainWindow

        app = QApplication(sys.argv)
        app.setApplicationName("NekoECAT Converter")
        app.setStyle("Fusion")

        window = MainWindow()
        window.show()

        # If --screenshot requested, capture after a short delay
        # so the window is fully rendered and focused
        if args.screenshot:
            def _take_screenshot():
                from gui.screenshot import capture_active_window
                path = capture_active_window(args.screenshot)
                if path:
                    print(f"Screenshot saved: {path}")
                else:
                    print("Screenshot failed")

            # 500ms delay ensures the window is painted and focused
            QTimer.singleShot(500, _take_screenshot)

        sys.exit(app.exec_())

    except ImportError:
        print("PyQt5 not installed. Use CLI: python cli.py convert --help")
        sys.exit(1)


if __name__ == "__main__":
    main()
