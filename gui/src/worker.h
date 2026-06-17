/**
 * ConvertWorker — QThread 后台转换工作线程。
 *
 * 设计原则:
 *   - 通过 QProcess 调用 Python CLI (cli.py convert)
 *   - 不直接耦合 Python 模块, 保持前后端分离
 *   - 通过 Qt 信号槽与 UI 通信, 不直接操作任何 UI 控件
 *   - 逐行读取 Python 输出, 实时转发到日志
 *
 * 信号:
 *   - log(msg)       → 实时日志
 *   - progress(val)  → 进度更新 (0-100)
 *   - finishedOk(dir) → 转换成功, 输出目录
 *   - failed(err)    → 转换失败, 错误信息
 */
#ifndef WORKER_H
#define WORKER_H

#include <QThread>
#include <QVariantMap>

class ConvertWorker : public QThread {
    Q_OBJECT

public:
    /**
     * 构造函数。
     * @param esiPath     ESI XML 文件路径
     * @param sdoPath     SDO 文件路径
     * @param templatePath SSC 模板 xlsx 路径
     * @param outputDir   输出目录
     * @param identity    身份覆盖 (vendor_id, product_code, ...)
     * @param rules       规则覆盖 (暂未使用)
     * @param parent      父对象
     */
    explicit ConvertWorker(
        const QString &esiPath,
        const QString &sdoPath,
        const QString &templatePath,
        const QString &outputDir,
        const QVariantMap &identity,
        const QVariantMap &rules,
        QObject *parent = nullptr
    );

signals:
    /** 追加日志 */
    void log(const QString &msg);
    /** 更新进度 (0-100) */
    void progress(int value);
    /** 转换成功, 输出目录路径 */
    void finishedOk(const QString &outputDir);
    /** 转换失败, 错误信息 */
    void failed(const QString &error);

protected:
    void run() override;

private:
    /** 定位项目根目录 (从可执行文件位置推断) */
    QString findProjectRoot() const;

    QString m_esiPath;
    QString m_sdoPath;
    QString m_templatePath;
    QString m_outputDir;
    QVariantMap m_identity;
    QVariantMap m_rules;
};

#endif // WORKER_H
