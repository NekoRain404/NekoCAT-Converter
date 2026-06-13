/**
 * NekoECAT Converter — C++ Qt5 GUI 入口。
 *
 * 用法：
 *   ./NekoECATConverter                          # 启动 GUI
 *   ./NekoECATConverter --screenshot /tmp/gui.png # 启动 + 截图
 *   ./NekoECATConverter --help
 */
#include <QDebug>
#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QProcess>
#include <QDateTime>
#include "mainwindow.h"

/**
 * 活动窗口截图 — 使用 xdotool + ImageMagick import。
 *
 * 原理：xdotool getactivewindow 获取当前焦点窗口 ID，
 * 然后 import -window <id> 精确截图该窗口。
 */
static QString captureActiveWindow(const QString &path) {
    // 获取活动窗口 ID
    QProcess xdotool;
    xdotool.start("xdotool", {"getactivewindow"});
    xdotool.waitForFinished(2000);
    QString winId = QString::fromUtf8(xdotool.readAllStandardOutput()).trimmed();

    if (winId.isEmpty()) {
        // 回退：截全屏
        QProcess::execute("import", {"-window", "root", path});
    } else {
        QProcess::execute("import", {"-window", winId, path});
    }
    return path;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("NekoECAT Converter");
    app.setApplicationVersion("0.1.0");
    app.setStyle("Fusion");

    // ── 命令行参数 ──────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription("NekoECAT Converter — ESI/SDO → SSC Tool xlsx");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption screenshotOpt(
        {"s", "screenshot"},
        "启动后截取活动窗口到指定路径",
        "PATH"
    );
    parser.addOption(screenshotOpt);
    parser.process(app);

    // ── 启动主窗口 ──────────────────────────
    MainWindow window;
    window.show();

    // ── 截图：延迟 500ms 等窗口渲染完成 ────
    if (parser.isSet(screenshotOpt)) {
        QString path = parser.value(screenshotOpt);
        QTimer::singleShot(500, [&path]() {
            QString result = captureActiveWindow(path);
            if (!result.isEmpty())
                qDebug() << "Screenshot saved:" << result;
            else
                qWarning() << "Screenshot failed";
        });
    }

    return app.exec();
}
