#pragma once

#include <QObject>
#include <QList>
#include <QQueue>
#include <QHash>
#include "PlaylistEntry.h"

class DownloadWorker;

/// Orchestrates parallel downloading of a playlist.
///
/// Holds a queue of entries and runs up to `maxParallel` DownloadWorkers at once.
/// As each worker finishes, the next queued entry is launched until the queue is
/// drained, then `allFinished()` is emitted. A `maxParallel` of 1 degrades
/// gracefully to sequential behaviour.
class ParallelDownloadManager : public QObject {
    Q_OBJECT

public:
    explicit ParallelDownloadManager(QObject* parent = nullptr);
    ~ParallelDownloadManager() override;

    /// Begin downloading all entries. Clamps `maxParallel` to [1, kMaxParallel].
    void start(const QList<PlaylistEntry>& entries,
               const QString& outputDir,
               const QString& format,
               const QString& quality,
               const QString& bitrate,
               int maxParallel);

    /// Cancel every active worker and drop the queue.
    void cancel();

    bool isBusy() const { return !m_active.isEmpty() || !m_queue.isEmpty(); }

    static constexpr int kMaxParallel = 5;

signals:
    void itemStarted(const QString& url, const QString& title, int index, int total);
    void itemProgress(const QString& url, int percent, const QString& speed, const QString& eta);
    void itemFinished(const QString& url, const QString& title, bool skipped);
    void itemFailed(const QString& url, const QString& title, const QString& error);
    /// Aggregate state after every change. `processed` = finished + failed so far.
    void queueStatus(int active, int queued, int processed, int total);
    void allFinished(const QString& outputDir, int completed, int failed);

private slots:
    void onItemFinished(const QString& url, bool skipped);
    void onItemFailed(const QString& url, const QString& error);

private:
    void pump();                       // launch queued items up to the parallel limit
    void retire(DownloadWorker* w);    // remove & delete a finished worker
    DownloadWorker* activeWorker(const QString& url) const;
    void emitQueueStatus();
    void checkAllFinished();

    QQueue<PlaylistEntry>  m_queue;
    QList<DownloadWorker*> m_active;
    QHash<QString, int>    m_indexForUrl;   // 1-based display position per URL

    QString m_outputDir;
    QString m_format;
    QString m_quality;
    QString m_bitrate;

    int  m_maxParallel = 1;
    int  m_total       = 0;
    int  m_completed   = 0;   // finished (including soft skips)
    int  m_failed      = 0;
    bool m_cancelled   = false;
};
