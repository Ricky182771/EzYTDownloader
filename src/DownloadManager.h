#pragma once

#include <QObject>
#include <QList>
#include "StreamInfo.h"
#include "PlaylistEntry.h"

class FetchWorker;
class ConvertWorker;

/// Orchestrates the full download pipeline for single videos and playlists.
///
/// Single video:  fetch metadata → download stream → convert/remux via ffmpeg
/// Playlist:      fetch entry list → download each item sequentially via yt-dlp post-processing
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

    /// Playlist: Download all entries sequentially using yt-dlp format selectors.
    /// quality: "best", "1080", "720", "480", "360"
    void downloadPlaylist(const QList<PlaylistEntry>& entries,
                          const QString& outputDir,
                          const QString& format,
                          const QString& quality,
                          const QString& bitrate);

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

    // ── Download phase ──────────────────────────────────────────────────
    void downloadProgress(int percent, const QString& speed, const QString& eta);
    void statusMessage(const QString& msg);

    // ── Playlist progress ───────────────────────────────────────────────
    void playlistItemStarted(int current, int total, const QString& currentTitle);

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

private:
    FetchWorker*   m_fetch   = nullptr;
    ConvertWorker* m_convert = nullptr;

    // ── Single-video state ───────────────────────────────────────────────
    QString    m_url;
    StreamInfo m_stream;
    QString    m_outputDir;
    QString    m_format;
    QString    m_bitrate;
    QString    m_tempPath;
    bool       m_cancelled = false;

    // ── Playlist state ───────────────────────────────────────────────────
    bool                 m_playlistMode  = false;
    QList<PlaylistEntry> m_playlistQueue;
    int                  m_playlistIndex = 0;
    QString              m_playlistOutputDir;
    QString              m_playlistFormat;
    QString              m_playlistQuality;
    QString              m_playlistBitrate;

    void cleanupTemp();
    QString buildFinalPath() const;
    void downloadNextPlaylistItem();
};
