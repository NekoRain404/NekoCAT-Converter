/**
 * MainWindow — 专业级实现 (重构版)。
 *
 * 参考 docs/ui/ 参考图:
 *   - 深海军蓝主题 + 红色强调 + 青色辅助
 *   - 侧边栏步骤导航带编号圆圈
 *   - 对象表格 9 列, 行着色 (正常/风险/CoE/PDO/跳过)
 *   - 渐变进度条 + 实时日志
 *   - 生成按钮组 (全部生成 + 单项生成)
 *
 * 解耦:
 *   - 样式由 core/theme 集中管理
 *   - Worker 线程通过信号槽通信, 不直接操作 UI
 */
#include "mainwindow.h"
#include <QScrollBar>
#include "worker.h"
#include "core/theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QGroupBox>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QDir>
#include <QMargins>
#include <QFileDialog>
#include <QHeaderView>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QApplication>
#include <algorithm>

// ═══ 构造函数 ═════════════════════════════════════════════
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_worker(nullptr)
{
    setWindowTitle("NekoECAT Converter");
    setMinimumSize(1200, 780);
    resize(1440, 900);
    setFont(Theme::fontNormal());

    // 应用全局样式 + 深色调色板
    setStyleSheet(Theme::globalStyleSheet());
    qApp->setPalette(Theme::lightPalette());

    // ── 根布局: 侧边栏 + 右侧面板 ───────────
    auto *root = new QWidget;
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(QMargins(0, 0, 0, 0));
    rootLayout->setSpacing(0);

    rootLayout->addWidget(buildSidebar(), 0);
    rootLayout->addWidget(buildRightPanel(), 1);

    setCentralWidget(root);

    // ── 状态栏 ──────────────────────────────
    statusBar()->showMessage("就绪");

    // ── 连接信号槽 ──────────────────────────
    connectSignals();
    onStepChanged(0); // 默认显示步骤1
}

// ═══ 侧边栏 ═══════════════════════════════════════════════
/**
 * 侧边栏: Logo + 3个步骤按钮 (带编号圆圈) + 导入按钮。
 *
 * 布局:
 *   ┌────────────────────┐
 *   │ NekoECAT Converter │  Logo
 *   │ ECAT -> SSC Tool   │  Subtitle
 *   │                    │
 *   │ [①] 文件导入       │  Step 1
 *   │ [②] 身份配置       │  Step 2
 *   │ [③] 规则策略       │  Step 3
 *   │                    │
 *   │   [ 导 入 ]        │  Import (红色渐变)
 *   └────────────────────┘
 */
QWidget *MainWindow::buildSidebar() {
    auto *sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(300);
    sidebar->setStyleSheet(Theme::sidebarStyleSheet());

    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(QMargins(18, 24, 18, 20));
    layout->setSpacing(4);

    // ── Logo 区 ─────────────────────────────
    auto *logo = new QLabel("NekoECAT Converter");
    logo->setObjectName("sidebarTitle");

    auto *subtitle = new QLabel("ECAT → SSC Tool");
    subtitle->setObjectName("sidebarSubtitle");

    layout->addWidget(logo);
    layout->addWidget(subtitle);
    layout->addSpacing(8);

    // ── 分隔线 ──────────────────────────────
    auto *sep1 = new QFrame;
    sep1->setObjectName("separator");
    sep1->setFrameShape(QFrame::HLine);
    layout->addWidget(sep1);
    layout->addSpacing(8);

    // ── 步骤按钮 (3个) ─────────────────────
    const char *stepLabels[] = {
        "文件导入",
        "身份配置",
        "规则策略"
    };
    const char *stepIcons[] = { "📁", "🏷", "⚙" };

    for (int i = 0; i < 3; ++i) {
        // 步骤按钮容器: [编号圆圈] [文字]
        auto *btnContainer = new QWidget;
        auto *btnLayout = new QHBoxLayout(btnContainer);
        btnLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        btnLayout->setSpacing(10);

        // 编号圆圈
        auto *numLabel = new QLabel(QString::number(i + 1));
        numLabel->setObjectName("stepNum");
        numLabel->setAlignment(Qt::AlignCenter);
        numLabel->setFixedSize(26, 26);

        // 按钮文字
        auto *btn = new QPushButton(
            QString("%1  %2").arg(stepIcons[i]).arg(stepLabels[i]));
        btn->setObjectName("stepBtn");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(44);

        btnLayout->addWidget(numLabel);
        btnLayout->addWidget(btn, 1);

        m_stepBtns[i] = btn;

        // 按钮点击 → 切换步骤
        int idx = i;
        connect(btn, &QPushButton::clicked, this, [this, idx]() {
            onStepChanged(idx);
        });

        layout->addWidget(btnContainer);
    }

    layout->addStretch();

    // ── 分隔线 ──────────────────────────────
    auto *sep2 = new QFrame;
    sep2->setObjectName("separator");
    sep2->setFrameShape(QFrame::HLine);
    layout->addWidget(sep2);
    layout->addSpacing(8);

    // ── 导入按钮 (红色渐变) ─────────────────
    m_importBtn = new QPushButton("📥  导  入");
    m_importBtn->setObjectName("importBtn");
    m_importBtn->setCursor(Qt::PointingHandCursor);
    m_importBtn->setMinimumHeight(48);
    connect(m_importBtn, &QPushButton::clicked, this, &MainWindow::onImport);

    layout->addWidget(m_importBtn);

    return sidebar;
}

// ═══ 步骤1: 文件路径选择 ═════════════════════════════════
/**
 * 步骤1面板: ESI/SDO/模板/输出路径选择。
 * 每行: [标签] [输入框] [浏览按钮]
 */
QWidget *MainWindow::buildStep1() {
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(QMargins(12, 8, 12, 8));
    layout->setSpacing(8);

    auto addPathRow = [&](const QString &label, QLineEdit *&edit,
                         const QString &filter, bool isDir = false) {
        auto *row = new QHBoxLayout;
        row->setSpacing(6);

        auto *lbl = new QLabel(label);
        lbl->setFixedWidth(65);
        lbl->setStyleSheet("color: #6b7280; font-size: 11px;");

        edit = new QLineEdit;
        edit->setPlaceholderText("点击浏览选择...");
        edit->setReadOnly(true);

        auto *btn = new QPushButton("...");
        btn->setFixedSize(32, 32);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [=]() {
            if (isDir) {
                QString dir = QFileDialog::getExistingDirectory(
                    this, "选择目录", edit->text());
                if (!dir.isEmpty()) edit->setText(dir);
            } else {
                QString file = QFileDialog::getOpenFileName(
                    this, "选择文件", QString(), filter);
                if (!file.isEmpty()) edit->setText(file);
            }
        });

        row->addWidget(lbl);
        row->addWidget(edit, 1);
        row->addWidget(btn);

        layout->addLayout(row);
    };

    addPathRow("ESI:", m_esiEdit, "XML files (*.xml);;All (*)");
    addPathRow("SDO:", m_sdoEdit, "XML/TXT files (*.xml *.txt);;All (*)");
    addPathRow("模板:", m_tplEdit, "Excel files (*.xlsx);;All (*)");
    addPathRow("输出:", m_outEdit, "", true);
    m_outEdit->setText("out");

    return widget;
}

// ═══ 步骤2: 设备身份配置 ═════════════════════════════════
/**
 * 步骤2面板: 显示当前身份 (只读) + 覆盖输入框。
 * 上半: 解析得到的只读身份信息
 * 下半: 覆盖字段 (Vendor ID / Product Code / Revision / Name)
 */
QWidget *MainWindow::buildStep2() {
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(QMargins(12, 8, 12, 8));
    layout->setSpacing(10);

    // ── 当前身份 (只读) ─────────────────────
    auto *currentGroup = new QGroupBox("当前身份 (解析结果)");
    auto *curGrid = new QGridLayout(currentGroup);
    curGrid->setSpacing(6);

    auto addRO = [&](int row, const QString &label, QLabel *&lbl) {
        auto *l = new QLabel(label);
        l->setStyleSheet("color: #6b7280; font-size: 11px;");
        lbl = new QLabel("--");
        lbl->setStyleSheet("color: #0d9488; font-size: 12px; font-weight: bold;");
        curGrid->addWidget(l, row, 0);
        curGrid->addWidget(lbl, row, 1);
    };

    addRO(0, "Vendor ID:", m_ov);
    addRO(1, "Product Code:", m_op);
    addRO(2, "Revision:", m_or);
    addRO(3, "Device Name:", m_on);

    layout->addWidget(currentGroup);

    // ── 覆盖字段 ────────────────────────────
    auto *overrideGroup = new QGroupBox("覆盖配置 (可选)");
    auto *ovGrid = new QGridLayout(overrideGroup);
    ovGrid->setSpacing(6);

    auto addOverride = [&](int row, const QString &label, QLineEdit *&edit,
                          const QString &placeholder) {
        auto *l = new QLabel(label);
        l->setStyleSheet("color: #6b7280; font-size: 11px;");
        edit = new QLineEdit;
        edit->setPlaceholderText(placeholder);
        ovGrid->addWidget(l, row, 0);
        ovGrid->addWidget(edit, row, 1);
    };

    addOverride(0, "Vendor ID:", m_nv, "0x00001234");
    addOverride(1, "Product Code:", m_np, "0x00005678");
    addOverride(2, "Revision:", m_nr, "0x00000001");
    addOverride(3, "Device Name:", m_nn, "MyEtherCAT Device");
    addOverride(4, "物理地址:", m_phys, "0");

    layout->addWidget(overrideGroup);

    // ── 映射模式 ────────────────────────────
    auto *modeGroup = new QGroupBox("映射模式");
    auto *modeLayout = new QHBoxLayout(modeGroup);
    m_modeGrp = new QButtonGroup(this);

    const char *modes[] = { "标准", "简单", "完整" };
    for (int i = 0; i < 3; ++i) {
        auto *rb = new QRadioButton(modes[i]);
        m_modeGrp->addButton(rb, i);
        modeLayout->addWidget(rb);
    }
    // 默认选中"标准"
    m_modeGrp->button(0)->setChecked(true);

    layout->addWidget(modeGroup);
    layout->addStretch();

    return widget;
}

// ═══ 步骤3: 转换策略 ═════════════════════════════════════
/**
 * 步骤3面板: 转换策略选项 (单选按钮组)。
 *   - OCTET_STRING 处理
 *   - 字对齐
 *   - 通信参数
 *   - Fxxx 对象
 */
QWidget *MainWindow::buildStep3() {
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(QMargins(12, 8, 12, 8));
    layout->setSpacing(10);

    auto addRadioGroup = [&](const QString &title, QButtonGroup *&grp,
                            const char *options[], int count) {
        auto *group = new QGroupBox(title);
        auto *gl = new QVBoxLayout(group);
        gl->setSpacing(6);
        grp = new QButtonGroup(this);
        for (int i = 0; i < count; ++i) {
            auto *rb = new QRadioButton(options[i]);
            grp->addButton(rb, i);
            gl->addWidget(rb);
        }
        grp->button(0)->setChecked(true);
        layout->addWidget(group);
    };

    // OCTET_STRING 策略
    const char *octOpts[] = {
        "保留原样 (默认)",
        "转为 TEXTLIST",
        "转为 UINT8 数组"
    };
    addRadioGroup("OCTET_STRING 处理", m_octGrp, octOpts, 3);

    // 字对齐
    const char *alnOpts[] = {
        "自动对齐 (推荐)",
        "强制 16-bit 对齐",
        "不强制对齐"
    };
    addRadioGroup("字对齐策略", m_alnGrp, alnOpts, 3);

    // 通信参数
    const char *comOpts[] = {
        "包含通信对象",
        "排除通信对象",
        "仅 SDO 通信"
    };
    addRadioGroup("通信参数", m_comGrp, comOpts, 3);

    // Fxxx 对象
    const char *fxxOpts[] = {
        "包含 Fxxx 对象",
        "排除 Fxxx 对象",
        "仅自动映射 Fxxx"
    };
    addRadioGroup("Fxxx 对象", m_fxxGrp, fxxOpts, 3);

    layout->addStretch();
    return widget;
}

// ═══ 右侧主内容区 ═════════════════════════════════════════
/**
 * 右侧内容区: 过滤栏 + 表格 + PDO 校验 + 底部工具栏。
 */
QWidget *MainWindow::buildRightPanel() {
    auto *panel = new QWidget;
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(QMargins(16, 16, 16, 12));
    layout->setSpacing(10);

    // ── 过滤栏 ──────────────────────────────
    layout->addWidget(buildFilterBar());

    // ── 对象字典表格 ────────────────────────
    m_table = new QTableWidget;
    m_table->setColumnCount(9);

    QStringList headers = {
        "Index", "名称", "Sub", "类别",
        "PDO", "SM", "风险标志", "CoE Read", "CoE Write"
    };
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setStyleSheet(Theme::tableStyleSheet());

    // 表格配置
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);

    // 列宽设置
    QHeaderView *header = m_table->horizontalHeader();
    header->setStretchLastSection(true);
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Index
    header->setSectionResizeMode(1, QHeaderView::Stretch);            // 名称
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // Sub
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);  // 类别
    for (int i = 4; i < 9; ++i)
        header->setSectionResizeMode(i, QHeaderView::ResizeToContents);

    m_table->setMinimumHeight(280);
    layout->addWidget(m_table, 1); // stretch factor 1, 占主要空间

    // ── PDO/SM 校验区 ──────────────────────
    auto *pdoGroup = new QGroupBox("PDO / SM 校验");
    auto *pdoLayout = new QVBoxLayout(pdoGroup);
    m_pdoTxt = new QTextEdit;
    m_pdoTxt->setObjectName("pdo");
    m_pdoTxt->setReadOnly(true);
    m_pdoTxt->setMaximumHeight(120);
    m_pdoTxt->setPlaceholderText("导入 ESI 后将显示 SM 校验信息...");
    pdoLayout->addWidget(m_pdoTxt);
    layout->addWidget(pdoGroup);

    // ── 底部工具栏 ──────────────────────────
    layout->addWidget(buildBottomBar());

    return panel;
}

// ═══ 过滤栏 ═══════════════════════════════════════════════
/**
 * 过滤栏: 分类下拉框 + 搜索输入框 + 对象计数。
 */
QWidget *MainWindow::buildFilterBar() {
    auto *bar = new QWidget;
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(QMargins(4, 0, 4, 0));
    layout->setSpacing(10);

    // 分类过滤
    auto *filterLabel = new QLabel("过滤:");
    filterLabel->setStyleSheet("color: #8899aa; font-size: 12px;");
    m_filter = new QComboBox;
    m_filter->addItems({
        "全部对象", "仅风险", "仅 CoE", "仅 PDO", "仅 Fxxx", "已跳过"
    });
    m_filter->setMinimumWidth(140);

    // 搜索框
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("🔍 搜索 Index / 名称...");
    m_searchEdit->setMaximumWidth(260);

    // 对象计数
    m_objCnt = new QLabel("0 对象");
    m_objCnt->setStyleSheet(
        "color: #0d9488; font-size: 12px; font-weight: bold;");

    layout->addWidget(filterLabel);
    layout->addWidget(m_filter);
    layout->addWidget(m_searchEdit);
    layout->addStretch();
    layout->addWidget(m_objCnt);

    return bar;
}

// ═══ 底部工具栏 ═══════════════════════════════════════════
/**
 * 底部工具栏: 生成按钮组 + 进度条 + 实时日志。
 *
 *   ┌──────────────────────────────────────────────┐
 *   │ [全部生成] [SSC xlsx] [TwinCAT] [报告] [C代码] │  按钮行
 *   │ ████████████████░░░░░░░░░░░░  60%             │  进度条
 *   │ > [日志内容...]                               │  日志
 *   └──────────────────────────────────────────────┘
 */
QWidget *MainWindow::buildBottomBar() {
    auto *bar = new QWidget;
    bar->setObjectName("bottomBar");
    bar->setStyleSheet(Theme::bottomBarStyleSheet());

    auto *layout = new QVBoxLayout(bar);
    layout->setContentsMargins(QMargins(8, 8, 8, 8));
    layout->setSpacing(8);

    // ── 生成按钮行 ──────────────────────────
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);

    auto addGenBtn = [&](const QString &label, const QString &objName,
                        int mode) -> QPushButton* {
        auto *btn = new QPushButton(label);
        btn->setObjectName(objName);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this,
                [this, mode]() { onGenerate(mode); });
        btnRow->addWidget(btn);
        return btn;
    };

    addGenBtn("全部生成",   "genBtnAll", 0);
    addGenBtn("SSC xlsx",  "genBtn",    1);
    addGenBtn("TwinCAT",   "genBtn",    2);
    addGenBtn("报告",       "genBtn",    3);
    addGenBtn("C 代码",     "genBtn",    4);

    btnRow->addStretch();
    layout->addLayout(btnRow);

    // ── 进度条 ──────────────────────────────
    m_prog = new QProgressBar;
    m_prog->setRange(0, 100);
    m_prog->setValue(0);
    m_prog->setTextVisible(true);
    layout->addWidget(m_prog);

    // ── 实时日志 ────────────────────────────
    m_log = new QTextEdit;
    m_log->setObjectName("log");
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(160);
    m_log->setPlaceholderText("日志输出...");
    layout->addWidget(m_log);

    return bar;
}

// ═══ 信号槽连接 ═══════════════════════════════════════════
/**
 * 连接所有信号槽:
 *   - 过滤下拉框 → 重新渲染表格
 *   - 搜索框 → 实时过滤
 */
void MainWindow::connectSignals() {
    // 过滤下拉框变化 → 重新渲染
    connect(m_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        if (m_devJson.isEmpty()) return;
        QJsonObject dev = QJsonDocument::fromJson(m_devJson.toUtf8()).object();
        updateObjectTable(dev["objects"].toObject());
    });

    // 搜索框变化 → 重新渲染
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &) {
        if (m_devJson.isEmpty()) return;
        QJsonObject dev = QJsonDocument::fromJson(m_devJson.toUtf8()).object();
        updateObjectTable(dev["objects"].toObject());
    });
}

// ═══ 步骤切换 ═════════════════════════════════════════════
/**
 * 切换侧边栏步骤:
 *   0 → 文件导入面板
 *   1 → 身份配置面板
 *   2 → 规则策略面板
 */
void MainWindow::onStepChanged(int step) {
    // 更新步骤按钮样式 (active 属性控制 QSS)
    for (int i = 0; i < 3; ++i) {
        m_stepBtns[i]->setProperty("active", (i == step));
        m_stepBtns[i]->style()->unpolish(m_stepBtns[i]);
        m_stepBtns[i]->style()->polish(m_stepBtns[i]);
    }

    // 同步更新步骤编号圆圈的 active 样式
    auto *container = m_stepBtns[step]->parentWidget();
    if (container) {
        auto *numLabel = container->findChild<QLabel*>("stepNum");
        // 遍历所有步骤容器更新
        for (int i = 0; i < 3; ++i) {
            auto *c = m_stepBtns[i]->parentWidget();
            if (c) {
                auto *nl = c->findChild<QLabel*>("stepNum");
                if (nl) {
                    nl->setProperty("active", (i == step));
                    nl->style()->unpolish(nl);
                    nl->style()->polish(nl);
                }
            }
        }
    }
}

// ═══ 导入 ═════════════════════════════════════════════════
/**
 * 导入按钮: 调用 Python parse_only() 解析 ESI/SDO 文件。
 *
 * 流程:
 *   1. 构造 Python 内联脚本
 *   2. 通过 QProcess 执行, 等待完成
 *   3. 解析 JSON 结果, 填充身份/表格/PDO 面板
 */
void MainWindow::onImport() {
    QString esi = m_esiEdit->text().trimmed();
    QString sdo = m_sdoEdit->text().trimmed();

    if (esi.isEmpty() && sdo.isEmpty()) {
        setStatus("请至少选择一个 ESI 或 SDO 文件");
        return;
    }

    setStatus("解析中...");
    m_prog->setValue(10);
    m_importBtn->setEnabled(false);
    log("[导入] 启动解析...");

    // 定位项目根目录
    QString appDir = QApplication::applicationDirPath();
    QString root = appDir;
    if (root.endsWith("/gui-cpp/build"))
        root = QFileInfo(root).path() + "/..";

    // Python 内联脚本: 调用 nekoecat.core.parse_only
    QString pyScript =
        "import sys, json\n"
        "sys.path.insert(0, '.')\n"
        "from nekoecat.core import parse_only\n"
        "esi = sys.argv[1] if sys.argv[1] != 'None' else None\n"
        "sdo = sys.argv[2] if sys.argv[2] != 'None' else None\n"
        "d = parse_only(esi_path=esi, sdo_path=sdo)\n"
        "print(json.dumps(d.model_dump(mode='json'), ensure_ascii=False))";

    QProcess proc;
    proc.setWorkingDirectory(root);
    proc.start("python3", {
        "-c", pyScript,
        esi.isEmpty() ? "None" : esi,
        sdo.isEmpty() ? "None" : sdo
    });

    if (!proc.waitForStarted(5000)) {
        log("[错误] 无法启动 Python 进程");
        setStatus("失败");
        m_prog->setValue(0);
        m_importBtn->setEnabled(true);
        return;
    }

    proc.waitForFinished(30000);

    if (proc.exitCode() != 0) {
        QString err = QString::fromUtf8(proc.readAllStandardError());
        log("[错误] " + err);
        setStatus("解析失败");
        m_prog->setValue(0);
        m_importBtn->setEnabled(true);
        return;
    }

    // ── 解析成功 ────────────────────────────
    m_devJson = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    m_prog->setValue(50);

    QJsonObject dev = QJsonDocument::fromJson(m_devJson.toUtf8()).object();
    QJsonObject id = dev["identity"].toObject();

    // 填充身份信息 (只读标签)
    auto hex = [](double val) -> QString {
        return QString("0x%1").arg((quint64)val, 8, 16, QChar('0'));
    };
    m_ov->setText(hex(id["vendor_id"].toDouble()));
    m_op->setText(hex(id["product_code"].toDouble()));
    m_or->setText(hex(id["revision_number"].toDouble()));
    m_on->setText(id["device_name"].toString());

    // 填充表格
    updateObjectTable(dev["objects"].toObject());

    // 填充 PDO 校验
    updatePdoPanel(dev);

    int objCount = dev["objects"].toObject().keys().size();
    m_prog->setValue(100);
    log(QString("[完成] 解析成功 — %1 个对象").arg(objCount));
    setStatus(QString("解析完成 — %1 个对象").arg(objCount));
    m_importBtn->setEnabled(true);
}

// ═══ 对象表格更新 ═════════════════════════════════════════
/**
 * 根据 JSON 数据更新对象字典表格。
 *
 * 行着色规则:
 *   - 风险对象 → 深红 (#3c1414)
 *   - CoE 对象 → 深黄 (#28230f)
 *   - PDO 对象 → 深绿 (#0f1e0f)
 *   - 正常交替 → 深蓝交替
 *
 * 支持过滤: 通过 m_filter 下拉框和 m_searchEdit 搜索框。
 */
void MainWindow::updateObjectTable(const QJsonObject &objs) {
    m_table->setRowCount(0);

    // ── 收集并排序对象 ──────────────────────
    QStringList keys = objs.keys();
    std::sort(keys.begin(), keys.end(), [](const QString &a, const QString &b) {
        return a.toInt() < b.toInt();
    });

    // ── 应用过滤 ────────────────────────────
    int filterIdx = m_filter->currentIndex();
    QString searchText = m_searchEdit->text().trimmed().toLower();

    QStringList filteredKeys;
    for (const QString &key : keys) {
        QJsonObject obj = objs[key].toObject();
        QString name = obj["name"].toString().toLower();
        QString indexStr = QString("0x%1").arg(key.toInt(), 4, 16, QChar('0'));

        // 搜索过滤
        if (!searchText.isEmpty() &&
            !name.contains(searchText) && !indexStr.contains(searchText))
            continue;

        // 分类过滤
        bool risk = !obj["risk_flags"].toArray().isEmpty();
        bool coe = obj["coe_read"].toBool() || obj["coe_write"].toBool();
        bool pdo = obj["is_pdo_mapping_object"].toBool();
        bool isFxxx = key.toInt() >= 0xF000;

        switch (filterIdx) {
        case 1: if (!risk) continue; break;       // 仅风险
        case 2: if (!coe) continue; break;        // 仅 CoE
        case 3: if (!pdo) continue; break;        // 仅 PDO
        case 4: if (!isFxxx) continue; break;     // 仅 Fxxx
        // case 5: 已跳过 (暂不支持, 简化处理)
        }

        filteredKeys.append(key);
    }

    // ── 填充表格 ────────────────────────────
    m_table->setRowCount(filteredKeys.size());

    for (int r = 0; r < filteredKeys.size(); ++r) {
        QJsonObject obj = objs[filteredKeys[r]].toObject();
        int idx = filteredKeys[r].toInt();
        bool risk = !obj["risk_flags"].toArray().isEmpty();
        bool coe = obj["coe_read"].toBool() || obj["coe_write"].toBool();
        bool pdo = obj["is_pdo_mapping_object"].toBool();

        // ── 行着色 ──────────────────────────
        QColor bgColor(255, 255, 255); // 默认深蓝
        if (risk)
            bgColor = QColor(254, 242, 242);        // 深红
        else if (coe)
            bgColor = QColor(255, 251, 235);        // 深黄
        else if (pdo)
            bgColor = QColor(240, 253, 244);        // 深绿
        else if (r % 2 == 1)
            bgColor = QColor(248, 249, 251);        // 交替行

        // ── 填充各列 ────────────────────────
        auto setCell = [&](int col, const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setBackground(QBrush(bgColor));
            item->setForeground(QBrush(QColor(107, 114, 128)));
            item->setFont(Theme::fontMono(11));
            m_table->setItem(r, col, item);
        };

        // Index: 红色高亮
        auto *idxItem = new QTableWidgetItem(
            QString("0x%1").arg(idx, 4, 16, QChar('0')).toUpper());
        idxItem->setBackground(QBrush(bgColor));
        idxItem->setForeground(QBrush(QColor(37, 99, 235))); // ACCENT_RED
        idxItem->setFont(Theme::fontMono(11));
        m_table->setItem(r, 0, idxItem);

        setCell(1, obj["name"].toString());
        setCell(2, QString::number(obj["subindices"].toArray().size()));
        setCell(3, obj["category"].toString());
        setCell(4, pdo ? "✓" : "");
        setCell(5, obj["is_sm_object"].toBool() ? "✓" : "");

        // 风险标志: 红色文字
        auto *riskItem = new QTableWidgetItem(
            risk ? obj["risk_flags"].toArray()[0].toString() : "");
        riskItem->setBackground(QBrush(bgColor));
        riskItem->setForeground(QBrush(
            risk ? QColor(220, 38, 38) : QColor(107, 114, 128)));
        riskItem->setFont(Theme::fontMono(11));
        m_table->setItem(r, 6, riskItem);

        setCell(7, obj["coe_read"].toBool() ? "TRUE" : "");
        setCell(8, obj["coe_write"].toBool() ? "TRUE" : "");
    }

    m_objCnt->setText(QString("%1 对象").arg(filteredKeys.size()));
}

// ═══ PDO/SM 校验面板 ═════════════════════════════════════
/**
 * 更新 PDO/SM 校验面板:
 *   对比每个 SM 的默认大小与计算大小, 标记是否匹配。
 */
void MainWindow::updatePdoPanel(const QJsonObject &dev) {
    QStringList lines;

    for (const auto &sv : dev["sm_configs"].toArray()) {
        QJsonObject sm = sv.toObject();
        int defaultSize = sm["default_size"].toInt();
        int calcSize = 0;

        for (const auto &pv : sm["pdos"].toArray()) {
            for (const auto &ev : pv.toObject()["entries"].toArray()) {
                calcSize += ev.toObject()["bit_length"].toInt();
            }
        }
        calcSize = (calcSize + 7) / 8; // 比特转字节, 向上取整

        bool match = (defaultSize == calcSize || calcSize == 0);

        lines << QString("SM%1 [%2]: 默认=%3B  计算=%4B  %5")
                    .arg(sm["index"].toInt())
                    .arg(sm["pdo_type"].toString())
                    .arg(defaultSize)
                    .arg(calcSize)
                    .arg(match ? "✅ OK" : "❌ MISMATCH");
    }

    m_pdoTxt->setPlainText(lines.isEmpty() ? "(无 SM 配置)" : lines.join("\n"));
}

// ═══ 生成 ═════════════════════════════════════════════════
/**
 * 启动转换生成:
 *   1. 收集身份覆盖信息
 *   2. 创建 ConvertWorker 线程
 *   3. 连接信号槽 (日志/进度/完成/失败)
 *   4. 启动线程
 */
void MainWindow::onGenerate(int mode) {
    if (m_devJson.isEmpty()) {
        setStatus("请先导入 ESI/SDO 文件");
        return;
    }

    setStatus("转换中...");
    m_prog->setValue(10);
    log(QString("[生成] 启动... (模式=%1)").arg(mode));

    // ── 收集身份覆盖 ────────────────────────
    QVariantMap identity;
    if (!m_nv->text().trimmed().isEmpty())
        identity["vendor_id"] = m_nv->text().trimmed();
    if (!m_np->text().trimmed().isEmpty())
        identity["product_code"] = m_np->text().trimmed();
    if (!m_nr->text().trimmed().isEmpty())
        identity["revision"] = m_nr->text().trimmed();
    if (!m_nn->text().trimmed().isEmpty())
        identity["device_name"] = m_nn->text().trimmed();

    // ── 创建 Worker ─────────────────────────
    m_worker = new ConvertWorker(
        m_esiEdit->text().trimmed(),
        m_sdoEdit->text().trimmed(),
        m_tplEdit->text().trimmed(),
        m_outEdit->text().trimmed(),
        identity,
        QVariantMap(), // rules (暂未使用)
        this
    );

    connect(m_worker, &ConvertWorker::log,
            this, [this](const QString &m) { log(m); });
    connect(m_worker, &ConvertWorker::progress,
            m_prog, &QProgressBar::setValue);
    connect(m_worker, &ConvertWorker::finishedOk,
            this, [this](const QString &d) {
        log("[完成] " + d);
        setStatus("转换完成");
        m_prog->setValue(100);
    });
    connect(m_worker, &ConvertWorker::failed,
            this, [this](const QString &e) {
        log("[失败] " + e);
        setStatus("转换失败");
        m_prog->setValue(0);
    });
    connect(m_worker, &ConvertWorker::finished,
            m_worker, &QObject::deleteLater);

    m_worker->start();
}

// ═══ 辅助方法 ═════════════════════════════════════════════
void MainWindow::setStatus(const QString &msg) {
    statusBar()->showMessage(msg);
}

void MainWindow::log(const QString &msg) {
    m_log->append(msg);
    // 自动滚动到底部
    auto *scrollBar = m_log->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
