#pragma once

#include <QObject>
#include <QList>
#include "StreamInfo.h"
#include "PlaylistEntry.h"

class FetchWorker;
class ConvertWorker;
class ParallelDownloadManager;

/// Orchestrates the full download pipeline for single videos and playlists.
///
/// Single video:  fetch metadata → download stream → convert/remux via ffmpeg
/// Playlist:      fetch entry list → delegate to ParallelDownloadManager, which
///                downloads up to N items at once via yt-dlp post-processing
class DownloadManager : public QObject {
    Q_OBJECT

public:
    explicit DownloadManager(QObject* parent = nullptr);
    ~DownloadManager() override;

    /// Phase 1: Fetch video or playlist metadata.
    void fetchMetadata(const QString& url);

    /// Phase 2+3 (single video): Download + convert a chosen stream.
    void startDownload(const QString& url,
                       const StreamInfo& stream,
                       const QString& outputDir,
                       const QString& format,    // "mp4", "mkv", "mp3"
                       const QString& bitrate);  // e.g. "320k" (mp3 only)

    /// Playlist: Download all entries using yt-dlp format selectors.
    /// quality: "best", "1080", "720", "480", "360"
    /// maxParallel: number of simultaneous downloads (1 = sequential).
    void downloadPlaylist(const QList<PlaylistEntry>& entries,
                          const QString& outputDir,
                          const QString& format,
                          const QString& quality,
                          const QString& bitrate,
                          int maxParallel = 1);

    /// Abort any running operation.
    void cancel();

    bool isBusy() const;

signals:
    // ── Metadata phase ──────────────────────────────────────────────────
    void metadataReady(const QList<StreamInfo>& streams,
                       const QString& title,
                       double duration,
                       const QString& thumbnail);
    void playlistDetected(const QList<PlaylistEntry>& entries, const QString& title);

    // ── Download phase (single video) ───────────────────────────────────
    void downloadProgress(int percent, const QString& speed, const QString& eta);
    void statusMessage(const QString& msg);

    // ── Playlist progress (parallel) ────────────────────────────────────
    void playlistItemStarted(const QString& url, const QString& title, int index, int total);
    void playlistItemProgress(const QString& url, int percent, const QString& speed, const QString& eta);
    void playlistItemFinished(const QString& url, const QString& title, bool skipped);
    void playlistItemFailed(const QString& url, const QString& title, const QString& error);
    void playlistQueueStatus(int active, int queued, int processed, int total);

    // ── Conversion phase (single video only) ────────────────────────────
    void conversionProgress(int percent);

    // ── Final outcome ───────────────────────────────────────────────────
    void finished(const QString& filePath);
    void errorOccurred(const QString& error);

private slots:
    void onMetadataReady(const QList<StreamInfo>& streams,
                         const QString& title,
                         double duration,
                         const QString& thumbnail);
    void onPlaylistDetected(const QList<PlaylistEntry>& entries, const QString& title);
    void onDownloadProgress(double percent, const QString& speed, const QString& eta);
    void onDownloadFinished(const QString& rawPath);
    void onConversionProgress(int percent);
    void onConversionFinished(const QString& finalPath);
    void onError(const QString& msg);

    // ── Parallel manager relays ─────────────────────────────────────────
    void onPlaylistItemStarted(const QString& url, const QString& title, int index, int total);
    void onPlaylistAllFinished(const QString& outputDir, int completed, int failed);

private:
    FetchWorker*             m_fetch    = nullptr;
    ConvertWorker*           m_convert  = nullptr;
    ParallelDownloadManager* m_parallel = nullptr;

    // ── Single-video state ───────────────────────────────────────────────
    QString    m_url;
    StreamInfo m_stream;
    QString    m_outputDir;
    QString    m_format;
    QString    m_bitrate;
    QString    m_tempPath;
    bool       m_cancelled = false;

    void cleanupTemp();
    QString buildFinalPath() const;
};
