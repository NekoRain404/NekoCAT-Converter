/**
 * MainWindow — 主窗口：左侧导航栏 + 右侧内容区 + 底部日志/进度。
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

class FileSelectPage;
class IdentityPage;
class RulePage;
class ObjectTablePage;
class PdoPage;
class GeneratePage;
class ConvertWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void onParse(const QVariantMap &paths);
    void onGenerateAll();
    void setActivePage(int index);

private:
    QWidget *buildSidebar();
    void connectSignals();
    void updatePdoPage(const QString &json);
    void logMessage(const QString &msg);

    FileSelectPage   *m_filePage;
    IdentityPage     *m_identityPage;
    RulePage         *m_rulePage;
    ObjectTablePage  *m_objPage;
    PdoPage          *m_pdoPage;
    GeneratePage     *m_genPage;
    QStackedWidget   *m_stack;
    QVector<QPushButton*> m_sidebarButtons;
    QTextEdit    *m_log;
    QProgressBar *m_progress;
    ConvertWorker *m_worker;
    QString m_deviceJson;
};

#endif // MAINWINDOW_H
