#include "DependencyDialog.h"
#include "Utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProcess>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

DependencyDialog::DependencyDialog(const QStringList& missing, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Missing Dependencies"));
    setMinimumWidth(440);
    setModal(true);
    setupUi(missing);
}

void DependencyDialog::setupUi(const QStringList& missing)
{
    auto* root = new QVBoxLayout(this);
    root->setSpacing(16);
    root->setContentsMargins(28, 24, 28, 24);

    // ── Icon + title ────────────────────────────────────────────────────
    auto* lblTitle = new QLabel(tr("⚠️  Some required tools are missing"));
    lblTitle->setObjectName(QStringLiteral("lblTitle"));
    lblTitle->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 700; color: #f0c040;"));
    root->addWidget(lblTitle);

    // ── Missing list ────────────────────────────────────────────────────
    QString listHtml = QStringLiteral(
        "<p style='color:#a0aec0; font-size:13px;'>"
        "The following tools were not found on your system:</p>"
        "<ul style='color:#e4e7ec; font-size:13px;'>");

    for (const auto& dep : missing)
        listHtml += QStringLiteral("<li><b>%1</b></li>").arg(dep);

    listHtml += QStringLiteral("</ul>"
        "<p style='color:#a0aec0; font-size:12px;'>"
        "EzYTDownloader needs these to function. You can install them "
        "automatically or do it manually.</p>");

    m_lblInfo = new QLabel(listHtml);
    m_lblInfo->setWordWrap(true);
    m_lblInfo->setTextFormat(Qt::RichText);
    root->addWidget(m_lblInfo);

    // ── Buttons ─────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();

    m_btnClose = new QPushButton(tr("Close"));
    m_btnClose->setObjectName(QStringLiteral("btnCancel"));
    m_btnClose->setFixedWidth(100);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(m_btnClose);

    m_btnInstall = new QPushButton(tr("Install Automatically"));
    m_btnInstall->setFixedWidth(180);
    connect(m_btnInstall, &QPushButton::clicked, this, &DependencyDialog::onInstallClicked);
    btnRow->addWidget(m_btnInstall);

    root->addLayout(btnRow);
}

void DependencyDialog::onInstallClicked()
{
    const QString script = Utils::findBashScript();
    if (script.isEmpty()) {
        QMessageBox::critical(this, tr("Error"),
            tr("Could not locate install_deps.sh.\n"
               "Please reinstall the application."));
        return;
    }

    const QString terminal = Utils::detectTerminal();
    if (terminal.isEmpty()) {
        QMessageBox::critical(this, tr("Error"),
            tr("No terminal emulator found.\n"
               "Please install konsole, gnome-terminal, or xterm."));
        return;
    }

    qDebug() << "[DepDialog] Launching" << terminal << "with" << script;

    // Build the command to run inside the terminal
    // Format: terminal -e bash /path/to/install_deps.sh
    QStringList args;
    const QString termName = QFileInfo(terminal).fileName();

    if (termName == QStringLiteral("konsole")) {
        args << QStringLiteral("-e") << QStringLiteral("bash") << script;
    } else if (termName == QStringLiteral("gnome-terminal")) {
        args << QStringLiteral("--") << QStringLiteral("bash") << script;
    } else if (termName == QStringLiteral("xfce4-terminal")) {
        args << QStringLiteral("-e") << QStringLiteral("bash %1").arg(script);
    } else if (termName == QStringLiteral("alacritty")) {
        args << QStringLiteral("-e") << QStringLiteral("bash") << script;
    } else if (termName == QStringLiteral("kitty")) {
        args << QStringLiteral("bash") << script;
    } else {
        // Generic fallback: most terminals support -e
        args << QStringLiteral("-e") << QStringLiteral("bash") << script;
    }

    const bool ok = QProcess::startDetached(terminal, args);
    if (!ok) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to launch terminal:\n%1").arg(terminal));
        return;
    }

    emit installRequested();

    QMessageBox::information(this, tr("Installing"),
        tr("A terminal window has been opened to install dependencies.\n\n"
           "After installation completes, please restart EzYTDownloader."));

    accept();
}
