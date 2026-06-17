/**
 * 页2: 身份配置 — 实现。
 */
#include "identitypage.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QRadioButton>

IdentityPage::IdentityPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    // 原始身份
    auto *origGroup = new QGroupBox("原始身份");
    auto *origGrid = new QGridLayout(origGroup);
    const char *keys[] = {"Vendor ID", "Product Code", "Revision", "Device Name"};
    for (int i = 0; i < 4; ++i) {
        origGrid->addWidget(new QLabel(QString(keys[i]) + ":"), i, 0);
        m_origLabels[i] = new QLabel("-");
        m_origLabels[i]->setStyleSheet("color: #888;");
        origGrid->addWidget(m_origLabels[i], i, 1);
    }
    layout->addWidget(origGroup);

    // 新身份
    auto *newGroup = new QGroupBox("新身份 (覆盖)");
    auto *newGrid = new QGridLayout(newGroup);
    auto addRow = [&](int row, const char *label) -> QLineEdit* {
        newGrid->addWidget(new QLabel(label), row, 0);
        auto *edit = new QLineEdit;
        newGrid->addWidget(edit, row, 1);
        return edit;
    };
    m_vendorEdit   = addRow(0, "Vendor ID:");
    m_productEdit  = addRow(1, "Product Code (hex):");
    m_revisionEdit = addRow(2, "Revision (hex):");
    m_nameEdit     = addRow(3, "Device Name:");
    layout->addWidget(newGroup);

    // 模式选择
    auto *modeGroup = new QGroupBox("模式");
    auto *modeLayout = new QHBoxLayout(modeGroup);
    m_modeGroup = new QButtonGroup(this);
    const char *modes[] = {"学习模式", "实验室克隆", "产品模式"};
    for (int i = 0; i < 3; ++i) {
        auto *rb = new QRadioButton(modes[i]);
        m_modeGroup->addButton(rb, i);
        modeLayout->addWidget(rb);
    }
    m_modeGroup->button(0)->setChecked(true);
    layout->addWidget(modeGroup);
    layout->addStretch();
}

void IdentityPage::setOriginalIdentity(const QVariantMap &id) {
    m_origLabels[0]->setText(id["Vendor ID"].toString());
    m_origLabels[1]->setText(id["Product Code"].toString());
    m_origLabels[2]->setText(id["Revision"].toString());
    m_origLabels[3]->setText(id["Device Name"].toString());
}

QVariantMap IdentityPage::getConfig() const {
    QVariantMap m;
    m["vendor_id"]    = m_vendorEdit->text().trimmed();
    m["product_code"] = m_productEdit->text().trimmed();
    m["revision"]     = m_revisionEdit->text().trimmed();
    m["device_name"]  = m_nameEdit->text().trimmed();
    const char *modeNames[] = {"learning", "lab", "product"};
    m["mode"] = modeNames[m_modeGroup->checkedId()];
    return m;
}
