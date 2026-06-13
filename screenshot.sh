#!/bin/bash
# 一键截图：启动 → 等待渲染 → 截图 → 关闭
# 用法: ./screenshot.sh [输出路径]
set -e
OUT="${1:-/tmp/nekoecat_screenshot.png}"
BIN="gui-cpp/build/NekoECATConverter"

echo "[1/3] 启动 $BIN ..."
./$BIN &
PID=$!
sleep 2

echo "[2/3] 截图 → $OUT"
# 用 spectacle 截活动窗口
spectacle -b -o "$OUT" 2>&1 || {
    echo "spectacle 失败，尝试 import"
    import -window root "$OUT" 2>&1
}
echo "OK: $OUT ($(du -h "$OUT" | cut -f1))"

echo "[3/3] 关闭进程 $PID"
kill $PID 2>/dev/null
wait $PID 2>/dev/null || true
echo "Done"
