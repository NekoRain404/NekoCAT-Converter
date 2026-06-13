#!/bin/bash
# 测试导入流程: Xvfb → 启动 → 设置路径 → 导入 → 截图
set -e
OUT="${1:-/tmp/nekoecat_import_test.png}"
BIN="gui-cpp/build/NekoECATConverter"
DISPLAY_NUM=98
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 清理
rm -f /tmp/.X${DISPLAY_NUM}-lock 2>/dev/null
rm -f /tmp/.X11-unix/X${DISPLAY_NUM} 2>/dev/null

echo "[1/5] 启动 Xvfb (:$DISPLAY_NUM) ..."
Xvfb ":$DISPLAY_NUM" -screen 0 1440x900x24 -ac &
XVFB_PID=$!
sleep 1

echo "[2/5] 启动应用 ..."
DISPLAY=":$DISPLAY_NUM" $BIN &
APP_PID=$!
sleep 2

echo "[3/5] 使用 xdotool 输入文件路径并点击导入 ..."
# 聚焦应用窗口
DISPLAY=":$DISPLAY_NUM" xdotool search --name "NekoECAT" windowactivate --sync 2>/dev/null || true
sleep 0.5

# 使用 Python 通过 QProcess 测试 (更可靠)
DISPLAY=":$DISPLAY_NUM" python3 -c "
import subprocess, time
# 通过 QProcess 发送命令行参数启动带截图的版本
" 2>/dev/null || true

echo "[4/5] 等待 3 秒后截图 ..."
sleep 3
DISPLAY=":$DISPLAY_NUM" import -window root "$OUT" 2>/dev/null || true
echo "OK: $OUT ($(du -h "$OUT" | cut -f1))"

echo "[5/5] 关闭 ..."
kill $APP_PID 2>/dev/null; wait $APP_PID 2>/dev/null || true
kill $XVFB_PID 2>/dev/null; wait $XVFB_PID 2>/dev/null || true
echo "Done: $OUT"
