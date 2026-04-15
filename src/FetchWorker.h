#pragma once

#include <QObject>
#include <QProcess>
#include <QList>
#include "StreamInfo.h"

/// Wraps a QProcess running `yt-dlp` directly (no Python intermediary).
/// For metadata: parses the JSON blob from `yt-dlp --dump-json`.
/// For downloads: parses custom progress lines via --progress-template.
class FetchWorker : public QObject {
    Q_OBJECT

public:
    explicit FetchWorker(QObject* parent = nullptr);
    ~FetchWorker() override;

    /// Fetch metadata for a URL.  Emits metadataReady() or errorOccurred().
    void fetchMetadata(const QString& url);

    /// Download a specific format.  Emits downloadProgress() / downloadFinished().
    /// If mergeAudio is true, requests video+bestaudio and merges into the
    /// extension derived from outputPath.
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
    enum class Mode { Metadata, Download };

    void startProcess(const QStringList& args, Mode mode);
    void parseMetadataJson(const QByteArray& json);
    void parseProgressLine(const QByteArray& line);

    QProcess*  m_process = nullptr;
    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
    bool       m_cancelled = false;
    Mode       m_mode      = Mode::Metadata;
    QString    m_expectedOutputPath;   // for download completion
};
