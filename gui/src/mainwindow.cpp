/**
 * MainWindow — Light Mode 专业级实现。
 *
 * 参考 docs/ui/ 参考图:
 *   - 明亮主题 + 蓝色强调
 *   - 侧边栏: 步骤导航 + 配置面板 (QStackedWidget)
 *   - 右侧: 对象字典表格 + PDO 校验 + 生成按钮 + 日志
 *
 * 解耦:
 *   - 样式由 core/theme 集中管理
 *   - Worker 线程通过信号槽通信, 不直接操作 UI
 *   - 步骤面板通过 QStackedWidget 切换
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
// ═══ ImportWorker 实现 ═════════════════════════════════════
ImportWorker::ImportWorker(const QString &esi, const QString &sdo,
                           const QString &root, QObject *parent)
    : QThread(parent), m_esi(esi), m_sdo(sdo), m_root(root) {}

void ImportWorker::run() {
    emit log("[导入] 启动解析...");
    emit progress(10);

    QString pyScript =
        "import sys, json, os\n"
        "os.environ.setdefault(\"PYTHONDONTWRITEBYTECODE\", \"1\")\n"
        "sys.path.insert(0, \".\")\n"
        "import logging\n"
        "logging.disable(logging.CRITICAL)\n"
        "from nekoecat.core import parse_only\n"
        "esi = sys.argv[1] if sys.argv[1] != \"None\" else None\n"
        "sdo = sys.argv[2] if sys.argv[2] != \"None\" else None\n"
        "d = parse_only(esi_path=esi, sdo_path=sdo)\n"
        "result = json.dumps(d.model_dump(mode=\"json\"), ensure_ascii=False)\n"
        "sys.stdout.write(result)\n"
        "sys.stdout.flush()";

    QProcess proc;
    proc.setWorkingDirectory(m_root);
    proc.start("python3", {
        "-c", pyScript,
        m_esi.isEmpty() ? "None" : m_esi,
        m_sdo.isEmpty() ? "None" : m_sdo
    });

    if (!proc.waitForStarted(5000)) {
        emit failed("无法启动 Python 进程，请确保已安装 Python3");
        return;
    }

    emit progress(30);

    if (!proc.waitForFinished(30000)) {
        proc.kill();
        emit failed("Python 进程超时");
        return;
    }

    emit progress(70);

    if (proc.exitCode() != 0) {
        QString err = QString::fromUtf8(proc.readAllStandardError());
        emit failed("解析失败: " + err);
        return;
    }

    QString rawOutput = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    int jsonStart = rawOutput.indexOf("{");
    int jsonEnd = rawOutput.lastIndexOf("}");
    if (jsonStart < 0 || jsonEnd <= jsonStart) {
        emit failed("返回数据无效");
        return;
    }

    QByteArray jsonData = rawOutput.mid(jsonStart, jsonEnd - jsonStart + 1).toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull()) {
        emit failed("JSON 解析失败");
        return;
    }

    emit progress(100);
    emit finished(doc.object());
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_worker(nullptr)
{
    setWindowTitle("NekoECAT Converter");
    setMinimumSize(1200, 780);
    resize(1440, 900);
    setFont(Theme::fontNormal());
    setStyleSheet(Theme::globalStyleSheet());
    qApp->setPalette(Theme::lightPalette());

    auto *root = new QWidget;
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(QMargins(0, 0, 0, 0));
    rootLayout->setSpacing(0);
    rootLayout->addWidget(buildSidebar(), 0);
    rootLayout->addWidget(buildRightPanel(), 1);
    setCentralWidget(root);

    statusBar()->showMessage("就绪");
    onStepChanged(0);
    connectSignals();
}

// ═══ 侧边栏 ═══════════════════════════════════════════════
QWidget *MainWindow::buildSidebar() {
    auto *sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(300);
    sidebar->setStyleSheet(Theme::sidebarStyleSheet());

    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(QMargins(14, 20, 14, 16));
    layout->setSpacing(4);

    // ── Logo ────────────────────────────────
    auto *logo = new QLabel("NekoECAT Converter");
    logo->setObjectName("sidebarTitle");
    auto *subtitle = new QLabel("ECAT → SSC Tool");
    subtitle->setObjectName("sidebarSubtitle");
    layout->addWidget(logo);
    layout->addWidget(subtitle);
    layout->addSpacing(4);

    auto *sep1 = new QFrame;
    sep1->setObjectName("separator");
    sep1->setFrameShape(QFrame::HLine);
    layout->addWidget(sep1);
    layout->addSpacing(4);

    // ── 步骤导航按钮 (3个) ─────────────────
    const char *stepLabels[] = { "文件导入", "身份配置", "规则策略" };
    const char *stepIcons[] = { "📁", "🏷", "⚙" };

    for (int i = 0; i < 3; ++i) {
        auto *btnContainer = new QWidget;
        auto *btnLayout = new QHBoxLayout(btnContainer);
        btnLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        btnLayout->setSpacing(8);

        auto *numLabel = new QLabel(QString::number(i + 1));
        numLabel->setObjectName("stepNum");
        m_stepNums[i] = numLabel;
        numLabel->setAlignment(Qt::AlignCenter);
        numLabel->setFixedSize(24, 24);

        auto *btn = new QPushButton(
            QString("%1  %2").arg(stepIcons[i]).arg(stepLabels[i]));
        btn->setObjectName("stepBtn");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);

        btnLayout->addWidget(numLabel);
        btnLayout->addWidget(btn, 1);
        m_stepBtns[i] = btn;

        int idx = i;
        connect(btn, &QPushButton::clicked, this, [this, idx]() {
            onStepChanged(idx);
        });

        layout->addWidget(btnContainer);
    }

    layout->addSpacing(4);

    // ── 步骤面板 (QStackedWidget) ──────────
    m_steps = new QStackedWidget;
    m_steps->addWidget(buildStep1());  // 步骤 0: 文件导入
    m_steps->addWidget(buildStep2());  // 步骤 1: 身份配置
    m_steps->addWidget(buildStep3());  // 步骤 2: 规则策略
    layout->addWidget(m_steps, 1);     // stretch=1 占剩余空间

    // ── 分隔线 + 导入按钮 ──────────────────
    auto *sep2 = new QFrame;
    sep2->setObjectName("separator");
    sep2->setFrameShape(QFrame::HLine);
    layout->addWidget(sep2);
    layout->addSpacing(4);

    m_importBtn = new QPushButton("📥  导  入");
    m_importBtn->setObjectName("importBtn");
    m_importBtn->setCursor(Qt::PointingHandCursor);
    m_importBtn->setMinimumHeight(44);
    connect(m_importBtn, &QPushButton::clicked, this, &MainWindow::onImport);
    layout->addWidget(m_importBtn);

    return sidebar;
}

// ═══ 步骤 0: 文件路径选择 ═════════════════════════════════
QWidget *MainWindow::buildStep1() {
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(QMargins(0, 4, 0, 4));
    layout->setSpacing(6);

    auto addPathRow = [&](const QString &label, QLineEdit *&edit,
                         const QString &filter, bool isDir = false) {
        auto *row = new QHBoxLayout;
        row->setSpacing(6);
        auto *lbl = new QLabel(label);
        lbl->setFixedWidth(55);
        lbl->setStyleSheet("color: #6b7280; font-size: 11px;");
        edit = new QLineEdit;
        edit->setPlaceholderText("点击浏览...");
        edit->setReadOnly(true);
        auto *btn = new QPushButton("...");
        btn->setFixedSize(30, 30);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [=]() {
            if (isDir) {
                QString d = QFileDialog::getExistingDirectory(this, "选择目录", edit->text());
                if (!d.isEmpty()) edit->setText(d);
            } else {
                QString f = QFileDialog::getOpenFileName(this, "选择文件", QString(), filter);
                if (!f.isEmpty()) edit->setText(f);
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

// ═══ 步骤 1: 设备身份配置 ═════════════════════════════════
QWidget *MainWindow::buildStep2() {
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(QMargins(0, 4, 0, 4));
    layout->setSpacing(8);

    // 当前身份 (只读)
    auto *curGroup = new QGroupBox("当前身份 (解析结果)");
    auto *curGrid = new QGridLayout(curGroup);
    curGrid->setSpacing(4);
    auto addRO = [&](int row, const QString &label, QLabel *&lbl) {
        auto *l = new QLabel(label);
        l->setStyleSheet("color: #6b7280; font-size: 11px;");
        lbl = new QLabel("--");
        lbl->setStyleSheet("color: #0d9488; font-size: 12px; font-weight: bold;");
        curGrid->addWidget(l, row, 0);
        curGrid->addWidget(lbl, row, 1);
    };
    addRO(0, "Vendor:", m_ov);
    addRO(1, "Product:", m_op);
    addRO(2, "Revision:", m_or);
    addRO(3, "Name:", m_on);
    layout->addWidget(curGroup);

    // 覆盖字段
    auto *ovGroup = new QGroupBox("覆盖配置 (可选)");
    auto *ovGrid = new QGridLayout(ovGroup);
    ovGrid->setSpacing(4);
    auto addOverride = [&](int row, const QString &label, QLineEdit *&edit, const QString &ph) {
        auto *l = new QLabel(label);
        l->setStyleSheet("color: #6b7280; font-size: 11px;");
        edit = new QLineEdit;
        edit->setPlaceholderText(ph);
        ovGrid->addWidget(l, row, 0);
        ovGrid->addWidget(edit, row, 1);
    };
    addOverride(0, "Vendor:", m_nv, "0x00001234");
    addOverride(1, "Product:", m_np, "0x00005678");
    addOverride(2, "Revision:", m_nr, "0x00000001");
    addOverride(3, "Name:", m_nn, "MyDevice");
    addOverride(4, "Addr:", m_phys, "0");
    layout->addWidget(ovGroup);

    layout->addStretch();
    return widget;
}

// ═══ 步骤 2: 转换策略 ═════════════════════════════════════
QWidget *MainWindow::buildStep3() {
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(QMargins(0, 4, 0, 4));
    layout->setSpacing(8);

    auto addGroup = [&](const QString &title, QButtonGroup *&grp,
                       const char *opts[], int count) {
        auto *g = new QGroupBox(title);
        auto *gl = new QVBoxLayout(g);
        gl->setSpacing(4);
        grp = new QButtonGroup(this);
        for (int i = 0; i < count; ++i) {
            auto *rb = new QRadioButton(opts[i]);
            grp->addButton(rb, i);
            gl->addWidget(rb);
        }
        grp->button(0)->setChecked(true);
        layout->addWidget(g);
    };

    const char *oct[] = { "保留原样 (默认)", "转为 TEXTLIST", "转为 UINT8 数组" };
    addGroup("OCTET_STRING", m_octGrp, oct, 3);

    const char *aln[] = { "自动对齐 (推荐)", "16-bit 对齐", "不强制对齐" };
    addGroup("字对齐", m_alnGrp, aln, 3);

    const char *com[] = { "包含通信对象", "排除通信对象", "仅 SDO" };
    addGroup("通信参数", m_comGrp, com, 3);

    const char *fxx[] = { "包含 Fxxx", "排除 Fxxx", "仅自动映射" };
    addGroup("Fxxx 对象", m_fxxGrp, fxx, 3);

    layout->addStretch();
    return widget;
}

// ═══ 右侧主内容区 ═════════════════════════════════════════
QWidget *MainWindow::buildRightPanel() {
    auto *panel = new QWidget;
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(QMargins(16, 16, 16, 12));
    layout->setSpacing(10);

    layout->addWidget(buildFilterBar());

    // ── 对象字典表格 ────────────────────────
    m_table = new QTableWidget;
    m_table->setColumnCount(9);
    QStringList headers = { "Index", "名称", "Sub", "类别", "PDO", "SM", "风险", "CoE R", "CoE W" };
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setStyleSheet(Theme::tableStyleSheet());
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    QHeaderView *hdr = m_table->horizontalHeader();
    hdr->setStretchLastSection(true);
    hdr->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int i = 2; i < 9; ++i)
        hdr->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    m_table->setMinimumHeight(260);
    layout->addWidget(m_table, 1);

    // ── PDO 校验 ───────────────────────────
    auto *pdoGroup = new QGroupBox("PDO / SM 校验");
    auto *pdoLayout = new QVBoxLayout(pdoGroup);
    m_pdoTxt = new QTextEdit;
    m_pdoTxt->setObjectName("pdo");
    m_pdoTxt->setReadOnly(true);
    m_pdoTxt->setMaximumHeight(100);
    m_pdoTxt->setPlaceholderText("导入 ESI 后显示 SM 校验信息...");
    pdoLayout->addWidget(m_pdoTxt);
    layout->addWidget(pdoGroup);

    layout->addWidget(buildBottomBar());
    return panel;
}

// ═══ 过滤栏 ═══════════════════════════════════════════════
QWidget *MainWindow::buildFilterBar() {
    auto *bar = new QWidget;
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(QMargins(4, 0, 4, 0));
    layout->setSpacing(10);

    auto *filterLabel = new QLabel("过滤:");
    filterLabel->setStyleSheet("color: #6b7280; font-size: 12px;");
    m_filter = new QComboBox;
    m_filter->addItems({ "全部对象", "仅风险", "仅 CoE", "仅 PDO", "仅 Fxxx", "已跳过" });
    m_filter->setMinimumWidth(120);

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("🔍 搜索 Index / 名称...");
    m_searchEdit->setMaximumWidth(240);

    m_objCnt = new QLabel("0 对象");
    m_objCnt->setStyleSheet("color: #0d9488; font-size: 12px; font-weight: bold;");

    layout->addWidget(filterLabel);
    layout->addWidget(m_filter);
    layout->addWidget(m_searchEdit);
    layout->addStretch();
    layout->addWidget(m_objCnt);
    return bar;
}

// ═══ 底部工具栏 ═══════════════════════════════════════════
QWidget *MainWindow::buildBottomBar() {
    auto *bar = new QWidget;
    bar->setObjectName("bottomBar");
    bar->setStyleSheet(Theme::bottomBarStyleSheet());
    auto *layout = new QVBoxLayout(bar);
    layout->setContentsMargins(QMargins(8, 8, 8, 8));
    layout->setSpacing(6);

    // 生成按钮行
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(6);
    auto addGenBtn = [&](const QString &label, const QString &obj, int mode) {
        auto *btn = new QPushButton(label);
        btn->setObjectName(obj);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, mode]() { onGenerate(mode); });
        btnRow->addWidget(btn);
    };
    addGenBtn("全部生成", "genBtnAll", 0);
    addGenBtn("SSC xlsx", "genBtn", 1);
    addGenBtn("TwinCAT", "genBtn", 2);
    addGenBtn("报告", "genBtn", 3);
    // 应用生成按钮样式
    bar->setStyleSheet(bar->styleSheet() + Theme::genButtonStyleSheet());
    addGenBtn("C 代码", "genBtn", 4);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    // 进度条
    m_prog = new QProgressBar;
    m_prog->setRange(0, 100);
    m_prog->setValue(0);
    m_prog->setTextVisible(true);
    layout->addWidget(m_prog);

    // 日志
    m_log = new QTextEdit;
    m_log->setObjectName("log");
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(140);
    m_log->setPlaceholderText("日志输出...");
    layout->addWidget(m_log);

    return bar;
}

// ═══ 信号槽连接 ═══════════════════════════════════════════
void MainWindow::connectSignals() {
    connect(m_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        if (m_devJson.isEmpty()) return;
        QJsonObject dev = QJsonDocument::fromJson(m_devJson.toUtf8()).object();
        updateObjectTable(dev["objects"].toObject());
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &) {
        if (m_devJson.isEmpty()) return;
        QJsonObject dev = QJsonDocument::fromJson(m_devJson.toUtf8()).object();
        updateObjectTable(dev["objects"].toObject());
    });
}

// ═══ 步骤切换 ═════════════════════════════════════════════
void MainWindow::onStepChanged(int step) {
    // 更新按钮 active 状态
    for (int i = 0; i < 3; ++i) {
        m_stepBtns[i]->setProperty("active", (i == step));
        m_stepBtns[i]->style()->unpolish(m_stepBtns[i]);
        m_stepBtns[i]->style()->polish(m_stepBtns[i]);

        // 更新步骤编号圆圈 active 状态
        m_stepNums[i]->setProperty("active", (i == step));
        m_stepNums[i]->style()->unpolish(m_stepNums[i]);
        m_stepNums[i]->style()->polish(m_stepNums[i]);
    }
    // 切换步骤面板
    if (m_steps) m_steps->setCurrentIndex(step);
}

// ═══ 导入 ═════════════════════════════════════════════════
/**
 * 导入按钮: 通过 QProcess 调用 Python parse_only() 解析 ESI/SDO。
 *
 * 流程:
 *   1. 检查文件路径
 *   2. 构造 Python 内联脚本
 *   3. QProcess 执行, 逐行读取输出
 *   4. 解析 JSON 结果, 填充身份/表格/PDO
 *
 * 注意: 使用 waitForReadyRead 代替 waitForFinished 避免 UI 阻塞。
 */
void MainWindow::onImport() {
    QString esi = m_esiEdit ? m_esiEdit->text().trimmed() : QString();
    QString sdo = m_sdoEdit ? m_sdoEdit->text().trimmed() : QString();

    if (esi.isEmpty() && sdo.isEmpty()) {
        setStatus("请至少选择一个 ESI 或 SDO 文件");
        return;
    }

    setStatus("解析中...");
    m_prog->setValue(10);
    m_importBtn->setEnabled(false);
    log("[导入] 启动解析...");

    QString appDir = QApplication::applicationDirPath();
    QString root = appDir;
    if (root.endsWith("/gui/build"))
        root = QFileInfo(root).path() + "/..";

    auto *worker = new ImportWorker(esi, sdo, root, this);
    connect(worker, &ImportWorker::log, this, &MainWindow::log);
    connect(worker, &ImportWorker::progress, m_prog, &QProgressBar::setValue);
    connect(worker, &ImportWorker::finished, this, [this](const QJsonObject &dev) {
        m_prog->setValue(100);
        m_devJson = QJsonDocument(dev).toJson();
        
        QJsonObject id = dev["identity"].toObject();
        auto hex = [](double val) -> QString {
            return QString("0x%1").arg((quint64)val, 8, 16, QChar('0'));
        };
        m_ov->setText(hex(id["vendor_id"].toDouble()));
        m_op->setText(hex(id["product_code"].toDouble()));
        m_or->setText(hex(id["revision_number"].toDouble()));
        m_on->setText(id["device_name"].toString());

        updateObjectTable(dev["objects"].toObject());
        updatePdoPanel(dev);

        int objCount = dev["objects"].toObject().keys().size();
        log(QString("[完成] 解析成功 — %1 个对象").arg(objCount));
        setStatus(QString("解析完成 — %1 个对象").arg(objCount));
        m_importBtn->setEnabled(true);
    });
    connect(worker, &ImportWorker::failed, this, [this](const QString &err) {
        log("[错误] " + err);
        setStatus("解析失败");
        m_prog->setValue(0);
        m_importBtn->setEnabled(true);
    });
    connect(worker, &ImportWorker::finished, worker, &QObject::deleteLater);
    worker->start();
}

// ═══ 对象表格更新 ═════════════════════════════════════════
void MainWindow::updateObjectTable(const QJsonObject &objs) {
    m_table->setRowCount(0);
    QStringList keys = objs.keys();
    std::sort(keys.begin(), keys.end(), [](const QString &a, const QString &b) {
        return a.toInt() < b.toInt();
    });

    int filterIdx = m_filter->currentIndex();
    QString searchText = m_searchEdit->text().trimmed().toLower();

    QStringList filteredKeys;
    for (const QString &key : keys) {
        QJsonObject obj = objs[key].toObject();
        QString name = obj["name"].toString().toLower();
        QString indexStr = QString("0x%1").arg(key.toInt(), 4, 16, QChar('0'));

        if (!searchText.isEmpty() && !name.contains(searchText) && !indexStr.contains(searchText))
            continue;

        bool risk = !obj["risk_flags"].toArray().isEmpty();
        bool coe = obj["coe_read"].toBool() || obj["coe_write"].toBool();
        bool pdo = obj["is_pdo_mapping_object"].toBool();
        bool isFxxx = key.toInt() >= 0xF000;

        switch (filterIdx) {
        case 1: if (!risk) continue; break;
        case 2: if (!coe) continue; break;
        case 3: if (!pdo) continue; break;
        case 4: if (!isFxxx) continue; break;
        }
        filteredKeys.append(key);
    }

    m_table->setRowCount(filteredKeys.size());
    for (int r = 0; r < filteredKeys.size(); ++r) {
        QJsonObject obj = objs[filteredKeys[r]].toObject();
        int idx = filteredKeys[r].toInt();
        bool risk = !obj["risk_flags"].toArray().isEmpty();
        bool coe = obj["coe_read"].toBool() || obj["coe_write"].toBool();
        bool pdo = obj["is_pdo_mapping_object"].toBool();

        QColor bgColor(255, 255, 255);
        if (risk)      bgColor = QColor(254, 242, 242);
        else if (coe)  bgColor = QColor(255, 251, 235);
        else if (pdo)  bgColor = QColor(240, 253, 244);
        else if (r % 2 == 1) bgColor = QColor(248, 249, 251);

        auto setCell = [&](int col, const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setBackground(QBrush(bgColor));
            item->setFont(Theme::fontMono(11));
            m_table->setItem(r, col, item);
        };

        // Index 列 (蓝色高亮)
        auto *idxItem = new QTableWidgetItem(
            QString("0x%1").arg(idx, 4, 16, QChar('0')).toUpper());
        idxItem->setBackground(QBrush(bgColor));
        idxItem->setForeground(QBrush(QColor(37, 99, 235)));
        idxItem->setFont(Theme::fontMono(11));
        m_table->setItem(r, 0, idxItem);

        setCell(1, obj["name"].toString());
        setCell(2, QString::number(obj["subindices"].toArray().size()));
        setCell(3, obj["category"].toString());
        setCell(4, pdo ? "✓" : "");
        setCell(5, obj["is_sm_object"].toBool() ? "✓" : "");

        // 风险标志 (红色文字)
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

// ═══ PDO/SM 校验 ═════════════════════════════════════════
void MainWindow::updatePdoPanel(const QJsonObject &dev) {
    QStringList lines;
    for (const auto &sv : dev["sm_configs"].toArray()) {
        QJsonObject sm = sv.toObject();
        int def = sm["default_size"].toInt();
        int calc = 0;
        for (const auto &pv : sm["pdos"].toArray())
            for (const auto &ev : pv.toObject()["entries"].toArray())
                calc += ev.toObject()["bit_length"].toInt();
        calc = (calc + 7) / 8;
        bool ok = (def == calc || calc == 0);
        lines << QString("SM%1 [%2]: 默认=%3B  计算=%4B  %5")
                    .arg(sm["index"].toInt())
                    .arg(sm["pdo_type"].toString())
                    .arg(def).arg(calc)
                    .arg(ok ? "✅ OK" : "❌ MISMATCH");
    }
    m_pdoTxt->setPlainText(lines.isEmpty() ? "(无 SM 配置)" : lines.join("\n"));
}

// ═══ 生成 ═════════════════════════════════════════════════
void MainWindow::onGenerate(int mode) {
    if (m_devJson.isEmpty()) {
        setStatus("请先导入 ESI/SDO 文件");
        return;
    }

    setStatus("转换中...");
    m_prog->setValue(10);
    log(QString("[生成] 启动... (模式=%1)").arg(mode));

    QVariantMap identity;
    if (m_nv && !m_nv->text().trimmed().isEmpty())
        identity["vendor_id"] = m_nv->text().trimmed();
    if (m_np && !m_np->text().trimmed().isEmpty())
        identity["product_code"] = m_np->text().trimmed();
    if (m_nr && !m_nr->text().trimmed().isEmpty())
        identity["revision"] = m_nr->text().trimmed();
    if (m_nn && !m_nn->text().trimmed().isEmpty())
        identity["device_name"] = m_nn->text().trimmed();

    m_worker = new ConvertWorker(
        m_esiEdit ? m_esiEdit->text().trimmed() : QString(),
        m_sdoEdit ? m_sdoEdit->text().trimmed() : QString(),
        m_tplEdit ? m_tplEdit->text().trimmed() : QString(),
        m_outEdit ? m_outEdit->text().trimmed() : "out",
        identity, QVariantMap(), this
    );

    connect(m_worker, &ConvertWorker::log, this, [this](const QString &m) { log(m); });
    connect(m_worker, &ConvertWorker::progress, m_prog, &QProgressBar::setValue);
    connect(m_worker, &ConvertWorker::finishedOk, this, [this](const QString &d) {
        log("[完成] " + d);
        setStatus("转换完成");
        m_prog->setValue(100);
    });
    connect(m_worker, &ConvertWorker::failed, this, [this](const QString &e) {
        log("[失败] " + e);
        setStatus("转换失败");
        m_prog->setValue(0);
    });
    connect(m_worker, &ConvertWorker::finished, m_worker, &QObject::deleteLater);
    m_worker->start();
}

// ═══ 辅助方法 ═════════════════════════════════════════════
void MainWindow::setStatus(const QString &msg) {
    statusBar()->showMessage(msg);
}

void MainWindow::log(const QString &msg) {
    m_log->append(msg);
    auto *sb = m_log->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());
}
