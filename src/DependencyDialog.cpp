#include "DependencyDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

DependencyDialog::DependencyDialog(const QStringList& missing, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Missing Dependencies"));
    setMinimumWidth(500);
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

    listHtml += QStringLiteral("</ul>");

    // ── Install instructions ────────────────────────────────────────────
    listHtml += QStringLiteral(
        "<p style='color:#a0aec0; font-size:13px;'>"
        "Install them with your package manager:</p>"
        "<table style='color:#e4e7ec; font-size:12px; margin-left:8px;'>"
        "<tr><td style='padding:3px 12px 3px 0;'><b>Debian/Ubuntu:</b></td>"
        "    <td><code>sudo apt install yt-dlp ffmpeg</code></td></tr>"
        "<tr><td style='padding:3px 12px 3px 0;'><b>Fedora:</b></td>"
        "    <td><code>sudo dnf install yt-dlp ffmpeg</code></td></tr>"
        "<tr><td style='padding:3px 12px 3px 0;'><b>Arch:</b></td>"
        "    <td><code>sudo pacman -S yt-dlp ffmpeg</code></td></tr>"
        "<tr><td style='padding:3px 12px 3px 0;'><b>openSUSE:</b></td>"
        "    <td><code>sudo zypper install yt-dlp ffmpeg</code></td></tr>"
        "</table>"
        "<p style='color:#a0aec0; font-size:12px; margin-top:12px;'>"
        "After installing, click <b>Retry</b> to check again.</p>");

    m_lblInfo = new QLabel(listHtml);
    m_lblInfo->setWordWrap(true);
    m_lblInfo->setTextFormat(Qt::RichText);
    m_lblInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(m_lblInfo);

    // ── Buttons ─────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();

    m_btnClose = new QPushButton(tr("Close"));
    m_btnClose->setObjectName(QStringLiteral("btnCancel"));
    m_btnClose->setFixedWidth(100);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(m_btnClose);

    m_btnRetry = new QPushButton(tr("Retry"));
    m_btnRetry->setFixedWidth(120);
    connect(m_btnRetry, &QPushButton::clicked, this, &DependencyDialog::onRetryClicked);
    btnRow->addWidget(m_btnRetry);

    root->addLayout(btnRow);
}

void DependencyDialog::onRetryClicked()
{
    emit retryRequested();
    accept();
}
