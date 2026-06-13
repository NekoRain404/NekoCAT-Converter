/**
 * 页5: PDO/SM 校验 — 实现。
 */
#include "pdopage.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFont>

PdoPage::PdoPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    // SM 校验
    auto *smGroup = new QGroupBox("Sync Manager 校验");
    auto *smLayout = new QVBoxLayout(smGroup);
    m_smText = new QTextEdit;
    m_smText->setReadOnly(true);
    m_smText->setFont(QFont("Monospace", 10));
    m_smText->setMaximumHeight(120);
    smLayout->addWidget(m_smText);
    layout->addWidget(smGroup);

    // PDO 布局
    auto *pdoGroup = new QGroupBox("PDO 布局");
    auto *pdoLayout = new QVBoxLayout(pdoGroup);
    m_pdoText = new QTextEdit;
    m_pdoText->setReadOnly(true);
    m_pdoText->setFont(QFont("Monospace", 9));
    pdoLayout->addWidget(m_pdoText);
    layout->addWidget(pdoGroup);
}

void PdoPage::setSmInfo(const QString &text) {
    m_smText->setPlainText(text);
}

void PdoPage::setPdoLayout(const QString &text) {
    m_pdoText->setPlainText(text);
}
