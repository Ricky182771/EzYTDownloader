#pragma once

#include <QDialog>
#include <QStringList>

class QLabel;
class QPushButton;

/// Modal dialog shown when required dependencies are missing.
/// Displays installation instructions for common distributions.
class DependencyDialog : public QDialog {
    Q_OBJECT

public:
    explicit DependencyDialog(const QStringList& missing, QWidget* parent = nullptr);

signals:
    void retryRequested();

private slots:
    void onRetryClicked();

private:
    void setupUi(const QStringList& missing);

    QLabel*       m_lblInfo    = nullptr;
    QPushButton*  m_btnRetry   = nullptr;
    QPushButton*  m_btnClose   = nullptr;
};
