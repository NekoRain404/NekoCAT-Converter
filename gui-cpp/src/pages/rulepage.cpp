/**
 * 页3: 规则策略 — 实现。
 */
#include "rulepage.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QRadioButton>

RulePage::RulePage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    // OCTET_STRING 策略
    auto *octetGroup = new QGroupBox("OCTET_STRING 处理");
    auto *octetLayout = new QVBoxLayout(octetGroup);
    m_octetGroup = new QButtonGroup(this);
    const char *octetOpts[] = {"转换为 STRING + CoE callback", "跳过", "保留 + 手动 callback"};
    for (int i = 0; i < 3; ++i) {
        auto *rb = new QRadioButton(octetOpts[i]);
        m_octetGroup->addButton(rb, i);
        octetLayout->addWidget(rb);
    }
    m_octetGroup->button(0)->setChecked(true);
    layout->addWidget(octetGroup);

    // 未对齐对象策略
    auto *alignGroup = new QGroupBox("未对齐对象 (word offset)");
    auto *alignLayout = new QVBoxLayout(alignGroup);
    m_alignGroup = new QButtonGroup(this);
    const char *alignOpts[] = {"对象级 CoE callback", "插入 padding", "跳过对象"};
    for (int i = 0; i < 3; ++i) {
        auto *rb = new QRadioButton(alignOpts[i]);
        m_alignGroup->addButton(rb, i);
        alignLayout->addWidget(rb);
    }
    m_alignGroup->button(0)->setChecked(true);
    layout->addWidget(alignGroup);

    // 通信对象策略
    auto *commGroup = new QGroupBox("通信对象 (0x1000-0x1FFF)");
    auto *commLayout = new QVBoxLayout(commGroup);
    m_commGroup = new QButtonGroup(this);
    const char *commOpts[] = {"由 SSC 自动生成", "强制导入", "跳过"};
    for (int i = 0; i < 3; ++i) {
        auto *rb = new QRadioButton(commOpts[i]);
        m_commGroup->addButton(rb, i);
        commLayout->addWidget(rb);
    }
    m_commGroup->button(0)->setChecked(true);
    layout->addWidget(commGroup);

    // Fxxx 对象
    auto *fxxxGroup = new QGroupBox("高级/厂商对象 (0xF000+)");
    auto *fxxxLayout = new QVBoxLayout(fxxxGroup);
    m_includeFxxx = new QCheckBox("包含 Fxxx 对象");
    m_includeFxxx->setChecked(true);
    fxxxLayout->addWidget(m_includeFxxx);
    layout->addWidget(fxxxGroup);

    layout->addStretch();
}

QVariantMap RulePage::getConfig() const {
    QVariantMap m;
    const char *octetMap[] = {"string_with_callback", "skip", "manual_callback"};
    const char *alignMap[] = {"coe_callback", "insert_padding", "skip"};
    const char *commMap[]  = {"ssc_auto", "force_import", "skip"};
    m["octet_string_strategy"]          = octetMap[m_octetGroup->checkedId()];
    m["unaligned_strategy"]             = alignMap[m_alignGroup->checkedId()];
    m["communication_object_strategy"]  = commMap[m_commGroup->checkedId()];
    m["include_fxxx"]                   = m_includeFxxx->isChecked();
    return m;
}
