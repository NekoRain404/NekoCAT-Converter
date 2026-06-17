/**
 * 页1: 文件导入 — 实现。
 */
#include "fileselectpage.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>

FileSelectPage::FileSelectPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    // ── 文件选择组 ──────────────────────────
    auto *fileGroup = new QGroupBox("输入文件");
    auto *grid = new QGridLayout(fileGroup);

    // ESI 行
    grid->addWidget(new QLabel("ESI.XML:"), 0, 0);
    m_esiEdit = new QLineEdit;
    grid->addWidget(m_esiEdit, 0, 1);
    auto *esiBtn = new QPushButton("浏览...");
    esiBtn->setMaximumWidth(80);
    connect(esiBtn, &QPushButton::clicked, this, [this]() {
        browseFile(m_esiEdit, "XML files (*.xml);;All (*)");
    });
    grid->addWidget(esiBtn, 0, 2);

    // SDO 行
    grid->addWidget(new QLabel("SDO 文件:"), 1, 0);
    m_sdoEdit = new QLineEdit;
    grid->addWidget(m_sdoEdit, 1, 1);
    auto *sdoBtn = new QPushButton("浏览...");
    sdoBtn->setMaximumWidth(80);
    connect(sdoBtn, &QPushButton::clicked, this, [this]() {
        browseFile(m_sdoEdit, "XML/TXT files (*.xml *.txt);;All (*)");
    });
    grid->addWidget(sdoBtn, 1, 2);

    // 模板行
    grid->addWidget(new QLabel("模板 xlsx:"), 2, 0);
    m_tplEdit = new QLineEdit;
    grid->addWidget(m_tplEdit, 2, 1);
    auto *tplBtn = new QPushButton("浏览...");
    tplBtn->setMaximumWidth(80);
    connect(tplBtn, &QPushButton::clicked, this, [this]() {
        browseFile(m_tplEdit, "Excel files (*.xlsx);;All (*)");
    });
    grid->addWidget(tplBtn, 2, 2);

    // 输出目录行
    grid->addWidget(new QLabel("输出目录:"), 3, 0);
    m_outEdit = new QLineEdit("out");
    grid->addWidget(m_outEdit, 3, 1);
    auto *outBtn = new QPushButton("浏览...");
    outBtn->setMaximumWidth(80);
    connect(outBtn, &QPushButton::clicked, this, [this]() {
        browseDir(m_outEdit);
    });
    grid->addWidget(outBtn, 3, 2);

    layout->addWidget(fileGroup);

    // ── 解析按钮 ────────────────────────────
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto *parseBtn = new QPushButton("解析");
    parseBtn->setMinimumWidth(120);
    connect(parseBtn, &QPushButton::clicked, this, &FileSelectPage::onParseClicked);
    btnRow->addWidget(parseBtn);
    layout->addLayout(btnRow);

    // ── 解析结果 ────────────────────────────
    auto *resultGroup = new QGroupBox("解析结果");
    auto *resultLayout = new QVBoxLayout(resultGroup);
    m_resultText = new QTextEdit;
    m_resultText->setReadOnly(true);
    m_resultText->setMaximumHeight(180);
    resultLayout->addWidget(m_resultText);
    layout->addWidget(resultGroup);

    layout->addStretch();
}

void FileSelectPage::onParseClicked() {
    QVariantMap paths;
    paths["esi_path"]      = m_esiEdit->text().trimmed().isEmpty() ? QVariant() : m_esiEdit->text().trimmed();
    paths["sdo_path"]      = m_sdoEdit->text().trimmed().isEmpty() ? QVariant() : m_sdoEdit->text().trimmed();
    paths["template_path"] = m_tplEdit->text().trimmed().isEmpty() ? QVariant() : m_tplEdit->text().trimmed();
    paths["output_dir"]    = m_outEdit->text().trimmed().isEmpty() ? "out" : m_outEdit->text().trimmed();

    if (paths["esi_path"].toString().isEmpty() && paths["sdo_path"].toString().isEmpty()) {
        m_resultText->setPlainText("错误: 请至少提供一个 ESI 或 SDO 文件。");
        return;
    }
    emit parseRequested(paths);
}

void FileSelectPage::showParseResult(const QVariantMap &info) {
    QStringList lines;
    for (auto it = info.begin(); it != info.end(); ++it)
        lines << QString("%1: %2").arg(it.key(), it.value().toString());
    m_resultText->setPlainText(lines.join("\n"));
}

QVariantMap FileSelectPage::getPaths() const {
    QVariantMap p;
    p["esi_path"]      = m_esiEdit->text().trimmed().isEmpty() ? QVariant() : m_esiEdit->text().trimmed();
    p["sdo_path"]      = m_sdoEdit->text().trimmed().isEmpty() ? QVariant() : m_sdoEdit->text().trimmed();
    p["template_path"] = m_tplEdit->text().trimmed().isEmpty() ? QVariant() : m_tplEdit->text().trimmed();
    p["output_dir"]    = m_outEdit->text().trimmed().isEmpty() ? "out" : m_outEdit->text().trimmed();
    return p;
}

void FileSelectPage::browseFile(QLineEdit *edit, const QString &filter) {
    QString path = QFileDialog::getOpenFileName(this, "选择文件", "", filter);
    if (!path.isEmpty()) edit->setText(path);
}

void FileSelectPage::browseDir(QLineEdit *edit) {
    QString path = QFileDialog::getExistingDirectory(this, "选择输出目录");
    if (!path.isEmpty()) edit->setText(path);
}
