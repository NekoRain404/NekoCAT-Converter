/**
 * MainWindow — 主窗口。
 *
 * 布局参考 docs/ui/ 参考图：
 *   ┌──────────────┬──────────────────────────────────┐
 *   │ 左侧边栏      │  右侧内容区                        │
 *   │ 标题          │  对象字典表格 (9列, 可过滤)          │
 *   │ Step1:文件    │  PDO/SM 校验                      │
 *   │ Step2:身份    ├──────────────────────────────────┤
 *   │ Step3:规则    │  底部: 生成按钮 + 进度条 + 日志     │
 *   │ [导入]        │                                  │
 *   └──────────────┴──────────────────────────────────┘
 *
 * 解耦: C++ 通过 QProcess 调 Python CLI, 零内部依赖。
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

class ConvertWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onImport();                         // 导入按钮
    void onGenerate(int mode);               // 生成按钮
    void onStepChanged(int step);            // 侧边栏步骤切换

private:
    // 构建
    QWidget *buildSidebar();
    QWidget *buildRightPanel();
    QWidget *buildBottomBar();
    void connectSignals();
    void applyTheme();

    // 更新
    void updateObjectTable(const QJsonObject &objects);
    void updatePdoPanel(const QJsonObject &device);
    void setStatus(const QString &msg);
    void log(const QString &msg);

    // ── 侧边栏 ──────────────────────────────
    QStackedWidget *m_steps;
    QPushButton    *m_stepBtns[3];
    QPushButton    *m_importBtn;
    QLabel         *m_fileStatus;

    // Step 1
    QLineEdit *m_esiEdit, *m_sdoEdit, *m_tplEdit, *m_outEdit;
    // Step 2
    QLabel *m_ov, *m_op, *m_or, *m_on;
    QLineEdit *m_nv, *m_np, *m_nr, *m_nn, *m_phys;
    QButtonGroup *m_modeGrp;
    // Step 3
    QButtonGroup *m_octGrp, *m_alnGrp, *m_comGrp, *m_fxxGrp;

    // ── 右侧 ────────────────────────────────
    QTableWidget *m_table;
    QComboBox    *m_filter;
    QLabel       *m_objCnt;
    QTextEdit    *m_pdoTxt;

    // ── 底部 ─────────────────────────────────
    QProgressBar *m_prog;
    QTextEdit    *m_log;

    // ── 后台 ─────────────────────────────────
    ConvertWorker *m_worker;
    QString m_devJson;
};
#endif
