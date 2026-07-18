#include "DownloadManager.h"
#include "FetchWorker.h"
#include "ConvertWorker.h"
#include "Utils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <QUuid>

DownloadManager::DownloadManager(QObject* parent)
    : QObject(parent)
    , m_fetch(new FetchWorker(this))
    , m_convert(new ConvertWorker(this))
{
    // ── FetchWorker signals ─────────────────────────────────────────────
    connect(m_fetch, &FetchWorker::metadataReady,
            this,    &DownloadManager::onMetadataReady);
    connect(m_fetch, &FetchWorker::playlistDetected,
            this,    &DownloadManager::onPlaylistDetected);
    connect(m_fetch, &FetchWorker::downloadProgress,
            this,    &DownloadManager::onDownloadProgress);
    connect(m_fetch, &FetchWorker::downloadFinished,
            this,    &DownloadManager::onDownloadFinished);
    connect(m_fetch, &FetchWorker::errorOccurred,
            this,    &DownloadManager::onError);

    // ── ConvertWorker signals ───────────────────────────────────────────
    connect(m_convert, &ConvertWorker::progressChanged,
            this,      &DownloadManager::onConversionProgress);
    connect(m_convert, &ConvertWorker::finished,
            this,      &DownloadManager::onConversionFinished);
    connect(m_convert, &ConvertWorker::errorOccurred,
            this,      &DownloadManager::onError);
}

DownloadManager::~DownloadManager()
{
    cancel();
    cleanupTemp();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void DownloadManager::fetchMetadata(const QString& url)
{
    m_cancelled = false;
    emit statusMessage(tr("Fetching video info…"));
    m_fetch->fetchMetadata(url);
}

void DownloadManager::startDownload(const QString& url,
                                     const StreamInfo& stream,
                                     const QString& outputDir,
                                     const QString& format,
                                     const QString& bitrate)
{
    m_cancelled = false;
    m_url       = url;
    m_stream    = stream;
    m_outputDir = outputDir;
    m_format    = format;
    m_bitrate   = bitrate;

    // Build a temp file name in our temp dir.
    // For video: yt-dlp merges to mkv, so use .mkv extension.
    // For audio: use the stream's native extension.
    const QString tmpDir   = Utils::tempDownloadDir();
    const QString safeName = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    const bool    isVideo  = !stream.isAudioOnly;
    const QString tmpExt   = isVideo ? QStringLiteral("mkv") : stream.ext;
    m_tempPath = tmpDir + QStringLiteral("/ezyt_%1.%2").arg(safeName, tmpExt);

    emit statusMessage(tr("Downloading…"));
    emit downloadProgress(0, QString(), QString());

    // For video streams, merge with bestaudio so the output has sound.
    m_fetch->downloadStream(url, stream.formatId, m_tempPath, /*mergeAudio=*/isVideo);
}

void DownloadManager::downloadPlaylist(const QList<PlaylistEntry>& entries,
                                        const QString& outputDir,
                                        const QString& format,
                                        const QString& quality,
                                        const QString& bitrate)
{
    m_cancelled        = false;
    m_playlistMode     = true;
    m_playlistQueue    = entries;
    m_playlistIndex    = 0;
    m_playlistOutputDir = outputDir;
    m_playlistFormat   = format;
    m_playlistQuality  = quality;
    m_playlistBitrate  = bitrate;

    downloadNextPlaylistItem();
}

void DownloadManager::cancel()
{
    m_cancelled    = true;
    m_playlistMode = false;
    m_fetch->cancel();
    m_convert->cancel();
    cleanupTemp();
    emit statusMessage(tr("Cancelled."));
}

bool DownloadManager::isBusy() const
{
    return m_fetch->isRunning() || m_convert->isRunning();
}

// ─────────────────────────────────────────────────────────────────────────────
// Private slots
// ─────────────────────────────────────────────────────────────────────────────

void DownloadManager::onMetadataReady(const QList<StreamInfo>& streams,
                                       const QString& title,
                                       double duration,
                                       const QString& thumbnail)
{
    emit statusMessage(tr("Metadata fetched."));
    emit metadataReady(streams, title, duration, thumbnail);
}

void DownloadManager::onPlaylistDetected(const QList<PlaylistEntry>& entries,
                                          const QString& title)
{
    emit statusMessage(tr("Playlist detected: %1 (%2 videos)").arg(title).arg(entries.size()));
    emit playlistDetected(entries, title);
}

void DownloadManager::onDownloadProgress(double percent, const QString& speed, const QString& eta)
{
    emit downloadProgress(static_cast<int>(percent), speed, eta);
}

void DownloadManager::onDownloadFinished(const QString& rawPath)
{
    if (m_cancelled) return;

    if (m_playlistMode) {
        if (rawPath == QStringLiteral("__SKIPPED__")) {
            const QString skippedTitle = m_playlistQueue[m_playlistIndex].title;
            emit statusMessage(tr("Skipped (unavailable): %1").arg(skippedTitle));
        }
        ++m_playlistIndex;
        downloadNextPlaylistItem();
        return;
    }

    m_tempPath = rawPath;
    qDebug() << "[DownloadManager] Raw download complete:" << rawPath;

    emit statusMessage(tr("Converting…"));
    emit conversionProgress(0);

    const QString finalPath = buildFinalPath();
    m_convert->convert(m_tempPath, finalPath, m_format, m_bitrate, m_stream.duration);
}

void DownloadManager::onConversionProgress(int percent)
{
    emit conversionProgress(percent);
}

void DownloadManager::onConversionFinished(const QString& finalPath)
{
    if (m_cancelled) return;

    qDebug() << "[DownloadManager] Conversion complete:" << finalPath;
    cleanupTemp();
    emit statusMessage(tr("Done!"));
    emit finished(finalPath);
}

void DownloadManager::onError(const QString& msg)
{
    cleanupTemp();
    emit errorOccurred(msg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void DownloadManager::cleanupTemp()
{
    if (!m_tempPath.isEmpty() && QFile::exists(m_tempPath)) {
        QFile::remove(m_tempPath);
        qDebug() << "[DownloadManager] Removed temp:" << m_tempPath;
    }
    m_tempPath.clear();
}

void DownloadManager::downloadNextPlaylistItem()
{
    if (m_cancelled || m_playlistIndex >= m_playlistQueue.size()) {
        m_playlistMode = false;
        if (!m_cancelled) {
            emit statusMessage(tr("Playlist download complete!"));
            emit finished(m_playlistOutputDir);
        }
        return;
    }

    const PlaylistEntry& entry = m_playlistQueue[m_playlistIndex];
    const int total = m_playlistQueue.size();

    emit playlistItemStarted(m_playlistIndex + 1, total, entry.title);
    emit statusMessage(tr("Downloading %1/%2: %3")
                           .arg(m_playlistIndex + 1).arg(total).arg(entry.title));
    emit downloadProgress(0, QString(), QString());

    m_fetch->downloadPlaylistItem(entry.url, m_playlistFormat,
                                   m_playlistQuality, m_playlistBitrate,
                                   m_playlistOutputDir);
}

QString DownloadManager::buildFinalPath() const
{
    // Sanitize the title for a filename
    QString safeName = m_stream.title;
    safeName.replace(QRegularExpression(QStringLiteral(R"([<>:"/\\|?*])")), QStringLiteral("_"));
    safeName = safeName.trimmed();
    if (safeName.isEmpty())
        safeName = QStringLiteral("download");

    // Truncate to something reasonable
    if (safeName.length() > 120)
        safeName = safeName.left(120);

    const QString ext = m_format; // "mp4", "mkv", "mp3"
    QString path = m_outputDir + QStringLiteral("/") + safeName + QStringLiteral(".") + ext;

    // Avoid overwriting: append (1), (2), … if needed
    if (QFile::exists(path)) {
        int i = 1;
        while (QFile::exists(m_outputDir + QStringLiteral("/%1 (%2).%3").arg(safeName).arg(i).arg(ext)))
            ++i;
        path = m_outputDir + QStringLiteral("/%1 (%2).%3").arg(safeName).arg(i).arg(ext);
    }

    return path;
}
