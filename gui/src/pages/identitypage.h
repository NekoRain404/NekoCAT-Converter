/**
 * 页2: 身份配置 (dev doc §12.2)
 */
#ifndef IDENTITYPAGE_H
#define IDENTITYPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QButtonGroup>
#include <QVariantMap>

class IdentityPage : public QWidget {
    Q_OBJECT
public:
    explicit IdentityPage(QWidget *parent = nullptr);
    void setOriginalIdentity(const QVariantMap &identity);
    QVariantMap getConfig() const;

private:
    QLabel *m_origLabels[4];
    QLineEdit *m_vendorEdit, *m_productEdit, *m_revisionEdit, *m_nameEdit;
    QButtonGroup *m_modeGroup;
};

#endif // IDENTITYPAGE_H
