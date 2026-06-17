/**
 * 页1: 文件导入 (dev doc §12.1)
 *
 * 控件：ESI 路径、SDO 路径、模板路径、输出目录
 * 按钮：浏览、解析
 * 解析后显示设备身份摘要
 */
#ifndef FILESELECTPAGE_H
#define FILESELECTPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVariantMap>

class FileSelectPage : public QWidget {
    Q_OBJECT

public:
    explicit FileSelectPage(QWidget *parent = nullptr);

    /** 显示解析结果摘要 */
    void showParseResult(const QVariantMap &info);
    /** 获取当前文件路径 */
    QVariantMap getPaths() const;

signals:
    /** 用户点击 "解析" 按钮 */
    void parseRequested(const QVariantMap &paths);

private slots:
    void onParseClicked();
    void browseFile(QLineEdit *edit, const QString &filter);
    void browseDir(QLineEdit *edit);

private:
    QLineEdit *m_esiEdit;
    QLineEdit *m_sdoEdit;
    QLineEdit *m_tplEdit;
    QLineEdit *m_outEdit;
    QTextEdit *m_resultText;
};

#endif // FILESELECTPAGE_H
