/**
 * 页6: 生成输出 (dev doc §12.6)
 */
#ifndef GENERATEPAGE_H
#define GENERATEPAGE_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>

class GeneratePage : public QWidget {
    Q_OBJECT
public:
    explicit GeneratePage(QWidget *parent = nullptr);
    void setStatus(const QString &text);

signals:
    void generateAll();

private:
    QListWidget *m_fileList;
    QLabel *m_status;
};
#endif
