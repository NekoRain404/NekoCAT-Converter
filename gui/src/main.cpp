/**
 * NekoECAT Converter — C++ Qt5 GUI 入口。
 *
 * 用法：
 *   ./NekoECATConverter                          # 启动 GUI
 *   ./NekoECATConverter --screenshot /tmp/gui.png # 启动 + 截图 + 退出
 */
#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QPixmap>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("NekoECAT Converter");
    app.setApplicationVersion("0.3.0");
    app.setStyle("Fusion");

    QCommandLineParser parser;
    parser.setApplicationDescription("NekoECAT Converter — ESI/SDO → SSC Tool xlsx");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption screenshotOpt(
        {"s", "screenshot"},
        "启动后截取窗口到指定路径, 截图后自动退出",
        "PATH"
    );
    parser.addOption(screenshotOpt);
    parser.process(app);

    MainWindow window;
    window.show();

    if (parser.isSet(screenshotOpt)) {
        QString path = parser.value(screenshotOpt);
        QTimer::singleShot(1500, [&window, &path]() {
            QPixmap pixmap = window.grab();
            if (!pixmap.isNull()) {
                pixmap.save(path, "PNG");
                fprintf(stderr, "Screenshot saved: %s\n", path.toUtf8().constData());
            }
            QApplication::quit();
        });
    }

    return app.exec();
}
