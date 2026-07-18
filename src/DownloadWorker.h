#pragma once

#include <QObject>
#include <QString>
#include "PlaylistEntry.h"

class FetchWorker;

/// Downloads a single playlist entry in isolation by wrapping one FetchWorker.
///
/// Playlist items are fetched through yt-dlp's built-in post-processing
/// (`--extract-audio` / `--remux-video`), so — unlike the single-video pipeline —
/// no separate ConvertWorker/ffmpeg pass is required. Each DownloadWorker owns its
/// own FetchWorker (and therefore its own QProcess), which is what makes running
/// several of them at once genuinely parallel.
class DownloadWorker : public QObject {
    Q_OBJECT

public:
    DownloadWorker(PlaylistEntry entry,
                   QString outputDir,
                   QString format,
                   QString quality,
                   QString bitrate,
                   QObject* parent = nullptr);
    ~DownloadWorker() override;

    /// Kick off the download. Emits started(), then progress()* and exactly one
    /// terminal signal: finished() (possibly a soft skip) or failed().
    void start();

    /// Abort the underlying yt-dlp process. No terminal signal is emitted after this.
    void cancel();

    QString url()   const { return m_entry.url; }
    QString title() const { return m_entry.title; }
    const PlaylistEntry& entry() const { return m_entry; }

signals:
    void started(const QString& url);
    void progress(const QString& url, int percent, const QString& speed, const QString& eta);
    /// Terminal: the item finished. `skipped` is true when yt-dlp reported the
    /// entry as unavailable (soft skip) rather than a genuine download.
    void finished(const QString& url, bool skipped);
    /// Terminal: the item failed with an error message.
    void failed(const QString& url, const QString& error);

private slots:
    void onDownloadProgress(double percent, const QString& speed, const QString& eta);
    void onDownloadFinished(const QString& rawPath);
    void onError(const QString& msg);

private:
    PlaylistEntry m_entry;
    QString       m_outputDir;
    QString       m_format;
    QString       m_quality;
    QString       m_bitrate;

    FetchWorker*  m_fetch     = nullptr;
    bool          m_cancelled = false;
    bool          m_done      = false;   // guards against double terminal emission
};
