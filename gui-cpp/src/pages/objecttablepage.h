/**
 * 页4: 对象字典表 (dev doc §12.4)
 * 可过滤表格，颜色编码：绿=自动，黄=callback，红=错误，灰=跳过
 */
#ifndef OBJECTTABLEPAGE_H
#define OBJECTTABLEPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QVector>
#include <QVariantMap>

class ObjectTablePage : public QWidget {
    Q_OBJECT
public:
    explicit ObjectTablePage(QWidget *parent = nullptr);
    void setObjects(const QList<QVariantMap> &objects);

private slots:
    void applyFilter();

private:
    QTableWidget *m_table;
    QComboBox *m_filterCombo;
    QLabel *m_countLabel;
    QList<QVariantMap> m_objects;
};
#endif
