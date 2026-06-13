#!/bin/bash
# 一键截图: Xvfb → 启动(--screenshot) → 自动截图并退出
# 用法: ./screenshot.sh [输出路径]
set -e
OUT="${1:-/tmp/nekoecat_screenshot.png}"
BIN="gui-cpp/build/NekoECATConverter"
DISPLAY_NUM=99

# 清理旧的 Xvfb
rm -f /tmp/.X${DISPLAY_NUM}-lock 2>/dev/null
rm -f /tmp/.X11-unix/X${DISPLAY_NUM} 2>/dev/null

echo "[1/3] 启动 Xvfb (:$DISPLAY_NUM) ..."
Xvfb ":$DISPLAY_NUM" -screen 0 1440x900x24 -ac &
XVFB_PID=$!
sleep 1

echo "[2/3] 启动 $BIN --screenshot $OUT ..."
DISPLAY=":$DISPLAY_NUM" $BIN --screenshot "$OUT"
echo "OK: $OUT ($(du -h "$OUT" | cut -f1))"

echo "[3/3] 关闭 Xvfb ..."
kill $XVFB_PID 2>/dev/null; wait $XVFB_PID 2>/dev/null || true
echo "Done: $OUT"
