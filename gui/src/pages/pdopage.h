/**
 * 页5: PDO/SM 校验 (dev doc §12.5)
 */
#ifndef PDOPAGE_H
#define PDOPAGE_H

#include <QWidget>
#include <QTextEdit>

class PdoPage : public QWidget {
    Q_OBJECT
public:
    explicit PdoPage(QWidget *parent = nullptr);
    void setSmInfo(const QString &text);
    void setPdoLayout(const QString &text);

private:
    QTextEdit *m_smText;
    QTextEdit *m_pdoText;
};
#endif
