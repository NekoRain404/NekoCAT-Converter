/**
 * 页3: 规则策略配置 (dev doc §12.3)
 */
#ifndef RULEPAGE_H
#define RULEPAGE_H

#include <QWidget>
#include <QButtonGroup>
#include <QCheckBox>
#include <QVariantMap>

class RulePage : public QWidget {
    Q_OBJECT
public:
    explicit RulePage(QWidget *parent = nullptr);
    QVariantMap getConfig() const;

private:
    QButtonGroup *m_octetGroup, *m_alignGroup, *m_commGroup;
    QCheckBox *m_includeFxxx;
};
#endif
