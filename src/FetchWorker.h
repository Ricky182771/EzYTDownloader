#pragma once

#include <QObject>
#include <QProcess>
#include <QList>
#include "StreamInfo.h"

/// Wraps a QProcess running `python3 fetch.py`.
/// Parses JSON-line output for metadata, progress, completion, and errors.
class FetchWorker : public QObject {
    Q_OBJECT

public:
    explicit FetchWorker(QObject* parent = nullptr);
    ~FetchWorker() override;

    /// Fetch metadata for a URL.  Emits metadataReady() or errorOccurred().
    void fetchMetadata(const QString& url);

    /// Download a specific format.  Emits downloadProgress() / downloadFinished().
    /// If mergeAudio is true, passes --merge-audio so yt-dlp downloads
    /// video+bestaudio and merges them into mkv.
    void downloadStream(const QString& url, const QString& formatId,
                        const QString& outputPath, bool mergeAudio = false);

    /// Kill the running process.
    void cancel();

    bool isRunning() const;

signals:
    void metadataReady(const QList<StreamInfo>& streams,
                       const QString& title,
                       double duration,
                       const QString& thumbnail);
    void downloadProgress(double percent, const QString& speed, const QString& eta);
    void downloadFinished(const QString& filePath);
    void errorOccurred(const QString& message);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void startProcess(const QStringList& args);
    void parseJsonLine(const QByteArray& line);

    QProcess*  m_process = nullptr;
    QByteArray m_stdoutBuffer;
    bool       m_cancelled = false;
};
