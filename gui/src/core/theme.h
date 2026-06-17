/**
 * theme.h — 全局主题与样式常量 (Light Mode)。
 *
 * 设计: 清爽明亮风格
 *   - 白色/浅灰背景
 *   - 深色文字
 *   - 蓝色强调 (#2563eb)
 *   - 绿色成功、红色警告、橙色提醒
 *   - 圆角卡片式布局
 *
 * 解耦: theme 模块只提供样式数据, 不引用任何 UI 类。
 */
#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QFont>
#include <QPalette>

namespace Theme {

// ─── 颜色常量 (Light Mode) ─────────────────────────────────
// 主背景
inline constexpr auto BG_MAIN      = "#f5f7fa";
inline constexpr auto BG_SIDEBAR   = "#ffffff";
inline constexpr auto BG_INPUT     = "#f0f2f5";
inline constexpr auto BG_TABLE     = "#ffffff";
inline constexpr auto BG_TABLE_ALT = "#f8f9fb";
inline constexpr auto BG_LOG       = "#f0f2f5";
inline constexpr auto BG_CARD      = "#ffffff";

// 强调色
inline constexpr auto ACCENT       = "#2563eb";
inline constexpr auto ACCENT_HOVER = "#1d4ed8";
inline constexpr auto ACCENT_LIGHT = "#dbeafe";
inline constexpr auto ACCENT_GREEN = "#16a34a";
inline constexpr auto ACCENT_RED   = "#dc2626";
inline constexpr auto ACCENT_ORANGE = "#ea580c";
inline constexpr auto ACCENT_TEAL  = "#0d9488";

// 边框
inline constexpr auto BORDER       = "#e2e5ea";
inline constexpr auto BORDER_LIGHT = "#d1d5db";
inline constexpr auto BORDER_FOCUS = "#2563eb";

// 文字
inline constexpr auto TEXT_PRIMARY = "#1f2937";
inline constexpr auto TEXT_SEC     = "#6b7280";
inline constexpr auto TEXT_DIM     = "#9ca3af";
inline constexpr auto TEXT_WHITE   = "#ffffff";

// 表格行色
inline constexpr auto ROW_NORMAL   = "#ffffff";
inline constexpr auto ROW_ALT      = "#f8f9fb";
inline constexpr auto ROW_RISK     = "#fef2f2";
inline constexpr auto ROW_COE      = "#fffbeb";
inline constexpr auto ROW_PDO      = "#f0fdf4";
inline constexpr auto ROW_SKIPPED  = "#f3f4f6";

// ─── 字体 ──────────────────────────────────────────────────
QFont fontNormal();
QFont fontMono(int size = 11);
QFont fontTitle(int size = 18);
QFont fontStep(int size = 13);

// ─── 调色板 ────────────────────────────────────────────────
QPalette lightPalette();

// ─── 样式表 ────────────────────────────────────────────────
QString globalStyleSheet();
QString sidebarStyleSheet();
QString tableStyleSheet();
QString bottomBarStyleSheet();
QString genButtonStyleSheet();

} // namespace Theme

#endif // THEME_H
