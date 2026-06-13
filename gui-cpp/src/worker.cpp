/**
 * ConvertWorker — 通过 QProcess 调用 Python CLI 完成转换。
 *
 * 调用方式：python3 cli.py convert --esi X --sdo Y --out Z
 * 输出目录中的 convert.log 包含详细日志。
 */
#include "worker.h"
#include <QProcess>
#include <QDir>
#include <QCoreApplication>

ConvertWorker::ConvertWorker(
    const QString &esiPath, const QString &sdoPath,
    const QString &templatePath, const QString &outputDir,
    const QVariantMap &identity, const QVariantMap &rules,
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

void ConvertWorker::run() {
    emit log("启动转换...");
    emit progress(10);

    // 定位项目根目录
    QString appDir = QCoreApplication::applicationDirPath();
    QString projectRoot = appDir;
    // 如果在 build/ 目录，回退两级
    if (projectRoot.endsWith("/gui-cpp/build"))
        projectRoot = QFileInfo(projectRoot).path() + "/..";

    QString script = projectRoot + "/cli.py";
    if (!QFile::exists(script)) {
        // 尝试当前工作目录
        script = QDir::currentPath() + "/cli.py";
    }

    if (!QFile::exists(script)) {
        emit failed("找不到 cli.py，请从项目根目录运行");
        return;
    }

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

    QProcess proc;
    proc.setWorkingDirectory(projectRoot);
    proc.start("python3", args);

    if (!proc.waitForStarted(5000)) {
        emit failed("无法启动 Python 进程");
        return;
    }

    // 逐行读取输出，实时转发到日志
    while (proc.state() == QProcess::Running) {
        proc.waitForFinished(500);
        QByteArray line = proc.readLine();
        while (!line.isEmpty()) {
            emit log(QString::fromUtf8(line).trimmed());
            line = proc.readLine();
        }
    }

    // 读取剩余输出
    QByteArray remaining = proc.readAllStandardOutput();
    for (const QByteArray &l : remaining.split('\n')) {
        if (!l.trimmed().isEmpty())
            emit log(QString::fromUtf8(l).trimmed());
    }

    emit progress(90);

    if (proc.exitCode() != 0) {
        QByteArray errOutput = proc.readAllStandardError();
        emit failed(QString::fromUtf8(errOutput).trimmed());
        return;
    }

    // 从输出目录找到最新的输出
    QDir outDir(m_outputDir);
    QStringList subdirs = outDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    if (!subdirs.isEmpty()) {
        emit finishedOk(m_outputDir + "/" + subdirs.first());
    } else {
        emit finishedOk(m_outputDir);
    }
}
