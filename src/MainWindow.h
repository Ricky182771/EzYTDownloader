#pragma once

#include <QMainWindow>
#include <QList>
#include "StreamInfo.h"

class QLineEdit;
class QComboBox;
class QProgressBar;
class QLabel;
class QPushButton;
class QGroupBox;
class DownloadManager;
class QNetworkAccessManager;

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

    // Output
    QLineEdit*   m_editOutputDir = nullptr;
    QPushButton* m_btnBrowse     = nullptr;

    // Progress
    QProgressBar* m_progressBar  = nullptr;
    QLabel*       m_lblStatus    = nullptr;

    // Actions
    QPushButton* m_btnDownload   = nullptr;
    QPushButton* m_btnCancel     = nullptr;

    // ── Logic ───────────────────────────────────────────────────────────
    DownloadManager*       m_manager    = nullptr;
    QNetworkAccessManager* m_netManager = nullptr;
    QList<StreamInfo>      m_streams;
    QString                m_currentUrl;
    double                 m_currentDuration = 0.0;
};
