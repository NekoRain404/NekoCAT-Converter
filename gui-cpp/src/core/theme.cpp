/**
 * theme.cpp — Light Mode 主题样式实现。
 */
#include "theme.h"

namespace Theme {

// ═══ 字体 ═══════════════════════════════════════════════════
QFont fontNormal() {
    QFont f("Noto Sans CJK SC", 13);
    f.setStyleStrategy(QFont::PreferAntialias);
    return f;
}

QFont fontMono(int size) {
    QFont f("Cascadia Code", size);
    f.setStyleHint(QFont::Monospace);
    return f;
}

QFont fontTitle(int size) {
    QFont f("Noto Sans CJK SC", size, QFont::Bold);
    f.setStyleStrategy(QFont::PreferAntialias);
    return f;
}

QFont fontStep(int size) {
    QFont f("Noto Sans CJK SC", size);
    f.setStyleStrategy(QFont::PreferAntialias);
    return f;
}

// ═══ 调色板 ═══════════════════════════════════════════════
QPalette lightPalette() {
    QPalette p;
    p.setColor(QPalette::Window,          QColor(BG_MAIN));
    p.setColor(QPalette::WindowText,      QColor(TEXT_PRIMARY));
    p.setColor(QPalette::Base,            QColor(BG_TABLE));
    p.setColor(QPalette::AlternateBase,   QColor(BG_TABLE_ALT));
    p.setColor(QPalette::Text,            QColor(TEXT_PRIMARY));
    p.setColor(QPalette::Button,          QColor(BG_SIDEBAR));
    p.setColor(QPalette::ButtonText,      QColor(TEXT_PRIMARY));
    p.setColor(QPalette::Highlight,       QColor(ACCENT));
    p.setColor(QPalette::HighlightedText, QColor(TEXT_WHITE));
    p.setColor(QPalette::PlaceholderText, QColor(TEXT_DIM));
    return p;
}

// ═══ 全局样式 ═════════════════════════════════════════════
QString globalStyleSheet() {
    return QStringLiteral(R"(
* {
    font-family: "Noto Sans CJK SC", "Segoe UI", sans-serif;
    font-size: 13px;
}
QMainWindow, QWidget {
    background: %1;
    color: %2;
}

/* ─── QLineEdit ─────────────────────────── */
QLineEdit {
    background: %3;
    border: 1px solid %4;
    border-radius: 6px;
    padding: 8px 12px;
    color: %2;
    font-size: 12px;
    selection-background-color: %5;
}
QLineEdit:focus {
    border: 1px solid %5;
    background: white;
}
QLineEdit:read-only {
    background: %6;
    color: %7;
}

/* ─── QGroupBox ─────────────────────────── */
QGroupBox {
    border: 1px solid %4;
    border-radius: 8px;
    margin-top: 16px;
    padding-top: 20px;
    font-weight: bold;
    color: %8;
    font-size: 12px;
    background: %9;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 8px;
    color: %8;
}

/* ─── QRadioButton / QCheckBox ──────────── */
QRadioButton, QCheckBox {
    color: %2;
    spacing: 8px;
    font-size: 12px;
}
QRadioButton::indicator {
    width: 16px; height: 16px;
    border: 2px solid %4;
    border-radius: 8px;
    background: white;
}
QRadioButton::indicator:checked {
    background: %5;
    border-color: %5;
}
QCheckBox::indicator {
    width: 16px; height: 16px;
    border: 2px solid %4;
    border-radius: 3px;
    background: white;
}
QCheckBox::indicator:checked {
    background: %5;
    border-color: %5;
}

/* ─── QComboBox ─────────────────────────── */
QComboBox {
    background: %3;
    border: 1px solid %4;
    border-radius: 6px;
    padding: 6px 12px;
    color: %2;
    font-size: 12px;
}
QComboBox:hover { border-color: %5; }
QComboBox:focus { border-color: %5; }
QComboBox::drop-down {
    border: none;
    width: 28px;
}
QComboBox QAbstractItemView {
    background: white;
    color: %2;
    border: 1px solid %4;
    border-radius: 6px;
    padding: 4px;
    selection-background-color: %5;
    selection-color: white;
}

/* ─── QToolTip ──────────────────────────── */
QToolTip {
    background: %2;
    color: %9;
    border: none;
    border-radius: 4px;
    padding: 6px 10px;
    font-size: 12px;
}

/* ─── QScrollBar ────────────────────────── */
QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: %4;
    min-height: 30px;
    border-radius: 4px;
}
QScrollBar::handle:vertical:hover { background: %5; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background: transparent;
    height: 8px;
}
QScrollBar::handle:horizontal {
    background: %4;
    min-width: 30px;
    border-radius: 4px;
}
QScrollBar::handle:horizontal:hover { background: %5; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

/* ─── QStatusBar ────────────────────────── */
QStatusBar {
    background: %6;
    color: %7;
    border-top: 1px solid %4;
    font-size: 11px;
    padding: 4px 12px;
}
)")
    .arg(BG_MAIN)           // %1
    .arg(TEXT_PRIMARY)       // %2
    .arg(BG_INPUT)          // %3
    .arg(BORDER)            // %4
    .arg(ACCENT)            // %5
    .arg(BG_SIDEBAR)        // %6
    .arg(TEXT_SEC)           // %7
    .arg(TEXT_PRIMARY)       // %8
    .arg(BG_CARD);          // %9
}

// ═══ 侧边栏样式 ═══════════════════════════════════════════
QString sidebarStyleSheet() {
    return QStringLiteral(R"(
QWidget#sidebar {
    background: %1;
    border-right: 1px solid %2;
}
QLabel#sidebarTitle {
    font-size: 20px;
    font-weight: bold;
    color: %3;
}
QLabel#sidebarSubtitle {
    font-size: 11px;
    color: %4;
}

/* ── 步骤按钮 ── */
QPushButton#stepBtn {
    background: transparent;
    text-align: left;
    padding: 16px 18px;
    border-radius: 8px;
    font-size: 13px;
    color: %5;
    border: 1px solid transparent;
}
QPushButton#stepBtn:hover {
    background: %6;
    border-color: %2;
}
QPushButton#stepBtn[active="true"] {
    background: %7;
    color: %8;
    font-weight: bold;
    border: 1px solid %8;
}

/* ── 步骤编号圆圈 ── */
QLabel#stepNum {
    background: %2;
    color: %4;
    border-radius: 12px;
    font-weight: bold;
    font-size: 12px;
    min-width: 24px; max-width: 24px;
    min-height: 24px; max-height: 24px;
    qproperty-alignment: AlignCenter;
}
QLabel#stepNum[active="true"] {
    background: %8;
    color: white;
}

/* ── 导入按钮 ── */
QPushButton#importBtn {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 %8, stop:1 %9);
    color: white;
    border: none;
    border-radius: 8px;
    padding: 14px;
    font: bold 14px "Noto Sans CJK SC", sans-serif;
}
QPushButton#importBtn:hover {
    background: %9;
}
QPushButton#importBtn:pressed {
    background: #1e40af;
}
QPushButton#importBtn:disabled {
    background: #d1d5db;
    color: #9ca3af;
}

/* ── 分隔线 ── */
QFrame#separator {
    background: %2;
    max-height: 1px;
}
)")
    .arg(BG_SIDEBAR)        // %1
    .arg(BORDER)            // %2
    .arg(ACCENT)            // %3 (title)
    .arg(TEXT_SEC)           // %4
    .arg(TEXT_PRIMARY)       // %5
    .arg(BG_INPUT)          // %6 (hover)
    .arg(ACCENT_LIGHT)      // %7 (active bg)
    .arg(ACCENT)            // %8 (active text/accent)
    .arg(ACCENT_HOVER);     // %9 (hover accent)
}

// ═══ 表格样式 ═════════════════════════════════════════════
QString tableStyleSheet() {
    return QStringLiteral(R"(
QTableWidget {
    background: %1;
    alternate-background-color: %2;
    color: %3;
    gridline-color: %4;
    border: 1px solid %4;
    border-radius: 8px;
    font-size: 12px;
    selection-background-color: %5;
    selection-color: white;
    outline: none;
}
QTableWidget::item {
    padding: 6px 10px;
    border: none;
}
QTableWidget::item:selected {
    background: %5;
    color: white;
}
QHeaderView::section {
    background: %6;
    color: %7;
    border: none;
    border-bottom: 2px solid %5;
    border-right: 1px solid %4;
    padding: 10px 12px;
    font-weight: bold;
    font-size: 12px;
}
QHeaderView::section:last {
    border-right: none;
}
QHeaderView::section:hover {
    background: %5;
    color: white;
}
)")
    .arg(BG_TABLE)          // %1
    .arg(BG_TABLE_ALT)      // %2
    .arg(TEXT_PRIMARY)       // %3
    .arg(BORDER)            // %4
    .arg(ACCENT)            // %5
    .arg(BG_INPUT)          // %6 (header bg)
    .arg(TEXT_SEC);          // %7 (header text)
}

// ═══ 底部工具栏 ═══════════════════════════════════════════
QString bottomBarStyleSheet() {
    return QStringLiteral(R"(
QWidget#bottomBar {
    background: %1;
    border-top: 1px solid %2;
}
QTextEdit#log {
    background: %3;
    border: 1px solid %2;
    color: %4;
    font-family: "Cascadia Code", "Consolas", monospace;
    font-size: 11px;
    border-radius: 6px;
    padding: 6px;
}
QTextEdit#pdo {
    background: %3;
    border: 1px solid %2;
    color: %5;
    font-family: "Cascadia Code", "Consolas", monospace;
    font-size: 11px;
    border-radius: 6px;
    padding: 6px;
}
QProgressBar {
    border: 1px solid %2;
    border-radius: 6px;
    text-align: center;
    color: white;
    font-size: 11px;
    font-weight: bold;
    background: %3;
    max-height: 16px;
    min-height: 16px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 %6, stop:1 %7);
    border-radius: 5px;
}
)")
    .arg(BG_SIDEBAR)        // %1
    .arg(BORDER)            // %2
    .arg(BG_INPUT)          // %3
    .arg(TEXT_SEC)           // %4
    .arg(ACCENT_TEAL)       // %5
    .arg(ACCENT)            // %6
    .arg(ACCENT_GREEN);     // %7
}

// ═══ 生成按钮组 ═══════════════════════════════════════════
QString genButtonStyleSheet() {
    return QStringLiteral(R"(
QPushButton#genBtn {
    background: %1;
    color: %2;
    border: 1px solid %3;
    border-radius: 6px;
    padding: 10px 20px;
    font-size: 12px;
    font-weight: bold;
}
QPushButton#genBtn:hover {
    background: %4;
    border-color: %4;
    color: white;
}
QPushButton#genBtn:pressed {
    background: #1e40af;
}
QPushButton#genBtnAll {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 %5, stop:1 %6);
    color: white;
    border: none;
    border-radius: 6px;
    padding: 10px 24px;
    font: bold 13px "Noto Sans CJK SC", sans-serif;
}
QPushButton#genBtnAll:hover {
    background: %6;
}
QPushButton#genBtnAll:pressed {
    background: #1e40af;
}
)")
    .arg(BG_INPUT)          // %1
    .arg(TEXT_PRIMARY)       // %2
    .arg(BORDER)            // %3
    .arg(ACCENT)            // %4
    .arg(ACCENT)            // %5
    .arg(ACCENT_HOVER);     // %6
}

} // namespace Theme
