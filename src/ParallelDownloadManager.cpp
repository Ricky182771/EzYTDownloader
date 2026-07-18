#include "ParallelDownloadManager.h"
#include "DownloadWorker.h"

#include <QDebug>

ParallelDownloadManager::ParallelDownloadManager(QObject* parent)
    : QObject(parent)
{}

ParallelDownloadManager::~ParallelDownloadManager()
{
    cancel();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void ParallelDownloadManager::start(const QList<PlaylistEntry>& entries,
                                     const QString& outputDir,
                                     const QString& format,
                                     const QString& quality,
                                     const QString& bitrate,
                                     int maxParallel)
{
    cancel(); // clear any prior run

    m_cancelled   = false;
    m_outputDir   = outputDir;
    m_format      = format;
    m_quality     = quality;
    m_bitrate     = bitrate;
    m_maxParallel = qBound(1, maxParallel, kMaxParallel);
    m_total       = entries.size();
    m_completed   = 0;
    m_failed      = 0;

    m_queue.clear();
    m_indexForUrl.clear();
    int idx = 1;
    for (const auto& e : entries) {
        m_queue.enqueue(e);
        m_indexForUrl.insert(e.url, idx++);
    }

    emitQueueStatus();
    pump();
}

void ParallelDownloadManager::cancel()
{
    m_cancelled = true;
    m_queue.clear();

    const auto workers = m_active;
    m_active.clear();
    for (DownloadWorker* w : workers) {
        w->cancel();
        w->deleteLater();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Worker slots
// ─────────────────────────────────────────────────────────────────────────────

void ParallelDownloadManager::onItemFinished(const QString& url, bool skipped)
{
    if (m_cancelled)
        return;

    DownloadWorker* w = activeWorker(url);
    const QString title = w ? w->title() : QString();

    ++m_completed;
    if (w) retire(w);

    emit itemFinished(url, title, skipped);
    emitQueueStatus();

    pump();
    checkAllFinished();
}

void ParallelDownloadManager::onItemFailed(const QString& url, const QString& error)
{
    if (m_cancelled)
        return;

    DownloadWorker* w = activeWorker(url);
    const QString title = w ? w->title() : QString();

    ++m_failed;
    if (w) retire(w);

    emit itemFailed(url, title, error);
    emitQueueStatus();

    pump();
    checkAllFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal
// ─────────────────────────────────────────────────────────────────────────────

void ParallelDownloadManager::pump()
{
    while (!m_cancelled && !m_queue.isEmpty() && m_active.size() < m_maxParallel) {
        const PlaylistEntry entry = m_queue.dequeue();

        auto* w = new DownloadWorker(entry, m_outputDir, m_format,
                                     m_quality, m_bitrate, this);
        connect(w, &DownloadWorker::progress,
                this, &ParallelDownloadManager::itemProgress);
        connect(w, &DownloadWorker::finished,
                this, &ParallelDownloadManager::onItemFinished);
        connect(w, &DownloadWorker::failed,
                this, &ParallelDownloadManager::onItemFailed);

        m_active.append(w);
        emit itemStarted(entry.url, entry.title,
                         m_indexForUrl.value(entry.url, 0), m_total);
        w->start();
    }

    emitQueueStatus();
}

void ParallelDownloadManager::retire(DownloadWorker* w)
{
    m_active.removeOne(w);
    w->deleteLater();
}

DownloadWorker* ParallelDownloadManager::activeWorker(const QString& url) const
{
    for (DownloadWorker* w : m_active)
        if (w->url() == url)
            return w;
    return nullptr;
}

void ParallelDownloadManager::emitQueueStatus()
{
    emit queueStatus(m_active.size(), m_queue.size(),
                     m_completed + m_failed, m_total);
}

void ParallelDownloadManager::checkAllFinished()
{
    if (m_cancelled || !m_queue.isEmpty() || !m_active.isEmpty())
        return;

    emit allFinished(m_outputDir, m_completed, m_failed);
}
