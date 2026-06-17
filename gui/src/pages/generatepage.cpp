/**
 * 页6: 生成输出 — 实现。
 */
#include "generatepage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>

GeneratePage::GeneratePage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    // 生成按钮
    auto *btnGroup = new QGroupBox("生成");
    auto *btnLayout = new QHBoxLayout(btnGroup);
    auto addBtn = [&](const char *label) -> QPushButton* {
        auto *btn = new QPushButton(label);
        btn->setMinimumWidth(140);
        btnLayout->addWidget(btn);
        return btn;
    };
    auto *allBtn = addBtn("全部生成");
    addBtn("仅 SSC xlsx");
    addBtn("仅 TwinCAT ESI");
    addBtn("仅报告");
    connect(allBtn, &QPushButton::clicked, this, &GeneratePage::generateAll);
    layout->addWidget(btnGroup);

    // 输出文件列表
    auto *outputGroup = new QGroupBox("输出文件");
    auto *outputLayout = new QVBoxLayout(outputGroup);
    m_fileList = new QListWidget;
    outputLayout->addWidget(m_fileList);
    layout->addWidget(outputGroup);

    // 状态
    m_status = new QLabel("就绪");
    layout->addWidget(m_status);
}

void GeneratePage::setStatus(const QString &text) {
    m_status->setText(text);
}
