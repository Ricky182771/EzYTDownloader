#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

/// Wraps a QProcess running ffmpeg for remuxing / audio conversion.
/// Parses ffmpeg stderr for progress (time= field vs total duration).
class ConvertWorker : public QObject {
    Q_OBJECT

public:
    explicit ConvertWorker(QObject* parent = nullptr);
    ~ConvertWorker() override;

    /// Remux or convert a downloaded file.
    ///
    /// @param input       Path to the raw downloaded file
    /// @param output      Desired final output path (e.g. .mp4, .mkv, .mp3)
    /// @param format      "mp4", "mkv", or "mp3"
    /// @param bitrate     Audio bitrate string for mp3 (e.g. "320k"), ignored for video remux
    /// @param durationSecs  Total duration in seconds (for progress calculation)
    void convert(const QString& input, const QString& output,
                 const QString& format, const QString& bitrate,
                 double durationSecs);

    /// Kill the ffmpeg process.
    void cancel();

    bool isRunning() const;

signals:
    void progressChanged(int percent);
    void finished(const QString& outputPath);
    void errorOccurred(const QString& error);

private slots:
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    QProcess*  m_process       = nullptr;
    double     m_totalDuration = 0.0;
    QString    m_outputPath;
    bool       m_cancelled     = false;
    QByteArray m_stderrBuffer;

    void parseFfmpegProgress(const QByteArray& data);
    static double parseTimeToSeconds(const QString& timeStr);
};
