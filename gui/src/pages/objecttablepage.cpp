/**
 * 页4: 对象字典表 — 实现。
 */
#include "objecttablepage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QBrush>
#include <QColor>

static const char *HEADERS[] = {
    "Index", "Name", "SubCount", "Category", "PDO Map", "SM Obj", "Risk", "CoeRead", "CoeWrite"
};
static const int NCOLS = 9;

ObjectTablePage::ObjectTablePage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    // 过滤栏
    auto *filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("过滤:"));
    m_filterCombo = new QComboBox;
    m_filterCombo->addItems({"全部", "仅错误", "仅 callback", "仅 PDO", "仅 Fxxx", "已跳过"});
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ObjectTablePage::applyFilter);
    filterRow->addWidget(m_filterCombo);
    filterRow->addStretch();
    m_countLabel = new QLabel("0 objects");
    filterRow->addWidget(m_countLabel);
    layout->addLayout(filterRow);

    // 表格
    m_table = new QTableWidget;
    m_table->setColumnCount(NCOLS);
    QStringList headers;
    for (int i = 0; i < NCOLS; ++i) headers << HEADERS[i];
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setEditTriggers(QTableWidget::NoEditTriggers);
    m_table->setSelectionBehavior(QTableWidget::SelectRows);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_table);
}

void ObjectTablePage::setObjects(const QList<QVariantMap> &objects) {
    m_objects = objects;
    applyFilter();
}

void ObjectTablePage::applyFilter() {
    int filter = m_filterCombo->currentIndex();
    QList<QVariantMap> filtered = m_objects;

    if (filter == 1) { // 仅错误
        filtered.erase(std::remove_if(filtered.begin(), filtered.end(),
            [](const QVariantMap &o) { return !o["risk"].toBool(); }), filtered.end());
    } else if (filter == 2) { // 仅 callback
        filtered.erase(std::remove_if(filtered.begin(), filtered.end(),
            [](const QVariantMap &o) { return o["CoeRead"].toString().isEmpty() && o["CoeWrite"].toString().isEmpty(); }), filtered.end());
    } else if (filter == 3) { // 仅 PDO
        filtered.erase(std::remove_if(filtered.begin(), filtered.end(),
            [](const QVariantMap &o) { return !o["pdo_map"].toBool(); }), filtered.end());
    } else if (filter == 4) { // 仅 Fxxx
        filtered.erase(std::remove_if(filtered.begin(), filtered.end(),
            [](const QVariantMap &o) { return o["index_int"].toInt() < 0xF000; }), filtered.end());
    } else if (filter == 5) { // 已跳过
        filtered.erase(std::remove_if(filtered.begin(), filtered.end(),
            [](const QVariantMap &o) { return !o["skipped"].toBool(); }), filtered.end());
    }

    m_table->setRowCount(filtered.size());
    for (int row = 0; row < filtered.size(); ++row) {
        const QVariantMap &obj = filtered[row];
        // 确定行颜色
        QColor color(255, 255, 255); // 白色
        if (obj["skipped"].toBool()) color = QColor(220, 220, 220);
        else if (obj["risk"].toBool()) color = QColor(255, 200, 200);
        else if (!obj["CoeRead"].toString().isEmpty() || !obj["CoeWrite"].toString().isEmpty())
            color = QColor(255, 255, 180);

        for (int col = 0; col < NCOLS; ++col) {
            auto *item = new QTableWidgetItem(obj[HEADERS[col]].toString());
            item->setBackground(QBrush(color));
            m_table->setItem(row, col, item);
        }
    }
    m_countLabel->setText(QString("%1 objects").arg(filtered.size()));
}
