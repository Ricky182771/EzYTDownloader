#pragma once

#include <QDialog>
#include <QStringList>

class QLabel;
class QPushButton;

/// Modal dialog shown when required dependencies are missing.
/// Offers the user an "Install automatically" button that launches a
/// terminal running install_deps.sh.
class DependencyDialog : public QDialog {
    Q_OBJECT

public:
    explicit DependencyDialog(const QStringList& missing, QWidget* parent = nullptr);

signals:
    void installRequested();

private slots:
    void onInstallClicked();

private:
    void setupUi(const QStringList& missing);

    QLabel*       m_lblInfo    = nullptr;
    QPushButton*  m_btnInstall = nullptr;
    QPushButton*  m_btnClose   = nullptr;
};
