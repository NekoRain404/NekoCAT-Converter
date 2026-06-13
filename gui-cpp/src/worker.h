/**
 * ConvertWorker — QThread 后台转换工作线程。
 *
 * 通过 QProcess 调用 Python CLI (cli.py convert)，
 * 解析其输出获取结果。不直接耦合 Python 模块。
 */
#ifndef WORKER_H
#define WORKER_H

#include <QThread>
#include <QVariantMap>

class ConvertWorker : public QThread {
    Q_OBJECT

public:
    explicit ConvertWorker(
        const QString &esiPath, const QString &sdoPath,
        const QString &templatePath, const QString &outputDir,
        const QVariantMap &identity, const QVariantMap &rules,
        QObject *parent = nullptr
    );

signals:
    void log(const QString &msg);
    void progress(int value);
    void finishedOk(const QString &outputDir);
    void failed(const QString &error);

protected:
    void run() override;

private:
    QString m_esiPath, m_sdoPath, m_templatePath, m_outputDir;
    QVariantMap m_identity, m_rules;
};

#endif // WORKER_H
