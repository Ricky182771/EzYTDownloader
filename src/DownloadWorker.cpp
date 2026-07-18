#include "DownloadWorker.h"
#include "FetchWorker.h"

#include <QDebug>

DownloadWorker::DownloadWorker(PlaylistEntry entry,
                               QString outputDir,
                               QString format,
                               QString quality,
                               QString bitrate,
                               QObject* parent)
    : QObject(parent)
    , m_entry(std::move(entry))
    , m_outputDir(std::move(outputDir))
    , m_format(std::move(format))
    , m_quality(std::move(quality))
    , m_bitrate(std::move(bitrate))
{}

DownloadWorker::~DownloadWorker()
{
    cancel();
}

void DownloadWorker::start()
{
    if (m_fetch)
        return; // already running

    m_fetch = new FetchWorker(this);
    connect(m_fetch, &FetchWorker::downloadProgress,
            this,    &DownloadWorker::onDownloadProgress);
    connect(m_fetch, &FetchWorker::downloadFinished,
            this,    &DownloadWorker::onDownloadFinished);
    connect(m_fetch, &FetchWorker::errorOccurred,
            this,    &DownloadWorker::onError);

    emit started(m_entry.url);

    m_fetch->downloadPlaylistItem(m_entry.url, m_format,
                                  m_quality, m_bitrate, m_outputDir);
}

void DownloadWorker::cancel()
{
    m_cancelled = true;
    if (m_fetch) {
        m_fetch->cancel();
        m_fetch->deleteLater();
        m_fetch = nullptr;
    }
}

void DownloadWorker::onDownloadProgress(double percent, const QString& speed, const QString& eta)
{
    if (m_cancelled || m_done)
        return;
    emit progress(m_entry.url, static_cast<int>(percent), speed, eta);
}

void DownloadWorker::onDownloadFinished(const QString& rawPath)
{
    if (m_cancelled || m_done)
        return;
    m_done = true;
    // FetchWorker signals a soft skip (unavailable entry) with the "__SKIPPED__" marker.
    emit finished(m_entry.url, rawPath == QStringLiteral("__SKIPPED__"));
}

void DownloadWorker::onError(const QString& msg)
{
    if (m_cancelled || m_done)
        return;
    m_done = true;
    emit failed(m_entry.url, msg);
}
