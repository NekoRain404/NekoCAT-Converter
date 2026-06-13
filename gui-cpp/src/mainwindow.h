/**
 * MainWindow — 主窗口：完整的专业级布局。
 *
 * 参考 docs/ui/ 参考图：
 *   ┌──────────────┬────────────────────────────────────┐
 *   │ 左侧边栏      │  右侧内容区                          │
 *   │ ┌──────────┐ │  ┌────────────────────────────────┐ │
 *   │ │ Step 1   │ │  │  对象字典表格 (可过滤/排序)      │ │
 *   │ │ 文件导入  │ │  │  Index|Name|Sub|Cat|PDO|Risk  │ │
 *   │ ├──────────┤ │  │  0x6000|DI|2|std| |           │ │
 *   │ │ Step 2   │ │  │  0x7009|Act|3|mfg|Rx|         │ │
 *   │ │ 身份配置  │ │  └────────────────────────────────┘ │
 *   │ ├──────────┤ │  ┌────────────────────────────────┐ │
 *   │ │ Step 3   │ │  │  PDO/SM 校验信息                │ │
 *   │ │ 规则策略  │ │  │  SM2: 13字节 OK                │ │
 *   │ └──────────┘ │  │  SM3: 53字节 OK                │ │
 *   │              │  └────────────────────────────────┘ │
 *   │ [导入按钮]   │  [全部生成] [SSC xlsx] [报告] [ESI]  │
 *   │ v0.1.0       │  进度条 ████████░░░░ 60%            │
 *   └──────────────┴────────────────────────────────────┘
 *
 * 解耦原则：通过 Worker 调用 Python CLI，不耦合解析/转换逻辑。
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QProgressBar>
#include <QTextEdit>
#include <QPushButton>
#include <QVector>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QButtonGroup>
#include <QCheckBox>
#include <QRadioButton>
#include <QListWidget>
#include <QVariantMap>
#include <QTimer>

class ConvertWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    /** 导入按钮：解析 ESI/SDO 并更新所有面板 */
    void onImport();
    /** 生成按钮：启动后台转换 */
    void onGenerate(int mode);
    /** 侧边栏步骤切换 */
    void onStepChanged(int step);

private:
    // ── 构建方法 ──────────────────────────────
    QWidget *buildSidebar();
    QWidget *buildStep1FileImport();
    QWidget *buildStep2Identity();
    QWidget *buildStep3Strategy();
    QWidget *buildRightPanel();
    QWidget *buildObjectTable();
    QWidget *buildPdoPanel();
    QWidget *buildBottomBar();
    void connectSignals();
    void applyDarkTheme();

    /** 更新对象字典表格 */
    void updateObjectTable(const QJsonObject &objects);
    /** 更新 PDO/SM 面板 */
    void updatePdoPanel(const QJsonObject &device);
    /** 更新状态栏 */
    void setStatus(const QString &msg);

    // ── 侧边栏控件 ────────────────────────────
    QStackedWidget *m_stepStack;     // 步骤切换
    QPushButton *m_stepButtons[3];   // 步骤导航按钮
    QPushButton *m_importBtn;        // 导入按钮

    // Step 1: 文件导入
    QLineEdit *m_esiEdit, *m_sdoEdit, *m_tplEdit, *m_outEdit;
    QLabel *m_fileStatus;            // 文件状态指示

    // Step 2: 身份配置
    QLabel *m_origVendor, *m_origProduct, *m_origRevision, *m_origName;
    QLineEdit *m_newVendor, *m_newProduct, *m_newRevision, *m_newName, *m_newPhysics;
    QButtonGroup *m_modeGroup;

    // Step 3: 规则策略
    QButtonGroup *m_octetGroup, *m_alignGroup, *m_commGroup, *m_fxxxGroup;

    // 右侧：对象表格
    QTableWidget *m_objTable;
    QComboBox *m_filterCombo;
    QLabel *m_objCountLabel;

    // 右侧：PDO/SM 面板
    QTextEdit *m_pdoText;

    // 底部
    QProgressBar *m_progress;
    QTextEdit *m_log;
    QLabel *m_statusLabel;

    // ── 后台线程 ──────────────────────────────
    ConvertWorker *m_worker;

    /** 缓存的设备 JSON（供生成时使用） */
    QString m_deviceJson;
};

#endif // MAINWINDOW_H
