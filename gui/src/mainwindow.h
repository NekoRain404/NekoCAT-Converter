/**
 * MainWindow — 主窗口 (重构版)。
 *
 * 布局参考 docs/ui/ 参考图:
 *
 *   ┌──────────────┬──────────────────────────────────────┐
 *   │  左侧边栏 300px │  右侧内容区                          │
 *   │  ┌──────────┐ │  ┌──────────────────────────────┐  │
 *   │  │ Logo     │ │  │  过滤栏: [分类▼] [搜索] [计数] │  │
 *   │  │ Subtitle │ │  ├──────────────────────────────┤  │
 *   │  │          │ │  │                              │  │
 *   │  │ Step1 文件│ │  │  对象字典表格 (9列, 可过滤)    │  │
 *   │  │ Step2 身份│ │  │  行色: 正常/交替/风险/CoE/PDO │  │
 *   │  │ Step3 策略│ │  │                              │  │
 *   │  │          │ │  ├──────────────────────────────┤  │
 *   │  │ [导入]   │ │  │  PDO/SM 校验文本区            │  │
 *   │  └──────────┘ │  ├──────────────────────────────┤  │
 *   │              │  │  生成按钮 | 进度条 | 日志       │  │
 *   │              │  └──────────────────────────────┘  │
 *   └──────────────┴──────────────────────────────────────┘
 *
 * 解耦:
 *   - C++ 前端 通过 QProcess 调 Python CLI, 无内部 Python 依赖
 *   - 样式集中管理于 core/theme, 主窗口只负责布局与交互
 *   - Worker 线程独立, 通过信号槽与 UI 通信
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
#include <QRadioButton>
#include <QVariantMap>

class ConvertWorker; // 前向声明, 避免头文件依赖

/**
 * MainWindow — 应用主窗口。
 *
 * 职责:
 *   1. 构建侧边栏 (步骤导航 + 配置面板)
 *   2. 构建右侧内容区 (过滤栏 + 表格 + 校验 + 生成)
 *   3. 连接信号槽, 协调 Worker 线程
 *   4. 更新 UI (表格着色、进度、日志)
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    /** 侧边栏步骤切换 */
    void onStepChanged(int step);
    /** 导入按钮: 调用 Python parse_only */
    void onImport();
    /** 生成按钮: 启动 Worker 线程 */
    void onGenerate(int mode);

private:
    // ── 构建方法 ─────────────────────────────
    /** 构建左侧边栏 (Logo + 步骤按钮 + 导入按钮) */
    QWidget *buildSidebar();
    /** 构建步骤1面板: 文件路径选择 */
    QWidget *buildStep1();
    /** 构建步骤2面板: 设备身份配置 */
    QWidget *buildStep2();
    /** 构建步骤3面板: 转换策略选项 */
    QWidget *buildStep3();
    /** 构建右侧主内容区 */
    QWidget *buildRightPanel();
    /** 构建过滤栏 (分类下拉 + 搜索 + 对象计数) */
    QWidget *buildFilterBar();
    /** 构建底部工具栏 (生成按钮 + 进度条 + 日志) */
    QWidget *buildBottomBar();
    /** 连接所有信号槽 */
    void connectSignals();

    // ── 更新方法 ─────────────────────────────
    /** 根据 JSON 更新对象表格 (含行着色) */
    void updateObjectTable(const QJsonObject &objects);
    /** 更新 PDO/SM 校验面板 */
    void updatePdoPanel(const QJsonObject &device);
    /** 设置状态栏消息 */
    void setStatus(const QString &msg);
    /** 追加日志 */
    void log(const QString &msg);

    // ═══ 成员变量 ═══════════════════════════════════════════

    // ── 侧边栏 ──────────────────────────────
    QStackedWidget *m_steps;        ///< 步骤面板切换器
    QPushButton    *m_stepBtns[3];  ///< 3个步骤导航按钮
    QPushButton    *m_importBtn;    ///< 导入按钮 (红色渐变)

    // Step 1: 文件路径
    QLineEdit *m_esiEdit;          ///< ESI XML 路径
    QLineEdit *m_sdoEdit;          ///< SDO 文件路径
    QLineEdit *m_tplEdit;          ///< SSC 模板路径
    QLineEdit *m_outEdit;          ///< 输出目录

    // Step 2: 设备身份 (只读, 从解析结果填充)
    QLabel *m_ov;                  ///< 当前 Vendor ID (只读)
    QLabel *m_op;                  ///< 当前 Product Code (只读)
    QLabel *m_or;                  ///< 当前 Revision (只读)
    QLabel *m_on;                  ///< 当前 Device Name (只读)
    QLineEdit *m_nv;               ///< 覆盖 Vendor ID
    QLineEdit *m_np;               ///< 覆盖 Product Code
    QLineEdit *m_nr;               ///< 覆盖 Revision
    QLineEdit *m_nn;               ///< 覆盖 Device Name
    QLineEdit *m_phys;             ///< 物理地址
    QButtonGroup *m_modeGrp;       ///< 映射模式

    // Step 3: 策略选项
    QButtonGroup *m_octGrp;        ///< OCTET_STRING 策略
    QButtonGroup *m_alnGrp;        ///< 字对齐策略
    QButtonGroup *m_comGrp;        ///< 通信参数策略
    QButtonGroup *m_fxxGrp;        ///< Fxxx 对象策略

    // ── 右侧内容区 ──────────────────────────
    QTableWidget *m_table;         ///< 对象字典表格 (9列)
    QComboBox    *m_filter;        ///< 分类过滤下拉框
    QLabel       *m_objCnt;        ///< 对象计数标签
    QLineEdit    *m_searchEdit;    ///< 搜索框
    QTextEdit    *m_pdoTxt;        ///< PDO/SM 校验文本

    // ── 底部 ─────────────────────────────────
    QProgressBar *m_prog;          ///< 进度条 (渐变)
    QTextEdit    *m_log;           ///< 实时日志

    // ── 后台 ─────────────────────────────────
    ConvertWorker *m_worker;       ///< 转换工作线程
    QString m_devJson;             ///< 解析后的设备 JSON 缓存
};

#endif // MAINWINDOW_H
