#pragma once

#include <QObject>
#include <QList>
#include "StreamInfo.h"

class FetchWorker;
class ConvertWorker;

/// Orchestrates the full download pipeline:
///   1. Fetch metadata (yt-dlp --dump-json)
///   2. Download stream (yt-dlp)
///   3. Convert / remux (ffmpeg)
///   4. Cleanup temp files
class DownloadManager : public QObject {
    Q_OBJECT

public:
    explicit DownloadManager(QObject* parent = nullptr);
    ~DownloadManager() override;

    /// Phase 1: Fetch video metadata.
    void fetchMetadata(const QString& url);

    /// Phase 2+3: Download + convert a chosen stream.
    void startDownload(const QString& url,
                       const StreamInfo& stream,
                       const QString& outputDir,
                       const QString& format,    // "mp4", "mkv", "mp3"
                       const QString& bitrate);  // e.g. "320k" (mp3 only)

    /// Abort any running operation.
    void cancel();

    bool isBusy() const;

signals:
    // ── Metadata phase ──────────────────────────────────────────────────
    void metadataReady(const QList<StreamInfo>& streams,
                       const QString& title,
                       double duration,
                       const QString& thumbnail);

    // ── Download phase ──────────────────────────────────────────────────
    void downloadProgress(int percent, const QString& speed, const QString& eta);
    void statusMessage(const QString& msg);

    // ── Conversion phase ────────────────────────────────────────────────
    void conversionProgress(int percent);

    // ── Final outcome ───────────────────────────────────────────────────
    void finished(const QString& filePath);
    void errorOccurred(const QString& error);

private slots:
    void onMetadataReady(const QList<StreamInfo>& streams,
                         const QString& title,
                         double duration,
                         const QString& thumbnail);
    void onDownloadProgress(double percent, const QString& speed, const QString& eta);
    void onDownloadFinished(const QString& rawPath);
    void onConversionProgress(int percent);
    void onConversionFinished(const QString& finalPath);
    void onError(const QString& msg);

private:
    FetchWorker*   m_fetch   = nullptr;
    ConvertWorker* m_convert = nullptr;

    // State for the current download
    QString    m_url;
    StreamInfo m_stream;
    QString    m_outputDir;
    QString    m_format;
    QString    m_bitrate;
    QString    m_tempPath;   // raw downloaded file
    bool       m_cancelled = false;

    void cleanupTemp();
    QString buildFinalPath() const;
};
