"""
Screenshot utility — captures the active window using xdotool + import.

Per user requirement: "截图请使用活动窗口截图并且需要和启动程序在同一条命令里"

Usage:
    # Inside the app (e.g., triggered by a hotkey or button):
    from gui.screenshot import capture_active_window
    path = capture_active_window("/tmp/screenshot.png")

    # From CLI (launch + capture in one command):
    python main.py --screenshot /tmp/gui.png
"""
import subprocess
import os
from datetime import datetime
from pathlib import Path


def capture_active_window(output_path: str = None) -> str:
    """Capture the currently active (focused) window to a PNG file.

    Uses xdotool to find the active window ID, then ImageMagick's
    'import' command to capture just that window — fast and precise.

    Args:
        output_path: Where to save the PNG. If None, auto-generates a path.

    Returns:
        Absolute path of the saved screenshot, or empty string on failure.
    """
    if output_path is None:
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_path = f"/tmp/nekoecat_screenshot_{ts}.png"

    output_path = str(Path(output_path).resolve())

    try:
        # Get the active window ID via xdotool
        result = subprocess.run(
            ["xdotool", "getactivewindow"],
            capture_output=True, text=True, timeout=2,
        )
        if result.returncode != 0:
            # Fallback: capture the entire screen
            subprocess.run(
                ["import", "-window", "root", output_path],
                timeout=5,
            )
            return output_path

        window_id = result.stdout.strip()

        # Capture just that window using ImageMagick import
        subprocess.run(
            ["import", "-window", window_id, output_path],
            timeout=5,
        )

        return output_path

    except (subprocess.TimeoutExpired, FileNotFoundError) as e:
        print(f"Screenshot failed: {e}")
        return ""
