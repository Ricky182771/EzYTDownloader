#pragma once

#include <QMainWindow>
#include <QList>
#include <QHash>
#include "StreamInfo.h"
#include "PlaylistEntry.h"

class QLineEdit;
class QComboBox;
class QSpinBox;
class QProgressBar;
class QLabel;
class QPushButton;
class QGroupBox;
class QTableWidget;
class DownloadManager;
class QNetworkAccessManager;
class QPropertyAnimation;

/// The main application window — URL input, format/resolution/bitrate selectors,
/// progress display, and download controls.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // ── User actions ────────────────────────────────────────────────────
    void onFetchClicked();
    void onDownloadClicked();
    void onCancelClicked();
    void onBrowseClicked();
    void onFormatChanged(int index);
    void onPasteClicked();

    // ── DownloadManager signals ─────────────────────────────────────────
    void onMetadataReady(const QList<StreamInfo>& streams,
                         const QString& title,
                         double duration,
                         const QString& thumbnail);
    void onPlaylistDetected(const QList<PlaylistEntry>& entries, const QString& title);
    void onPlaylistItemStarted(const QString& url, const QString& title, int index, int total);
    void onPlaylistItemProgress(const QString& url, int percent, const QString& speed, const QString& eta);
    void onPlaylistItemFinished(const QString& url, const QString& title, bool skipped);
    void onPlaylistItemFailed(const QString& url, const QString& title, const QString& error);
    void onPlaylistQueueStatus(int active, int queued, int processed, int total);
    void onDownloadProgress(int percent, const QString& speed, const QString& eta);
    void onConversionProgress(int percent);
    void onFinished(const QString& filePath);
    void onError(const QString& error);
    void onStatus(const QString& msg);

private:
    void setupUi();
    void applyStylesheet();
    void loadSettings();
    void saveSettings();
    void setUiState(const QString& state); // "idle", "fetching", "downloading", "converting"
    void loadThumbnail(const QString& url);
    void buildProgressTable();                 // one row per playlist entry
    int  rowForUrl(const QString& url) const;  // -1 if unknown

    // ── Animation helpers ────────────────────────────────────────────────
    void fadeInWidget(QWidget* w, int durationMs = 220); // smooth reveal
    void setProgressAnimated(int value);                 // interpolate the bar

    // ── Widgets ─────────────────────────────────────────────────────────
    // Header
    QLabel*      m_lblTitle      = nullptr;
    QLabel*      m_lblSubtitle   = nullptr;

    // URL bar
    QLineEdit*   m_editUrl       = nullptr;
    QPushButton* m_btnPaste      = nullptr;
    QPushButton* m_btnFetch      = nullptr;

    // Video info
    QGroupBox*   m_grpInfo       = nullptr;
    QLabel*      m_lblVideoTitle = nullptr;
    QLabel*      m_lblDuration   = nullptr;
    QLabel*      m_lblThumbnail  = nullptr;

    // Options
    QGroupBox*   m_grpOptions    = nullptr;
    QComboBox*   m_cmbFormat     = nullptr;
    QComboBox*   m_cmbResolution = nullptr;
    QComboBox*   m_cmbBitrate    = nullptr;
    QLabel*      m_lblFormat     = nullptr;
    QLabel*      m_lblResolution = nullptr;
    QLabel*      m_lblBitrate    = nullptr;

    // Parallel downloads (playlist only)
    QLabel*      m_lblParallel   = nullptr;
    QSpinBox*    m_spinParallel  = nullptr;

    // Output
    QLineEdit*   m_editOutputDir = nullptr;
    QPushButton* m_btnBrowse     = nullptr;

    // Progress
    QProgressBar* m_progressBar  = nullptr;
    QLabel*       m_lblStatus    = nullptr;

    // Parallel download progress table (playlist only)
    QGroupBox*    m_grpProgress  = nullptr;
    QLabel*       m_lblParallelInfo = nullptr;
    QTableWidget* m_tableProgress   = nullptr;

    // Actions
    QPushButton* m_btnDownload   = nullptr;
    QPushButton* m_btnCancel     = nullptr;

    // ── Logic ───────────────────────────────────────────────────────────
    DownloadManager*       m_manager      = nullptr;
    QNetworkAccessManager* m_netManager   = nullptr;
    QPropertyAnimation*    m_progressAnim = nullptr; // smooth main progress bar
    QList<StreamInfo>      m_streams;
    QString                m_currentUrl;
    double                 m_currentDuration = 0.0;

    // ── Playlist state ───────────────────────────────────────────────────
    bool                 m_isPlaylist = false;
    QList<PlaylistEntry> m_playlistEntries;
    QHash<QString, int>  m_rowForUrl;   // playlist URL → progress-table row
};
