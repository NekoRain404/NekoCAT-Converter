/**
 * MainWindow — 完整的专业级主窗口实现。
 *
 * 参考 docs/ui/ 参考图的布局和交互。
 * 侧边栏分步导航，右侧内容区实时更新。
 */
#include "mainwindow.h"
#include "worker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QDir>
#include <QMargins>
#include <QFileDialog>
#include <QHeaderView>
#include <QFont>
#include <QBrush>
#include <QColor>

// ═════════════════════════════════════════════
// 深色主题样式 (参考 UI 图)
// ═════════════════════════════════════════════
static const char *DARK_STYLE = R"(
* { font-family: "Segoe UI", "Noto Sans CJK SC", sans-serif; }
QMainWindow, QWidget { background-color: #1a1a2e; color: #e0e0e0; font-size: 13px; }

/* 侧边栏 */
QWidget#sidebar { background-color: #16213e; border-right: 1px solid #0f3460; }
QLabel#appTitle { font-size: 18px; font-weight: bold; color: #e94560; }
QLabel#appSubtitle { font-size: 11px; color: #8899aa; }
QLabel#stepLabel { font-size: 10px; font-weight: bold; color: #53a8b6; text-transform: uppercase; }

QPushButton#stepBtn {
    background-color: transparent; text-align: left;
    padding: 12px 16px; border-radius: 8px; font-size: 13px; color: #aabbcc;
    border: 1px solid transparent;
}
QPushButton#stepBtn:hover { background-color: #1a1a3e; border: 1px solid #0f3460; }
QPushButton#stepBtn[active="true"] {
    background-color: #0f3460; color: #e94560; font-weight: bold;
    border: 1px solid #e94560;
}

QPushButton#importBtn {
    background-color: #e94560; color: white; border: none;
    border-radius: 8px; padding: 12px 24px; font-size: 14px; font-weight: bold;
}
QPushButton#importBtn:hover { background-color: #c73e54; }
QPushButton#importBtn:pressed { background-color: #a83246; }
QPushButton#importBtn:disabled { background-color: #444; color: #888; }

QPushButton#generateBtn {
    background-color: #0f3460; color: #e0e0e0; border: 1px solid #53a8b6;
    border-radius: 6px; padding: 8px 16px; font-size: 12px;
}
QPushButton#generateBtn:hover { background-color: #1a4a8a; border-color: #e94560; }

/* 输入框 */
QLineEdit {
    background-color: #0f3460; border: 1px solid #1a4a8a;
    border-radius: 4px; padding: 6px 10px; color: #e0e0e0; font-size: 12px;
}
QLineEdit:focus { border: 1px solid #e94560; }
QLineEdit:read-only { background-color: #16213e; color: #8899aa; }

/* 文件状态指示 */
QLabel#fileStatus { font-size: 11px; color: #53a8b6; padding: 4px; }
QLabel#fileStatus[ready="true"] { color: #4caf50; }

/* 表格 */
QTableWidget {
    background-color: #0f1225; alternate-background-color: #131730;
    color: #e0e0e0; gridline-color: #1a2a4a; border: 1px solid #1a2a4a;
    border-radius: 6px; font-size: 12px;
}
QTableWidget::item { padding: 6px 8px; }
QTableWidget::item:selected { background-color: #0f3460; }
QHeaderView::section {
    background-color: #16213e; color: #53a8b6; border: none;
    border-bottom: 2px solid #0f3460; padding: 8px; font-weight: bold;
    font-size: 11px;
}

/* 过滤器 */
QComboBox {
    background-color: #0f3460; border: 1px solid #1a4a8a;
    border-radius: 4px; padding: 4px 8px; color: #e0e0e0; font-size: 12px;
}
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView { background-color: #16213e; color: #e0e0e0; }

/* 分组框 */
QGroupBox {
    border: 1px solid #1a2a4a; border-radius: 6px;
    margin-top: 12px; padding-top: 16px; font-weight: bold; color: #53a8b6;
    font-size: 12px;
}
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }

/* 单选按钮 */
QRadioButton { color: #c0c0c0; spacing: 8px; font-size: 12px; }
QRadioButton::indicator { width: 16px; height: 16px; border: 2px solid #1a4a8a; border-radius: 8px; }
QRadioButton::indicator:checked { background-color: #e94560; border-color: #e94560; }

/* 复选框 */
QCheckBox { color: #c0c0c0; spacing: 8px; font-size: 12px; }

/* 日志 */
QTextEdit#logArea {
    background-color: #0a0e1a; border: 1px solid #1a2a4a;
    color: #8899aa; font-family: "Cascadia Code", Consolas, monospace;
    font-size: 11px; border-radius: 4px;
}

/* 进度条 */
QProgressBar {
    border: 1px solid #1a2a4a; border-radius: 6px;
    text-align: center; color: white; font-size: 11px; background-color: #0f1225;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #e94560, stop:1 #53a8b6);
    border-radius: 5px;
}

/* 状态栏 */
QStatusBar { background-color: #16213e; color: #8899aa; border-top: 1px solid #1a2a4a; }

/* PDO 文本 */
QTextEdit#pdoArea {
    background-color: #0a0e1a; border: 1px solid #1a2a4a;
    color: #53a8b6; font-family: "Cascadia Code", Consolas, monospace;
    font-size: 11px; border-radius: 4px;
}
)";

// ═════════════════════════════════════════════
// 构造函数
// ═════════════════════════════════════════════

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_worker(nullptr)
{
    setWindowTitle("NekoECAT Converter");
    setMinimumSize(1200, 750);
    resize(1400, 850);
    applyDarkTheme();

    // ══ 左侧边栏 ═══════════════════════════════
    QWidget *sidebar = buildSidebar();

    // ══ 右侧内容区 ═════════════════════════════
    QWidget *rightPanel = buildRightPanel();

    // ══ 底部栏 ═════════════════════════════════
    QWidget *bottomBar = buildBottomBar();

    // ══ 组装布局 ═══════════════════════════════
    QWidget *rightSide = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightSide);
    rightLayout->setContentsMargins(QMargins(0, 0, 0, 0));
    rightLayout->setSpacing(0);
    rightLayout->addWidget(rightPanel, 1);
    rightLayout->addWidget(bottomBar, 0);

    QWidget *mainWidget = new QWidget;
    auto *mainLayout = new QHBoxLayout(mainWidget);
    mainLayout->setContentsMargins(QMargins(0, 0, 0, 0));
    mainLayout->setSpacing(0);
    mainLayout->addWidget(sidebar, 0);
    mainLayout->addWidget(rightSide, 1);

    setCentralWidget(mainWidget);
    statusBar()->showMessage("就绪 — 请导入 ESI/SDO 文件开始");

    connectSignals();
}

// ═════════════════════════════════════════════
// 主题
// ═════════════════════════════════════════════

void MainWindow::applyDarkTheme() {
    setStyleSheet(DARK_STYLE);
}

// ═════════════════════════════════════════════
// 侧边栏构建
// ═════════════════════════════════════════════

QWidget *MainWindow::buildSidebar() {
    QWidget *sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(300);

    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(QMargins(16, 20, 16, 20));
    layout->setSpacing(6);

    // ── 应用标题 ────────────────────────────
    auto *title = new QLabel("NekoECAT Converter");
    title->setObjectName("appTitle");
    auto *subtitle = new QLabel("ECAT → SSC Tool");
    subtitle->setObjectName("appSubtitle");
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(8);

    // ── 步骤导航按钮 ────────────────────────
    struct StepDef { const char *label; const char *icon; };
    StepDef steps[] = {
        {"Step 1: 文件导入", "📁"},
        {"Step 2: 身份配置", "🏷"},
        {"Step 3: 规则策略", "⚙"},
    };
    for (int i = 0; i < 3; ++i) {
        m_stepButtons[i] = new QPushButton(QString("%1 %2").arg(steps[i].icon, steps[i].label));
        m_stepButtons[i]->setObjectName("stepBtn");
        m_stepButtons[i]->setCursor(Qt::PointingHandCursor);
        int idx = i;
        connect(m_stepButtons[i], &QPushButton::clicked, this, [this, idx]() { onStepChanged(idx); });
        layout->addWidget(m_stepButtons[i]);
    }

    layout->addSpacing(8);

    // ── 步骤内容（QStackedWidget） ───────────
    m_stepStack = new QStackedWidget;
    m_stepStack->addWidget(buildStep1FileImport());
    m_stepStack->addWidget(buildStep2Identity());
    m_stepStack->addWidget(buildStep3Strategy());
    layout->addWidget(m_stepStack, 1);

    layout->addSpacing(8);

    // ── 导入按钮 ────────────────────────────
    m_importBtn = new QPushButton("📥  导入并解析");
    m_importBtn->setObjectName("importBtn");
    m_importBtn->setMinimumHeight(44);
    connect(m_importBtn, &QPushButton::clicked, this, &MainWindow::onImport);
    layout->addWidget(m_importBtn);

    // ── 文件状态 ────────────────────────────
    m_fileStatus = new QLabel("未选择文件");
    m_fileStatus->setObjectName("fileStatus");
    layout->addWidget(m_fileStatus);

    layout->addStretch();

    // ── 版本 ─────────────────────────────────
    auto *ver = new QLabel("v0.1.0  |  C++ Qt5");
    ver->setStyleSheet("color: #445566; font-size: 10px;");
    layout->addWidget(ver);

    onStepChanged(0);
    return sidebar;
}

// ═════════════════════════════════════════════
// Step 1: 文件导入
// ═════════════════════════════════════════════

QWidget *MainWindow::buildStep1FileImport() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(QMargins(0, 0, 0, 0));
    layout->setSpacing(8);

    auto addFileRow = [&](const char *label, const char *filter, QLineEdit *&edit) {
        auto *row = new QWidget;
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(QMargins(0, 0, 0, 0));
        h->setSpacing(4);
        auto *lbl = new QLabel(label);
        lbl->setFixedWidth(60);
        lbl->setStyleSheet("color: #8899aa; font-size: 11px;");
        edit = new QLineEdit;
        edit->setPlaceholderText("点击浏览...");
        auto *btn = new QPushButton("...");
        btn->setFixedWidth(32);
        connect(btn, &QPushButton::clicked, this, [this, edit, filter]() {
            QString path = QFileDialog::getOpenFileName(this, "选择文件", "", filter);
            if (!path.isEmpty()) edit->setText(path);
        });
        h->addWidget(lbl);
        h->addWidget(edit, 1);
        h->addWidget(btn);
        layout->addWidget(row);
    };

    addFileRow("ESI:", "XML files (*.xml);;All (*)", m_esiEdit);
    addFileRow("SDO:", "XML/TXT files (*.xml *.txt);;All (*)", m_sdoEdit);
    addFileRow("模板:", "Excel files (*.xlsx);;All (*)", m_tplEdit);

    // 输出目录
    auto *outRow = new QWidget;
    auto *outH = new QHBoxLayout(outRow);
    outH->setContentsMargins(QMargins(0, 0, 0, 0));
    outH->setSpacing(4);
    auto *outLbl = new QLabel("输出:");
    outLbl->setFixedWidth(60);
    outLbl->setStyleSheet("color: #8899aa; font-size: 11px;");
    m_outEdit = new QLineEdit("out");
    auto *outBtn = new QPushButton("...");
    outBtn->setFixedWidth(32);
    connect(outBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, "选择输出目录");
        if (!path.isEmpty()) m_outEdit->setText(path);
    });
    outH->addWidget(outLbl);
    outH->addWidget(m_outEdit, 1);
    outH->addWidget(outBtn);
    layout->addWidget(outRow);

    return page;
}

// ═════════════════════════════════════════════
// Step 2: 身份配置
// ═════════════════════════════════════════════

QWidget *MainWindow::buildStep2Identity() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(QMargins(0, 0, 0, 0));
    layout->setSpacing(8);

    // 原始身份
    auto *origGroup = new QGroupBox("原始身份");
    auto *origGrid = new QGridLayout(origGroup);
    origGrid->setSpacing(4);
    const char *origKeys[] = {"厂商:", "产品:", "修订:", "名称:"};
    QLabel **origLabels[] = {&m_origVendor, &m_origProduct, &m_origRevision, &m_origName};
    for (int i = 0; i < 4; ++i) {
        auto *lbl = new QLabel(origKeys[i]);
        lbl->setStyleSheet("color: #667788; font-size: 11px;");
        *origLabels[i] = new QLabel("-");
        (*origLabels[i])->setStyleSheet("color: #8899aa; font-size: 11px;");
        origGrid->addWidget(lbl, i, 0);
        origGrid->addWidget(*origLabels[i], i, 1);
    }
    layout->addWidget(origGroup);

    // 新身份
    auto *newGroup = new QGroupBox("新身份 (覆盖)");
    auto *newGrid = new QGridLayout(newGroup);
    newGrid->setSpacing(4);
    auto addField = [&](int row, const char *label, QLineEdit *&edit, const char *ph = "") {
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet("color: #667788; font-size: 11px;");
        edit = new QLineEdit;
        edit->setPlaceholderText(ph);
        newGrid->addWidget(lbl, row, 0);
        newGrid->addWidget(edit, row, 1);
    };
    addField(0, "厂商ID:", m_newVendor, "0x...");
    addField(1, "产品码:", m_newProduct, "0x...");
    addField(2, "修订:", m_newRevision, "0x...");
    addField(3, "名称:", m_newName, "设备名");
    addField(4, "Physics:", m_newPhysics, "YY");
    layout->addWidget(newGroup);

    // 模式选择
    auto *modeGroup = new QGroupBox("模式");
    auto *modeH = new QHBoxLayout(modeGroup);
    m_modeGroup = new QButtonGroup(this);
    const char *modes[] = {"学习", "实验室", "产品"};
    for (int i = 0; i < 3; ++i) {
        auto *rb = new QRadioButton(modes[i]);
        m_modeGroup->addButton(rb, i);
        modeH->addWidget(rb);
    }
    m_modeGroup->button(0)->setChecked(true);
    layout->addWidget(modeGroup);

    return page;
}

// ═════════════════════════════════════════════
// Step 3: 规则策略
// ═════════════════════════════════════════════

QWidget *MainWindow::buildStep3Strategy() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(QMargins(0, 0, 0, 0));
    layout->setSpacing(6);

    auto addRadioGroup = [&](const char *title, QButtonGroup *&group, const char *opts[], int count) {
        auto *grp = new QGroupBox(title);
        auto *v = new QVBoxLayout(grp);
        v->setSpacing(2);
        group = new QButtonGroup(this);
        for (int i = 0; i < count; ++i) {
            auto *rb = new QRadioButton(opts[i]);
            group->addButton(rb, i);
            v->addWidget(rb);
        }
        group->button(0)->setChecked(true);
        layout->addWidget(grp);
    };

    const char *octetOpts[] = {"STRING + callback ✓", "跳过", "手动"};
    addRadioGroup("OCTET_STRING", m_octetGroup, octetOpts, 3);

    const char *alignOpts[] = {"CoE callback ✓", "padding", "跳过"};
    addRadioGroup("未对齐对象", m_alignGroup, alignOpts, 3);

    const char *commOpts[] = {"SSC 自动 ✓", "强制导入", "跳过"};
    addRadioGroup("通信对象", m_commGroup, commOpts, 3);

    // Fxxx
    auto *fxxxGroup = new QGroupBox("Fxxx 对象");
    auto *fxxxH = new QHBoxLayout(fxxxGroup);
    m_fxxxGroup = new QButtonGroup(this);
    const char *fxxxOpts[] = {"包含", "跳过", "callback stub"};
    for (int i = 0; i < 3; ++i) {
        auto *rb = new QRadioButton(fxxxOpts[i]);
        m_fxxxGroup->addButton(rb, i);
        fxxxH->addWidget(rb);
    }
    m_fxxxGroup->button(0)->setChecked(true);
    layout->addWidget(fxxxGroup);

    return page;
}

// ═════════════════════════════════════════════
// 右侧内容区
// ═════════════════════════════════════════════

QWidget *MainWindow::buildRightPanel() {
    auto *panel = new QWidget;
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(QMargins(12, 12, 12, 0));
    layout->setSpacing(8);

    // ── 对象表格区 ──────────────────────────
    layout->addWidget(buildObjectTable(), 2);

    // ── PDO/SM 校验区 ───────────────────────
    layout->addWidget(buildPdoPanel(), 1);

    return panel;
}

QWidget *MainWindow::buildObjectTable() {
    auto *grp = new QGroupBox("对象字典");
    auto *layout = new QVBoxLayout(grp);

    // 过滤栏
    auto *filterRow = new QHBoxLayout;
    auto *filterLbl = new QLabel("过滤:");
    filterLbl->setStyleSheet("color: #8899aa;");
    m_filterCombo = new QComboBox;
    m_filterCombo->addItems({"全部", "仅错误", "仅 callback", "仅 PDO", "仅 Fxxx"});
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { /* 过滤逻辑在 updateObjectTable 中处理 */ });
    m_objCountLabel = new QLabel("0 对象");
    m_objCountLabel->setStyleSheet("color: #53a8b6;");
    filterRow->addWidget(filterLbl);
    filterRow->addWidget(m_filterCombo);
    filterRow->addStretch();
    filterRow->addWidget(m_objCountLabel);
    layout->addLayout(filterRow);

    // 表格
    m_objTable = new QTableWidget;
    m_objTable->setColumnCount(9);
    QStringList headers = {"Index", "Name", "Sub", "Category", "PDO", "SM", "Risk", "CoeR", "CoeW"};
    m_objTable->setHorizontalHeaderLabels(headers);
    m_objTable->setEditTriggers(QTableWidget::NoEditTriggers);
    m_objTable->setSelectionBehavior(QTableWidget::SelectRows);
    m_objTable->setAlternatingRowColors(true);
    m_objTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_objTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_objTable->verticalHeader()->setVisible(false);
    m_objTable->setShowGrid(false);
    layout->addWidget(m_objTable);

    return grp;
}

QWidget *MainWindow::buildPdoPanel() {
    auto *grp = new QGroupBox("PDO / SM 校验");
    auto *layout = new QVBoxLayout(grp);
    m_pdoText = new QTextEdit;
    m_pdoText->setObjectName("pdoArea");
    m_pdoText->setReadOnly(true);
    m_pdoText->setPlaceholderText("解析文件后显示 PDO/SM 校验信息...");
    layout->addWidget(m_pdoText);
    return grp;
}

// ═════════════════════════════════════════════
// 底部栏
// ═════════════════════════════════════════════

QWidget *MainWindow::buildBottomBar() {
    auto *bar = new QWidget;
    bar->setStyleSheet("background-color: #16213e; border-top: 1px solid #1a2a4a;");
    auto *layout = new QVBoxLayout(bar);
    layout->setContentsMargins(QMargins(12, 8, 12, 8));
    layout->setSpacing(6);

    // 生成按钮行
    auto *btnRow = new QHBoxLayout;
    struct BtnDef { const char *label; int mode; };
    BtnDef btns[] = {
        {"🚀 全部生成", 0}, {"📊 SSC xlsx", 1}, {"📋 报告", 2}, {"📄 ESI XML", 3},
    };
    for (auto &b : btns) {
        auto *btn = new QPushButton(b.label);
        btn->setObjectName("generateBtn");
        btn->setMinimumWidth(130);
        int mode = b.mode;
        connect(btn, &QPushButton::clicked, this, [this, mode]() { onGenerate(mode); });
        btnRow->addWidget(btn);
    }
    btnRow->addStretch();
    layout->addLayout(btnRow);

    // 进度条
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setMaximumHeight(14);
    layout->addWidget(m_progress);

    // 日志
    m_log = new QTextEdit;
    m_log->setObjectName("logArea");
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(80);
    m_log->setPlaceholderText("操作日志...");
    layout->addWidget(m_log);

    return bar;
}

// ═════════════════════════════════════════════
// 信号连接
// ═════════════════════════════════════════════

void MainWindow::connectSignals() {
    // 文件路径变化 → 更新状态
    auto checkFiles = [this]() {
        bool hasEsi = !m_esiEdit->text().trimmed().isEmpty();
        bool hasSdo = !m_sdoEdit->text().trimmed().isEmpty();
        m_importBtn->setEnabled(hasEsi || hasSdo);
        if (hasEsi || hasSdo) {
            m_fileStatus->setText(QString("已选择: %1 %2")
                .arg(hasEsi ? "ESI" : "", hasSdo ? "SDO" : ""));
            m_fileStatus->setProperty("ready", true);
        } else {
            m_fileStatus->setText("未选择文件");
            m_fileStatus->setProperty("ready", false);
        }
        m_fileStatus->style()->unpolish(m_fileStatus);
        m_fileStatus->style()->polish(m_fileStatus);
    };
    connect(m_esiEdit, &QLineEdit::textChanged, this, checkFiles);
    connect(m_sdoEdit, &QLineEdit::textChanged, this, checkFiles);
}

// ═════════════════════════════════════════════
// 步骤切换
// ═════════════════════════════════════════════

void MainWindow::onStepChanged(int step) {
    m_stepStack->setCurrentIndex(step);
    for (int i = 0; i < 3; ++i) {
        m_stepButtons[i]->setProperty("active", i == step);
        m_stepButtons[i]->style()->unpolish(m_stepButtons[i]);
        m_stepButtons[i]->style()->polish(m_stepButtons[i]);
    }
}

// ═════════════════════════════════════════════
// 导入解析
// ═════════════════════════════════════════════

void MainWindow::onImport() {
    QString esi = m_esiEdit->text().trimmed();
    QString sdo = m_sdoEdit->text().trimmed();
    if (esi.isEmpty() && sdo.isEmpty()) {
        setStatus("错误: 请至少选择一个 ESI 或 SDO 文件");
        return;
    }

    setStatus("正在解析...");
    m_progress->setValue(10);
    m_log->append("[导入] 开始解析...");

    // 获取项目根目录
    QString projectRoot = QDir::currentPath();

    // Python 内联脚本：解析并输出 JSON
    QString pyScript = R"(
import sys, json
sys.path.insert(0, '.')
from nekoecat.core import parse_only
esi = sys.argv[1] if sys.argv[1] != 'None' else None
sdo = sys.argv[2] if sys.argv[2] != 'None' else None
device = parse_only(esi_path=esi, sdo_path=sdo)
print(json.dumps(device.model_dump(mode='json'), ensure_ascii=False))
)";

    QProcess proc;
    proc.setWorkingDirectory(projectRoot);
    proc.start("python3", {"-c", pyScript,
        esi.isEmpty() ? "None" : esi,
        sdo.isEmpty() ? "None" : sdo});
    proc.waitForFinished(30000);

    if (proc.exitCode() != 0) {
        QString err = QString::fromUtf8(proc.readAllStandardError()).trimmed();
        m_log->append("[错误] " + err);
        setStatus("解析失败");
        m_progress->setValue(0);
        return;
    }

    m_deviceJson = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    m_progress->setValue(50);

    // 解析 JSON
    QJsonDocument doc = QJsonDocument::fromJson(m_deviceJson.toUtf8());
    QJsonObject dev = doc.object();
    QJsonObject ident = dev["identity"].toObject();

    // 更新原始身份显示
    m_origVendor->setText(QString("0x%1").arg((quint64)ident["vendor_id"].toDouble(), 8, 16, QChar('0')));
    m_origProduct->setText(QString("0x%1").arg((quint64)ident["product_code"].toDouble(), 8, 16, QChar('0')));
    m_origRevision->setText(QString("0x%1").arg((quint64)ident["revision_number"].toDouble(), 8, 16, QChar('0')));
    m_origName->setText(ident["device_name"].toString());

    // 更新对象表格
    updateObjectTable(dev["objects"].toObject());

    // 更新 PDO 面板
    updatePdoPanel(dev);

    m_progress->setValue(60);
    int objCount = dev["objects"].toObject().keys().size();
    m_log->append(QString("[完成] 解析完成: %1 个对象").arg(objCount));
    setStatus(QString("解析完成 — %1 个对象").arg(objCount));
}

// ═════════════════════════════════════════════
// 对象表格更新
// ═════════════════════════════════════════════

void MainWindow::updateObjectTable(const QJsonObject &objects) {
    m_objTable->setRowCount(0);

    QStringList indices = objects.keys();
    std::sort(indices.begin(), indices.end(), [](const QString &a, const QString &b) {
        return a.toInt() < b.toInt();
    });

    m_objTable->setRowCount(indices.size());
    for (int row = 0; row < indices.size(); ++row) {
        QJsonObject obj = objects[indices[row]].toObject();
        int idx = indices[row].toInt();

        // 颜色编码
        QColor bgColor(15, 18, 37); // 默认深色
        bool hasRisk = !obj["risk_flags"].toArray().isEmpty();
        bool hasCoe = obj["coe_read"].toBool() || obj["coe_write"].toBool();
        bool isPdo = obj["is_pdo_mapping_object"].toBool();

        if (hasRisk) bgColor = QColor(60, 20, 20);       // 红色
        else if (hasCoe) bgColor = QColor(40, 35, 15);   // 黄色
        else if (isPdo) bgColor = QColor(15, 30, 15);    // 绿色

        auto addItem = [&](int col, const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setBackground(QBrush(bgColor));
            item->setForeground(QBrush(QColor(200, 200, 200)));
            m_objTable->setItem(row, col, item);
        };

        addItem(0, QString("0x%1").arg(idx, 4, 16, QChar('0')).toUpper());
        addItem(1, obj["name"].toString());
        addItem(2, QString::number(obj["subindices"].toArray().size()));
        addItem(3, obj["category"].toString());
        addItem(4, isPdo ? "✓" : "");
        addItem(5, obj["is_sm_object"].toBool() ? "✓" : "");
        addItem(6, hasRisk ? obj["risk_flags"].toArray()[0].toString() : "");
        addItem(7, obj["coe_read"].toBool() ? "TRUE" : "");
        addItem(8, obj["coe_write"].toBool() ? "TRUE" : "");
    }
    m_objCountLabel->setText(QString("%1 对象").arg(indices.size()));
}

// ═════════════════════════════════════════════
// PDO 面板更新
// ═════════════════════════════════════════════

void MainWindow::updatePdoPanel(const QJsonObject &device) {
    QStringList lines;
    QJsonArray sms = device["sm_configs"].toArray();
    for (const auto &smV : sms) {
        QJsonObject sm = smV.toObject();
        int defSize = sm["default_size"].toInt();
        int calcSize = 0;
        for (const auto &pdoV : sm["pdos"].toArray())
            for (const auto &eV : pdoV.toObject()["entries"].toArray())
                calcSize += eV.toObject()["bit_length"].toInt();
        calcSize = (calcSize + 7) / 8;
        QString status = (defSize == calcSize || calcSize == 0) ? "✅ OK" : "❌ MISMATCH";
        lines << QString("SM%1 [%2]:  默认=%3B  计算=%4B  %5")
                     .arg(sm["index"].toInt())
                     .arg(sm["pdo_type"].toString())
                     .arg(defSize).arg(calcSize).arg(status);
    }
    m_pdoText->setPlainText(lines.isEmpty() ? "(无 SM 配置)" : lines.join("\n"));
}

// ═════════════════════════════════════════════
// 生成
// ═════════════════════════════════════════════

void MainWindow::onGenerate(int mode) {
    if (m_deviceJson.isEmpty()) {
        setStatus("错误: 请先导入文件");
        return;
    }
    setStatus("转换中...");
    m_progress->setValue(10);
    m_log->append("[生成] 启动转换...");

    // 收集配置
    QVariantMap identity;
    identity["vendor_id"]    = m_newVendor->text().trimmed();
    identity["product_code"] = m_newProduct->text().trimmed();
    identity["revision"]     = m_newRevision->text().trimmed();
    identity["device_name"]  = m_newName->text().trimmed();

    m_worker = new ConvertWorker(
        m_esiEdit->text().trimmed(), m_sdoEdit->text().trimmed(),
        m_tplEdit->text().trimmed(), m_outEdit->text().trimmed(),
        identity, QVariantMap(), this);

    connect(m_worker, &ConvertWorker::log, this, [this](const QString &msg) {
        m_log->append(msg);
    });
    connect(m_worker, &ConvertWorker::progress, m_progress, &QProgressBar::setValue);
    connect(m_worker, &ConvertWorker::finishedOk, this, [this](const QString &dir) {
        m_log->append("[完成] 输出: " + dir);
        setStatus("转换完成");
        m_progress->setValue(100);
    });
    connect(m_worker, &ConvertWorker::failed, this, [this](const QString &err) {
        m_log->append("[失败] " + err);
        setStatus("转换失败");
        m_progress->setValue(0);
    });
    connect(m_worker, &ConvertWorker::finished, m_worker, &QObject::deleteLater);
    m_worker->start();
}

// ═════════════════════════════════════════════
// 状态
// ═════════════════════════════════════════════

void MainWindow::setStatus(const QString &msg) {
    statusBar()->showMessage(msg);
}
