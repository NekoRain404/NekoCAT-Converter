/**
 * ConvertWorker — 通过 QProcess 调用 Python CLI 完成转换。
 *
 * 调用方式: python3 cli.py convert --esi X --sdo Y --out Z
 * 输出目录中的 convert.log 包含详细日志。
 *
 * 错误处理:
 *   - Python 进程无法启动 → failed 信号
 *   - 非零退出码 → failed 信号 + stderr
 *   - 输出目录不存在 → failed 信号
 */
#include "worker.h"
#include <QProcess>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>

ConvertWorker::ConvertWorker(
    const QString &esiPath,
    const QString &sdoPath,
    const QString &templatePath,
    const QString &outputDir,
    const QVariantMap &identity,
    const QVariantMap &rules,
    QObject *parent)
    : QThread(parent)
    , m_esiPath(esiPath)
    , m_sdoPath(sdoPath)
    , m_templatePath(templatePath)
    , m_outputDir(outputDir)
    , m_identity(identity)
    , m_rules(rules)
{
}

// ═══ 定位项目根目录 ═══════════════════════════════════════
/**
 * 从可执行文件路径推断项目根目录:
 *   - 如果在 build/ 目录 → 回退两级
 *   - 否则 → 使用当前工作目录
 */
QString ConvertWorker::findProjectRoot() const {
    QString appDir = QCoreApplication::applicationDirPath();
    QString root = appDir;

    // 如果在 build/ 目录下, 回退两级
    if (root.endsWith("/gui-cpp/build"))
        root = QFileInfo(root).path() + "/..";

    return root;
}

// ═══ 主线程 ═══════════════════════════════════════════════
void ConvertWorker::run() {
    emit log("启动转换...");
    emit progress(10);

    // ── 定位 cli.py ─────────────────────────
    QString projectRoot = findProjectRoot();
    QString script = projectRoot + "/cli.py";

    if (!QFile::exists(script)) {
        // 回退: 尝试当前工作目录
        script = QDir::currentPath() + "/cli.py";
    }

    if (!QFile::exists(script)) {
        emit failed("找不到 cli.py, 请从项目根目录运行");
        return;
    }

    // ── 构造命令行参数 ──────────────────────
    QStringList args;
    args << script << "convert";

    if (!m_esiPath.isEmpty())     args << "--esi" << m_esiPath;
    if (!m_sdoPath.isEmpty())     args << "--sdo" << m_sdoPath;
    if (!m_templatePath.isEmpty()) args << "--template" << m_templatePath;
    args << "--out" << m_outputDir;

    // 身份覆盖
    if (!m_identity["vendor_id"].toString().isEmpty())
        args << "--vendor-id" << m_identity["vendor_id"].toString();
    if (!m_identity["product_code"].toString().isEmpty())
        args << "--product-code" << m_identity["product_code"].toString();
    if (!m_identity["revision"].toString().isEmpty())
        args << "--revision" << m_identity["revision"].toString();
    if (!m_identity["device_name"].toString().isEmpty())
        args << "--name" << m_identity["device_name"].toString();

    emit log("执行: python3 " + args.join(" "));
    emit progress(20);

    // ── 启动 Python 进程 ────────────────────
    QProcess proc;
    proc.setWorkingDirectory(projectRoot);
    proc.start("python3", args);

    if (!proc.waitForStarted(5000)) {
        emit failed("无法启动 Python 进程");
        return;
    }

    // ── 逐行读取输出, 实时转发 ──────────────
    while (proc.state() == QProcess::Running) {
        proc.waitForFinished(500);
        QByteArray line = proc.readLine();
        while (!line.isEmpty()) {
            QString msg = QString::fromUtf8(line).trimmed();
            if (!msg.isEmpty())
                emit log(msg);
            line = proc.readLine();
        }
    }

    // 读取剩余输出
    QByteArray remaining = proc.readAllStandardOutput();
    for (const QByteArray &l : remaining.split('\n')) {
        QString msg = QString::fromUtf8(l).trimmed();
        if (!msg.isEmpty())
            emit log(msg);
    }

    emit progress(90);

    // ── 检查退出码 ──────────────────────────
    if (proc.exitCode() != 0) {
        QByteArray errOutput = proc.readAllStandardError();
        emit failed(QString::fromUtf8(errOutput).trimmed());
        return;
    }

    // ── 查找输出 ────────────────────────────
    QDir outDir(m_outputDir);
    if (!outDir.exists()) {
        emit failed("输出目录不存在: " + m_outputDir);
        return;
    }

    QStringList subdirs = outDir.entryList(
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);

    emit progress(100);

    if (!subdirs.isEmpty())
        emit finishedOk(m_outputDir + "/" + subdirs.first());
    else
        emit finishedOk(m_outputDir);
}
