#include "MainWindow.h"
#include "DownloadManager.h"
#include "SettingsManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFrame>
#include <QSpacerItem>
#include <QGraphicsDropShadowEffect>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_manager(new DownloadManager(this))
    , m_netManager(new QNetworkAccessManager(this))
{
    setupUi();
    applyStylesheet();
    loadSettings();

    // ── Connect DownloadManager ─────────────────────────────────────────
    connect(m_manager, &DownloadManager::metadataReady,
            this,      &MainWindow::onMetadataReady);
    connect(m_manager, &DownloadManager::downloadProgress,
            this,      &MainWindow::onDownloadProgress);
    connect(m_manager, &DownloadManager::conversionProgress,
            this,      &MainWindow::onConversionProgress);
    connect(m_manager, &DownloadManager::finished,
            this,      &MainWindow::onFinished);
    connect(m_manager, &DownloadManager::errorOccurred,
            this,      &MainWindow::onError);
    connect(m_manager, &DownloadManager::statusMessage,
            this,      &MainWindow::onStatus);

    setUiState(QStringLiteral("idle"));
}

MainWindow::~MainWindow()
{
    saveSettings();
}

// ─────────────────────────────────────────────────────────────────────────────
// UI construction
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Easy YouTube Video Downloader"));
    setMinimumSize(640, 520);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setSpacing(12);
    root->setContentsMargins(24, 20, 24, 20);

    // ─── Header ─────────────────────────────────────────────────────────
    m_lblTitle = new QLabel(QStringLiteral("Easy YouTube Video Downloader"));
    m_lblTitle->setObjectName(QStringLiteral("lblTitle"));

    m_lblSubtitle = new QLabel(QStringLiteral("Download YouTube videos & audio with ease"));
    m_lblSubtitle->setObjectName(QStringLiteral("lblSubtitle"));

    root->addWidget(m_lblTitle);
    root->addWidget(m_lblSubtitle);

    // Separator
    auto* sep1 = new QFrame;
    sep1->setObjectName(QStringLiteral("separator"));
    sep1->setFrameShape(QFrame::HLine);
    root->addWidget(sep1);

    // ─── URL Bar ────────────────────────────────────────────────────────
    auto* urlRow = new QHBoxLayout;
    urlRow->setSpacing(8);

    m_editUrl = new QLineEdit;
    m_editUrl->setPlaceholderText(QStringLiteral("Paste YouTube URL here…"));
    m_editUrl->setClearButtonEnabled(true);

    m_btnPaste = new QPushButton(QStringLiteral("📋"));
    m_btnPaste->setToolTip(tr("Paste from clipboard"));
    m_btnPaste->setFixedSize(38, 38);
    m_btnPaste->setObjectName(QStringLiteral("btnBrowse")); // reuse subtle style
    connect(m_btnPaste, &QPushButton::clicked, this, &MainWindow::onPasteClicked);

    m_btnFetch = new QPushButton(QStringLiteral("Fetch"));
    m_btnFetch->setFixedWidth(90);
    connect(m_btnFetch, &QPushButton::clicked, this, &MainWindow::onFetchClicked);

    urlRow->addWidget(m_editUrl, 1);
    urlRow->addWidget(m_btnPaste);
    urlRow->addWidget(m_btnFetch);
    root->addLayout(urlRow);

    // ─── Video Info Card ────────────────────────────────────────────────
    m_grpInfo = new QGroupBox(QStringLiteral("VIDEO INFO"));
    auto* infoRow = new QHBoxLayout(m_grpInfo);
    infoRow->setSpacing(12);

    // Thumbnail (left side)
    m_lblThumbnail = new QLabel;
    m_lblThumbnail->setObjectName(QStringLiteral("lblThumbnail"));
    m_lblThumbnail->setFixedSize(160, 90);
    m_lblThumbnail->setAlignment(Qt::AlignCenter);
    m_lblThumbnail->setStyleSheet(
        QStringLiteral("background: rgba(255,255,255,0.05); border-radius: 6px;"));
    m_lblThumbnail->setText(QStringLiteral("🎬"));
    m_lblThumbnail->setScaledContents(false);

    // Text info (right side)
    auto* infoText = new QVBoxLayout;
    infoText->setSpacing(4);

    m_lblVideoTitle = new QLabel(QStringLiteral("—"));
    m_lblVideoTitle->setObjectName(QStringLiteral("lblVideoTitle"));
    m_lblVideoTitle->setWordWrap(true);

    m_lblDuration = new QLabel(QStringLiteral("Duration: —"));
    m_lblDuration->setObjectName(QStringLiteral("lblDuration"));

    infoText->addWidget(m_lblVideoTitle);
    infoText->addWidget(m_lblDuration);
    infoText->addStretch();

    infoRow->addWidget(m_lblThumbnail);
    infoRow->addLayout(infoText, 1);
    m_grpInfo->setVisible(false);
    root->addWidget(m_grpInfo);

    // ─── Options Card ───────────────────────────────────────────────────
    m_grpOptions = new QGroupBox(QStringLiteral("OPTIONS"));
    auto* optGrid = new QGridLayout(m_grpOptions);
    optGrid->setSpacing(10);
    optGrid->setContentsMargins(14, 20, 14, 14);

    // Format
    m_lblFormat = new QLabel(QStringLiteral("Format"));
    m_cmbFormat = new QComboBox;
    m_cmbFormat->addItems({QStringLiteral("MP4 (Video)"),
                           QStringLiteral("MKV (Video)"),
                           QStringLiteral("MP3 (Audio)")});
    connect(m_cmbFormat, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFormatChanged);

    optGrid->addWidget(m_lblFormat,   0, 0);
    optGrid->addWidget(m_cmbFormat,   0, 1);

    // Resolution (video)
    m_lblResolution = new QLabel(QStringLiteral("Resolution"));
    m_cmbResolution = new QComboBox;
    optGrid->addWidget(m_lblResolution, 1, 0);
    optGrid->addWidget(m_cmbResolution, 1, 1);

    // Bitrate (audio)
    m_lblBitrate = new QLabel(QStringLiteral("Bitrate"));
    m_cmbBitrate = new QComboBox;
    m_cmbBitrate->addItems({QStringLiteral("320k"),
                            QStringLiteral("256k"),
                            QStringLiteral("192k"),
                            QStringLiteral("128k")});
    optGrid->addWidget(m_lblBitrate, 2, 0);
    optGrid->addWidget(m_cmbBitrate, 2, 1);

    optGrid->setColumnStretch(1, 1);
    root->addWidget(m_grpOptions);

    // ─── Output directory ───────────────────────────────────────────────
    auto* outRow = new QHBoxLayout;
    outRow->setSpacing(8);

    auto* lblOut = new QLabel(QStringLiteral("Save to"));
    m_editOutputDir = new QLineEdit;
    m_editOutputDir->setReadOnly(true);

    m_btnBrowse = new QPushButton(QStringLiteral("Browse…"));
    m_btnBrowse->setObjectName(QStringLiteral("btnBrowse"));
    m_btnBrowse->setFixedWidth(90);
    connect(m_btnBrowse, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);

    outRow->addWidget(lblOut);
    outRow->addWidget(m_editOutputDir, 1);
    outRow->addWidget(m_btnBrowse);
    root->addLayout(outRow);

    // ─── Progress ───────────────────────────────────────────────────────
    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat(QStringLiteral("%p%"));
    root->addWidget(m_progressBar);

    m_lblStatus = new QLabel(QStringLiteral("Ready"));
    m_lblStatus->setObjectName(QStringLiteral("lblStatus"));
    root->addWidget(m_lblStatus);

    // ─── Action Buttons ─────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->addStretch();

    m_btnCancel = new QPushButton(QStringLiteral("Cancel"));
    m_btnCancel->setObjectName(QStringLiteral("btnCancel"));
    m_btnCancel->setFixedWidth(100);
    connect(m_btnCancel, &QPushButton::clicked, this, &MainWindow::onCancelClicked);
    btnRow->addWidget(m_btnCancel);

    m_btnDownload = new QPushButton(QStringLiteral("⬇  Download"));
    m_btnDownload->setFixedWidth(150);
    connect(m_btnDownload, &QPushButton::clicked, this, &MainWindow::onDownloadClicked);
    btnRow->addWidget(m_btnDownload);

    root->addLayout(btnRow);

    // Initial format state
    onFormatChanged(0);
}

void MainWindow::applyStylesheet()
{
    QFile qss(QStringLiteral(":/style/dark_theme.qss"));
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(qss.readAll()));
        qss.close();
    } else {
        qWarning() << "[MainWindow] Could not load dark_theme.qss";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Settings save/load
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::loadSettings()
{
    auto& s = SettingsManager::instance();

    m_editOutputDir->setText(s.lastDirectory());

    const QString fmt = s.lastFormat();
    if (fmt == QStringLiteral("mkv"))      m_cmbFormat->setCurrentIndex(1);
    else if (fmt == QStringLiteral("mp3")) m_cmbFormat->setCurrentIndex(2);
    else                                    m_cmbFormat->setCurrentIndex(0);

    const int brIdx = m_cmbBitrate->findText(s.lastBitrate());
    if (brIdx >= 0) m_cmbBitrate->setCurrentIndex(brIdx);

    const QSize sz = s.windowSize();
    resize(sz);

    const QPoint pos = s.windowPos();
    if (pos.x() >= 0 && pos.y() >= 0)
        move(pos);
}

void MainWindow::saveSettings()
{
    auto& s = SettingsManager::instance();
    s.setLastDirectory(m_editOutputDir->text());

    static const QStringList fmtKeys = {
        QStringLiteral("mp4"),
        QStringLiteral("mkv"),
        QStringLiteral("mp3"),
    };
    s.setLastFormat(fmtKeys.value(m_cmbFormat->currentIndex(), QStringLiteral("mp4")));

    if (m_cmbResolution->count() > 0)
        s.setLastResolution(m_cmbResolution->currentText());

    s.setLastBitrate(m_cmbBitrate->currentText());
    s.setWindowSize(size());
    s.setWindowPos(pos());
    s.save();
}

// ─────────────────────────────────────────────────────────────────────────────
// UI state machine
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setUiState(const QString& state)
{
    const bool idle        = (state == QStringLiteral("idle"));
    const bool fetching    = (state == QStringLiteral("fetching"));
    const bool downloading = (state == QStringLiteral("downloading"));
    const bool converting  = (state == QStringLiteral("converting"));
    const bool busy        = fetching || downloading || converting;

    m_editUrl->setEnabled(!busy);
    m_btnPaste->setEnabled(!busy);
    m_btnFetch->setEnabled(!busy);
    m_cmbFormat->setEnabled(!busy);
    m_cmbResolution->setEnabled(!busy && m_cmbResolution->count() > 0);
    m_cmbBitrate->setEnabled(!busy);
    m_btnBrowse->setEnabled(!busy);
    m_btnDownload->setEnabled(idle && m_cmbResolution->count() > 0);
    m_btnCancel->setEnabled(busy);

    if (idle) {
        m_progressBar->setValue(0);
    }

    if (converting) {
        m_progressBar->setFormat(QStringLiteral("Converting: %p%"));
    } else {
        m_progressBar->setFormat(QStringLiteral("%p%"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots — user actions
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onPasteClicked()
{
    const QString clip = QApplication::clipboard()->text().trimmed();
    if (!clip.isEmpty())
        m_editUrl->setText(clip);
}

void MainWindow::onFetchClicked()
{
    const QString url = m_editUrl->text().trimmed();
    if (url.isEmpty()) {
        m_lblStatus->setText(tr("Please enter a YouTube URL."));
        m_lblStatus->setObjectName(QStringLiteral("lblError"));
        applyStylesheet(); // re-apply to pick up object name change
        return;
    }

    m_currentUrl = url;
    m_streams.clear();
    m_cmbResolution->clear();
    m_grpInfo->setVisible(false);
    m_lblStatus->setObjectName(QStringLiteral("lblStatus"));
    applyStylesheet();

    setUiState(QStringLiteral("fetching"));
    m_manager->fetchMetadata(url);
}

void MainWindow::onDownloadClicked()
{
    if (m_streams.isEmpty()) {
        m_lblStatus->setText(tr("No streams available. Fetch a video first."));
        return;
    }

    // Retrieve the real index into m_streams stored as combo userData
    const int comboIdx = m_cmbResolution->currentIndex();
    if (comboIdx < 0)
        return;

    const QVariant data = m_cmbResolution->currentData();
    if (!data.isValid())
        return;

    const int realIdx = data.toInt();
    if (realIdx < 0 || realIdx >= m_streams.size()) {
        m_lblStatus->setText(tr("Invalid stream selection."));
        return;
    }

    const QString outDir = m_editOutputDir->text();
    if (outDir.isEmpty()) {
        QMessageBox::warning(this, tr("No Output Directory"),
            tr("Please select an output directory first."));
        return;
    }

    static const QStringList fmtKeys = {
        QStringLiteral("mp4"),
        QStringLiteral("mkv"),
        QStringLiteral("mp3"),
    };
    const QString format  = fmtKeys.value(m_cmbFormat->currentIndex(), QStringLiteral("mp4"));
    const QString bitrate = m_cmbBitrate->currentText();

    qDebug() << "[MainWindow] Download: format=" << format
             << "stream=" << m_streams[realIdx].formatId
             << "resolution=" << m_streams[realIdx].resolution;

    setUiState(QStringLiteral("downloading"));
    m_manager->startDownload(m_currentUrl, m_streams[realIdx], outDir, format, bitrate);
}

void MainWindow::onCancelClicked()
{
    m_manager->cancel();
    setUiState(QStringLiteral("idle"));
}

void MainWindow::onBrowseClicked()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Output Directory"), m_editOutputDir->text());
    if (!dir.isEmpty())
        m_editOutputDir->setText(dir);
}

void MainWindow::onFormatChanged(int index)
{
    const bool isAudio = (index == 2); // MP3

    m_lblResolution->setVisible(!isAudio);
    m_cmbResolution->setVisible(!isAudio);
    m_lblBitrate->setVisible(isAudio);
    m_cmbBitrate->setVisible(isAudio);

    // Re-populate combo, storing the REAL index into m_streams as userData
    if (!m_streams.isEmpty()) {
        m_cmbResolution->clear();
        for (int i = 0; i < m_streams.size(); ++i) {
            const auto& s = m_streams[i];
            if (isAudio && s.isAudioOnly) {
                m_cmbResolution->addItem(s.label(), QVariant(i));
            } else if (!isAudio && !s.isAudioOnly) {
                m_cmbResolution->addItem(s.label(), QVariant(i));
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots — DownloadManager signals
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onMetadataReady(const QList<StreamInfo>& streams,
                                  const QString& title,
                                  double duration,
                                  const QString& thumbnail)
{
    m_streams         = streams;
    m_currentDuration = duration;

    // Load thumbnail preview
    loadThumbnail(thumbnail);

    // Show info card
    m_lblVideoTitle->setText(title);
    const int mins = static_cast<int>(duration) / 60;
    const int secs = static_cast<int>(duration) % 60;
    m_lblDuration->setText(tr("Duration: %1:%2")
        .arg(mins, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0')));
    m_grpInfo->setVisible(true);

    // Populate resolution combo based on current format
    onFormatChanged(m_cmbFormat->currentIndex());

    // Try to restore last used resolution
    const QString lastRes = SettingsManager::instance().lastResolution();
    for (int i = 0; i < m_cmbResolution->count(); ++i) {
        if (m_cmbResolution->itemText(i).contains(lastRes)) {
            m_cmbResolution->setCurrentIndex(i);
            break;
        }
    }

    setUiState(QStringLiteral("idle"));
}

void MainWindow::onDownloadProgress(int percent, const QString& speed, const QString& eta)
{
    m_progressBar->setValue(percent);

    QString statusText = tr("Downloading… %1%").arg(percent);
    if (!speed.isEmpty()) statusText += QStringLiteral("  •  ") + speed;
    if (!eta.isEmpty())   statusText += QStringLiteral("  •  ETA: ") + eta;

    m_lblStatus->setText(statusText);
}

void MainWindow::onConversionProgress(int percent)
{
    setUiState(QStringLiteral("converting"));
    m_progressBar->setValue(percent);
    m_lblStatus->setText(tr("Converting… %1%").arg(percent));
}

void MainWindow::onFinished(const QString& filePath)
{
    setUiState(QStringLiteral("idle"));
    m_progressBar->setValue(100);

    m_lblStatus->setObjectName(QStringLiteral("lblSuccess"));
    m_lblStatus->setText(tr("✅  Saved: %1").arg(filePath));
    applyStylesheet();

    saveSettings();
}

void MainWindow::onError(const QString& error)
{
    setUiState(QStringLiteral("idle"));

    m_lblStatus->setObjectName(QStringLiteral("lblError"));
    m_lblStatus->setText(tr("❌  %1").arg(error));
    applyStylesheet();

    qWarning() << "[MainWindow] Error:" << error;
}

void MainWindow::onStatus(const QString& msg)
{
    m_lblStatus->setObjectName(QStringLiteral("lblStatus"));
    m_lblStatus->setText(msg);
    // Don't re-apply stylesheet for normal status messages to avoid flicker
}

// ─────────────────────────────────────────────────────────────────────────────
// Thumbnail download
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::loadThumbnail(const QString& url)
{
    if (url.isEmpty()) {
        m_lblThumbnail->setText(QStringLiteral("🎬"));
        m_lblThumbnail->setPixmap(QPixmap());
        return;
    }

    QNetworkReply* reply = m_netManager->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[MainWindow] Thumbnail download failed:" << reply->errorString();
            return;
        }

        QPixmap pixmap;
        if (pixmap.loadFromData(reply->readAll())) {
            // Scale to fit label keeping aspect ratio
            m_lblThumbnail->setPixmap(
                pixmap.scaled(m_lblThumbnail->size(),
                              Qt::KeepAspectRatio,
                              Qt::SmoothTransformation));
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Close event
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_manager->isBusy()) {
        const auto answer = QMessageBox::question(
            this, tr("Download in progress"),
            tr("A download is still running. Cancel and quit?"),
            QMessageBox::Yes | QMessageBox::No);

        if (answer == QMessageBox::No) {
            event->ignore();
            return;
        }

        m_manager->cancel();
    }

    saveSettings();
    event->accept();
}
