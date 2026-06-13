"""
GUI entry point for NekoECAT Converter (Qt5).

TODO: Phase 4 — implement PyQt5 GUI.
Currently falls back to CLI.
"""
import sys


def main():
    try:
        from PyQt5.QtWidgets import QApplication
        # TODO: import and launch GUI
        print("GUI not yet implemented — use CLI: python cli.py convert --help")
        sys.exit(0)
    except ImportError:
        print("PyQt5 not installed. Use CLI: python cli.py convert --help")
        sys.exit(1)


if __name__ == "__main__":
    main()
