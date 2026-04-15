#include "ConvertWorker.h"

#include <QRegularExpression>
#include <QDebug>

ConvertWorker::ConvertWorker(QObject* parent)
    : QObject(parent)
{}

ConvertWorker::~ConvertWorker()
{
    cancel();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void ConvertWorker::convert(const QString& input, const QString& output,
                             const QString& format, const QString& bitrate,
                             double durationSecs)
{
    cancel();

    m_totalDuration = durationSecs;
    m_outputPath    = output;
    m_cancelled     = false;
    m_stderrBuffer.clear();

    QStringList args;
    args << QStringLiteral("-y")                 // overwrite
         << QStringLiteral("-i") << input         // input
         << QStringLiteral("-progress") << QStringLiteral("pipe:2"); // progress to stderr

    if (format == QStringLiteral("mp3")) {
        // Audio extraction + conversion to MP3
        args << QStringLiteral("-vn")             // no video
             << QStringLiteral("-acodec") << QStringLiteral("libmp3lame")
             << QStringLiteral("-ab") << bitrate  // e.g. "320k"
             << QStringLiteral("-f") << QStringLiteral("mp3");
    } else if (format == QStringLiteral("mkv")) {
        // Remux to MKV — copy all streams (MKV supports any codec)
        args << QStringLiteral("-c") << QStringLiteral("copy")
             << QStringLiteral("-f") << QStringLiteral("matroska");
    } else {
        // Remux to MP4 — copy video, re-encode audio to AAC.
        // Opus inside MP4 is not widely supported by players, so we
        // always transcode audio to AAC for maximum compatibility.
        args << QStringLiteral("-c:v") << QStringLiteral("copy")
             << QStringLiteral("-c:a") << QStringLiteral("aac")
             << QStringLiteral("-b:a") << QStringLiteral("192k")
             << QStringLiteral("-movflags") << QStringLiteral("+faststart")
             << QStringLiteral("-f") << QStringLiteral("mp4");
    }

    args << output;

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardError,
            this,      &ConvertWorker::onReadyReadStderr);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,      &ConvertWorker::onProcessFinished);

    qDebug() << "[ConvertWorker] Starting: ffmpeg" << args;
    m_process->start(QStringLiteral("ffmpeg"), args);
}

void ConvertWorker::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_cancelled = true;
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

bool ConvertWorker::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void ConvertWorker::onReadyReadStderr()
{
    m_stderrBuffer += m_process->readAllStandardError();

    while (true) {
        int idxN = m_stderrBuffer.indexOf('\n');
        int idxR = m_stderrBuffer.indexOf('\r');

        int idx;
        if (idxN < 0 && idxR < 0) break;
        else if (idxN < 0)        idx = idxR;
        else if (idxR < 0)        idx = idxN;
        else                      idx = qMin(idxN, idxR);

        const QByteArray line = m_stderrBuffer.left(idx).trimmed();
        m_stderrBuffer.remove(0, idx + 1);

        if (!line.isEmpty())
            parseFfmpegProgress(line);
    }
}

void ConvertWorker::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (m_cancelled) {
        m_process->deleteLater();
        m_process = nullptr;
        return;
    }

    if (status == QProcess::CrashExit || exitCode != 0) {
        emit errorOccurred(tr("ffmpeg exited with code %1.\n%2")
            .arg(exitCode)
            .arg(QString::fromUtf8(m_stderrBuffer).right(500)));
    } else {
        emit progressChanged(100);
        emit finished(m_outputPath);
    }

    m_process->deleteLater();
    m_process = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// ffmpeg progress parsing
// ─────────────────────────────────────────────────────────────────────────────

void ConvertWorker::parseFfmpegProgress(const QByteArray& data)
{
    if (m_totalDuration <= 0.0)
        return;

    // ffmpeg with -progress pipe:2 outputs key=value lines.
    // We look for "out_time_ms=<microseconds>" or "out_time=HH:MM:SS.xxxxxx"
    const QString text = QString::fromUtf8(data);

    // Try out_time_ms first (microseconds)
    static const QRegularExpression reMicro(QStringLiteral(R"(out_time_ms=(\d+))"));
    auto match = reMicro.match(text);

    double currentSecs = 0.0;

    if (match.hasMatch()) {
        const qint64 us = match.captured(1).toLongLong();
        currentSecs = static_cast<double>(us) / 1'000'000.0;
    } else {
        // Fallback: out_time=HH:MM:SS.micro
        static const QRegularExpression reTime(
            QStringLiteral(R"(out_time=(\d+:\d+:\d+\.\d+))"));
        match = reTime.match(text);
        if (match.hasMatch())
            currentSecs = parseTimeToSeconds(match.captured(1));
    }

    if (currentSecs > 0.0) {
        int pct = static_cast<int>((currentSecs / m_totalDuration) * 100.0);
        pct = qBound(0, pct, 99); // don't emit 100 until process finishes
        emit progressChanged(pct);
    }
}

double ConvertWorker::parseTimeToSeconds(const QString& timeStr)
{
    // Format: HH:MM:SS.micro
    const QStringList parts = timeStr.split(QLatin1Char(':'));
    if (parts.size() != 3)
        return 0.0;

    const double h = parts[0].toDouble();
    const double m = parts[1].toDouble();
    const double s = parts[2].toDouble();
    return h * 3600.0 + m * 60.0 + s;
}
