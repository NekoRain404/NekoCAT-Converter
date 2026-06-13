/**
 * MainWindow — 主窗口实现。
 *
 * 布局：左侧导航栏 + 右侧 QStackedWidget + 底部日志/进度条。
 */
#include "mainwindow.h"
#include "worker.h"
#include "pages/fileselectpage.h"
#include "pages/identitypage.h"
#include "pages/rulepage.h"
#include "pages/objecttablepage.h"
#include "pages/pdopage.h"
#include "pages/generatepage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QDir>
#include <QMargins>

static const char *DARK_STYLE = R"(
QMainWindow, QWidget {
    background-color: #2b2b2b; color: #e0e0e0; font-size: 13px;
}
QGroupBox {
    border: 1px solid #555; border-radius: 4px;
    margin-top: 10px; padding-top: 14px; font-weight: bold; color: #aaa;
}
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
QLineEdit {
    background-color: #3c3c3c; border: 1px solid #555;
    border-radius: 3px; padding: 5px; color: #e0e0e0;
}
QPushButton {
    background-color: #0d6efd; color: white; border: none;
    border-radius: 4px; padding: 6px 16px; font-weight: bold;
}
QPushButton:hover { background-color: #0b5ed7; }
QPushButton:pressed { background-color: #0a58ca; }
QPushButton:disabled { background-color: #555; color: #888; }
QPushButton#sidebarBtn {
    background-color: transparent; text-align: left;
    padding: 10px 14px; border-radius: 6px; font-size: 13px; color: #ccc;
}
QPushButton#sidebarBtn:hover { background-color: #3c3c3c; }
QPushButton#sidebarBtn[active="true"] {
    background-color: #0d6efd; color: white; font-weight: bold;
}
QRadioButton { color: #e0e0e0; spacing: 6px; }
QCheckBox { color: #e0e0e0; spacing: 6px; }
QTextEdit {
    background-color: #1e1e1e; border: 1px solid #444; color: #d4d4d4;
    font-family: Consolas, monospace; font-size: 12px;
}
QTableWidget {
    background-color: #1e1e1e; alternate-background-color: #252525;
    color: #e0e0e0; gridline-color: #444; border: 1px solid #444;
}
QHeaderView::section {
    background-color: #333; color: #e0e0e0; border: 1px solid #444;
    padding: 5px; font-weight: bold;
}
QProgressBar {
    border: 1px solid #555; border-radius: 4px; text-align: center; color: white;
}
QProgressBar::chunk { background-color: #0d6efd; border-radius: 3px; }
QStatusBar { background-color: #333; color: #aaa; }
)";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_worker(nullptr) {
    setWindowTitle("NekoECAT Converter");
    setMinimumSize(1100, 700);
    resize(1200, 750);
    setStyleSheet(DARK_STYLE);

    m_filePage     = new FileSelectPage;
    m_identityPage = new IdentityPage;
    m_rulePage     = new RulePage;
    m_objPage      = new ObjectTablePage;
    m_pdoPage      = new PdoPage;
    m_genPage      = new GeneratePage;

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_filePage);
    m_stack->addWidget(m_identityPage);
    m_stack->addWidget(m_rulePage);
    m_stack->addWidget(m_objPage);
    m_stack->addWidget(m_pdoPage);
    m_stack->addWidget(m_genPage);

    QWidget *sidebar = buildSidebar();

    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(120);

    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setMaximumHeight(20);

    QWidget *bottom = new QWidget;
    auto *bl = new QVBoxLayout(bottom);
    bl->setContentsMargins(QMargins(0, 0, 0, 0));
    bl->addWidget(m_log);
    bl->addWidget(m_progress);

    QWidget *right = new QWidget;
    auto *rl = new QVBoxLayout(right);
    rl->setContentsMargins(QMargins(0, 0, 0, 0));
    rl->addWidget(m_stack, 1);
    rl->addWidget(bottom, 0);

    QWidget *mainWidget = new QWidget;
    auto *ml = new QHBoxLayout(mainWidget);
    ml->setContentsMargins(QMargins(0, 0, 0, 0));
    ml->setSpacing(0);
    ml->addWidget(sidebar, 0);
    ml->addWidget(right, 1);

    setCentralWidget(mainWidget);
    statusBar()->showMessage("就绪");
    connectSignals();
    setActivePage(0);
}

QWidget *MainWindow::buildSidebar() {
    QWidget *sidebar = new QWidget;
    sidebar->setFixedWidth(240);
    sidebar->setStyleSheet("background-color: #1e1e1e;");
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(QMargins(12, 16, 12, 16));
    layout->setSpacing(4);

    auto *title = new QLabel("NekoECAT Converter");
    title->setStyleSheet("font-size: 16px; font-weight: bold; color: #0d6efd;");
    auto *subtitle = new QLabel("ECAT → SSC Tool");
    subtitle->setStyleSheet("font-size: 11px; color: #888;");
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(16);

    struct StepInfo { const char *step; const char *name; };
    StepInfo steps[] = {
        {"Step 1", "文件导入"}, {"Step 2", "身份配置"}, {"Step 3", "规则策略"},
        {"Step 4", "对象字典"}, {"Step 5", "PDO/SM 校验"}, {"Step 6", "生成输出"},
    };
    for (int i = 0; i < 6; ++i) {
        auto *btn = new QPushButton(QString("%1: %2").arg(steps[i].step, steps[i].name));
        btn->setObjectName("sidebarBtn");
        btn->setCursor(Qt::PointingHandCursor);
        int idx = i;
        connect(btn, &QPushButton::clicked, this, [this, idx]() { setActivePage(idx); });
        m_sidebarButtons.append(btn);
        layout->addWidget(btn);
    }
    layout->addStretch();
    auto *ver = new QLabel("v0.1.0");
    ver->setStyleSheet("color: #555; font-size: 11px;");
    layout->addWidget(ver);
    return sidebar;
}

void MainWindow::connectSignals() {
    connect(m_filePage, &FileSelectPage::parseRequested, this, &MainWindow::onParse);
    connect(m_genPage,  &GeneratePage::generateAll,       this, &MainWindow::onGenerateAll);
}

void MainWindow::setActivePage(int index) {
    m_stack->setCurrentIndex(index);
    for (int i = 0; i < m_sidebarButtons.size(); ++i) {
        m_sidebarButtons[i]->setProperty("active", i == index);
        m_sidebarButtons[i]->style()->unpolish(m_sidebarButtons[i]);
        m_sidebarButtons[i]->style()->polish(m_sidebarButtons[i]);
    }
}

void MainWindow::onParse(const QVariantMap &paths) {
    logMessage("正在解析文件...");
    m_progress->setValue(10);

    QString projectRoot = QDir::currentPath();
    QString pyScript = R"(
import sys, json
sys.path.insert(0, '.')
from nekoecat.core import parse_only
esi = sys.argv[1] if len(sys.argv) > 1 and sys.argv[1] != 'None' else None
sdo = sys.argv[2] if len(sys.argv) > 2 and sys.argv[2] != 'None' else None
device = parse_only(esi_path=esi, sdo_path=sdo)
print(json.dumps(device.model_dump(mode='json'), ensure_ascii=False))
)";

    QProcess proc;
    proc.setWorkingDirectory(projectRoot);
    proc.start("python3", {"-c", pyScript,
        paths["esi_path"].toString().isEmpty() ? "None" : paths["esi_path"].toString(),
        paths["sdo_path"].toString().isEmpty() ? "None" : paths["sdo_path"].toString()});
    proc.waitForFinished(30000);

    if (proc.exitCode() != 0) {
        logMessage("解析失败: " + QString::fromUtf8(proc.readAllStandardError()));
        m_progress->setValue(0);
        return;
    }

    m_deviceJson = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    m_progress->setValue(50);

    QJsonDocument doc = QJsonDocument::fromJson(m_deviceJson.toUtf8());
    QJsonObject dev = doc.object();
    QJsonObject ident = dev["identity"].toObject();

    QVariantMap result;
    result["厂商 ID"] = QString("0x%1").arg((quint64)ident["vendor_id"].toDouble(), 8, 16, QChar('0'));
    result["产品代码"] = QString("0x%1").arg((quint64)ident["product_code"].toDouble(), 8, 16, QChar('0'));
    result["修订号"] = QString("0x%1").arg((quint64)ident["revision_number"].toDouble(), 8, 16, QChar('0'));
    result["设备名称"] = ident["device_name"].toString();
    result["对象数量"] = dev["objects"].toObject().keys().size();
    m_filePage->showParseResult(result);

    QVariantMap identMap;
    identMap["Vendor ID"] = result["厂商 ID"];
    identMap["Product Code"] = result["产品代码"];
    identMap["Revision"] = result["修订号"];
    identMap["Device Name"] = result["设备名称"];
    m_identityPage->setOriginalIdentity(identMap);

    QJsonObject objects = dev["objects"].toObject();
    QList<QVariantMap> objRows;
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        QJsonObject obj = it.value().toObject();
        QVariantMap row;
        int idx = it.key().toInt();
        row["Index"] = QString("0x%1").arg(idx, 4, 16, QChar('0')).toUpper();
        row["index_int"] = idx;
        row["Name"] = obj["name"].toString();
        row["SubCount"] = obj["subindices"].toArray().size();
        row["Category"] = obj["category"].toString();
        row["pdo_map"] = obj["is_pdo_mapping_object"].toBool();
        row["SM Obj"] = obj["is_sm_object"].toBool();
        QJsonArray risks = obj["risk_flags"].toArray();
        row["Risk"] = risks.isEmpty() ? "" : risks[0].toString();
        row["CoeRead"] = obj["coe_read"].toBool() ? "TRUE" : "";
        row["CoeWrite"] = obj["coe_write"].toBool() ? "TRUE" : "";
        row["skipped"] = obj["skipped"].toBool();
        row["risk"] = !risks.isEmpty();
        objRows.append(row);
    }
    m_objPage->setObjects(objRows);
    updatePdoPage(m_deviceJson);

    m_progress->setValue(60);
    logMessage(QString("解析完成: %1 个对象").arg(objects.keys().size()));
    statusBar()->showMessage("解析完成");
}

void MainWindow::onGenerateAll() {
    if (m_deviceJson.isEmpty()) {
        logMessage("错误: 请先解析文件");
        return;
    }
    logMessage("开始转换...");
    m_progress->setValue(10);

    QVariantMap paths = m_filePage->getPaths();
    QVariantMap identity = m_identityPage->getConfig();
    QVariantMap rules = m_rulePage->getConfig();

    m_worker = new ConvertWorker(
        paths["esi_path"].toString(), paths["sdo_path"].toString(),
        paths["template_path"].toString(), paths["output_dir"].toString(),
        identity, rules, this);

    connect(m_worker, &ConvertWorker::log, this, &MainWindow::logMessage);
    connect(m_worker, &ConvertWorker::progress, m_progress, &QProgressBar::setValue);
    connect(m_worker, &ConvertWorker::finishedOk, this, [this](const QString &dir) {
        logMessage("完成: " + dir);
        m_genPage->setStatus("转换完成!");
        m_progress->setValue(100);
        statusBar()->showMessage("转换完成");
    });
    connect(m_worker, &ConvertWorker::failed, this, [this](const QString &err) {
        logMessage("失败: " + err);
        m_genPage->setStatus("失败: " + err);
        m_progress->setValue(0);
        statusBar()->showMessage("转换失败");
    });
    connect(m_worker, &ConvertWorker::finished, m_worker, &QObject::deleteLater);
    m_worker->start();
    statusBar()->showMessage("转换中...");
    m_genPage->setStatus("转换进行中...");
}

void MainWindow::updatePdoPage(const QString &json) {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject dev = doc.object();
    QJsonArray sms = dev["sm_configs"].toArray();
    QStringList lines;
    for (const auto &smV : sms) {
        QJsonObject sm = smV.toObject();
        int defSize = sm["default_size"].toInt();
        int calcSize = 0;
        for (const auto &pdoV : sm["pdos"].toArray())
            for (const auto &eV : pdoV.toObject()["entries"].toArray())
                calcSize += eV.toObject()["bit_length"].toInt();
        calcSize = (calcSize + 7) / 8;
        QString status = (defSize == calcSize || calcSize == 0) ? "OK" : "MISMATCH";
        lines << QString("SM%1 [%2]: 默认=%3字节, 计算=%4字节 — %5")
                     .arg(sm["index"].toInt()).arg(sm["pdo_type"].toString())
                     .arg(defSize).arg(calcSize).arg(status);
    }
    m_pdoPage->setSmInfo(lines.join("\n"));
}

void MainWindow::logMessage(const QString &msg) {
    m_log->append(msg);
}
