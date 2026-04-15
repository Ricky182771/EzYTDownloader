#include "FetchWorker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

FetchWorker::FetchWorker(QObject* parent)
    : QObject(parent)
{}

FetchWorker::~FetchWorker()
{
    cancel();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void FetchWorker::fetchMetadata(const QString& url)
{
    QStringList args = {
        QStringLiteral("--dump-json"),
        QStringLiteral("--no-download"),
        QStringLiteral("--no-warnings"),
        url,
    };

    startProcess(args, Mode::Metadata);
}

void FetchWorker::downloadStream(const QString& url, const QString& formatId,
                                  const QString& outputPath, bool mergeAudio)
{
    m_expectedOutputPath = outputPath;

    // Derive the extension for --merge-output-format
    const QString ext = QFileInfo(outputPath).suffix();

    // Build the format string
    QString dlFormat;
    if (mergeAudio)
        dlFormat = QStringLiteral("%1+bestaudio/%1").arg(formatId);
    else
        dlFormat = formatId;

    QStringList args = {
        QStringLiteral("-f"), dlFormat,
        QStringLiteral("-o"), outputPath,
        QStringLiteral("--no-warnings"),
        QStringLiteral("--newline"),
        QStringLiteral("--progress-template"),
        QStringLiteral("download:EZPROG %(progress._percent_str)s %(progress._speed_str)s %(progress._eta_str)s"),
    };

    if (mergeAudio && !ext.isEmpty())
        args << QStringLiteral("--merge-output-format") << ext;

    args << url;

    startProcess(args, Mode::Download);
}

void FetchWorker::cancel()
{
    if (m_process) {
        m_cancelled = true;
        m_process->disconnect();   // disconnect ALL signals first
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
        m_process->deleteLater();
        m_process = nullptr;
    }
}

bool FetchWorker::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal
// ─────────────────────────────────────────────────────────────────────────────

void FetchWorker::startProcess(const QStringList& args, Mode mode)
{
    // Fully clean up any prior process — disconnect its signals so a delayed
    // 'finished' from the old process can never touch the new one.
    cancel();

    m_mode         = mode;
    m_cancelled    = false;
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this,      &FetchWorker::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this,      &FetchWorker::onReadyReadStderr);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,      &FetchWorker::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this,      &FetchWorker::onProcessError);

    qDebug() << "[FetchWorker] Starting: yt-dlp" << args;
    m_process->start(QStringLiteral("yt-dlp"), args);

    if (!m_process->waitForStarted(5000)) {
        emit errorOccurred(tr("Failed to start yt-dlp. Is it installed?"));
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void FetchWorker::onReadyReadStdout()
{
    if (!m_process) return;
    m_stdoutBuffer += m_process->readAllStandardOutput();

    if (m_mode == Mode::Metadata) {
        // For metadata, yt-dlp dumps one big JSON object.
        // We accumulate everything and parse on process finish.
        return;
    }

    // Download mode: process line-by-line for progress updates.
    while (true) {
        int idxN = m_stdoutBuffer.indexOf('\n');
        int idxR = m_stdoutBuffer.indexOf('\r');

        int idx;
        if (idxN < 0 && idxR < 0) break;
        else if (idxN < 0)        idx = idxR;
        else if (idxR < 0)        idx = idxN;
        else                       idx = qMin(idxN, idxR);

        const QByteArray line = m_stdoutBuffer.left(idx).trimmed();
        m_stdoutBuffer.remove(0, idx + 1);

        if (!line.isEmpty())
            parseProgressLine(line);
    }
}

void FetchWorker::onReadyReadStderr()
{
    if (!m_process) return;
    m_stderrBuffer += m_process->readAllStandardError();

    // Process complete lines from stderr
    while (true) {
        int idxN = m_stderrBuffer.indexOf('\n');
        int idxR = m_stderrBuffer.indexOf('\r');

        int idx;
        if (idxN < 0 && idxR < 0) break;
        else if (idxN < 0)        idx = idxR;
        else if (idxR < 0)        idx = idxN;
        else                       idx = qMin(idxN, idxR);

        const QByteArray line = m_stderrBuffer.left(idx).trimmed();
        m_stderrBuffer.remove(0, idx + 1);

        if (line.isEmpty())
            continue;

        // In download mode, yt-dlp emits --progress-template output to stderr
        if (m_mode == Mode::Download && line.contains("EZPROG")) {
            parseProgressLine(line);
        } else {
            qWarning() << "[FetchWorker] stderr:" << line;
        }
    }
}

void FetchWorker::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    if (m_cancelled || !m_process) return;

    emit errorOccurred(tr("Process error: %1").arg(m_process->errorString()));
}

void FetchWorker::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_process) return;

    // Collect any remaining stdout
    m_stdoutBuffer += m_process->readAllStandardOutput();

    if (m_mode == Mode::Metadata) {
        // Parse the entire accumulated JSON blob
        if (!m_cancelled && status == QProcess::NormalExit && exitCode == 0)
            parseMetadataJson(m_stdoutBuffer);
    } else {
        // Download mode: process remaining progress lines
        while (true) {
            const int idx = m_stdoutBuffer.indexOf('\n');
            if (idx < 0)
                break;
            const QByteArray line = m_stdoutBuffer.left(idx).trimmed();
            m_stdoutBuffer.remove(0, idx + 1);
            if (!line.isEmpty())
                parseProgressLine(line);
        }
    }

    m_stdoutBuffer.clear();

    // Clean up the process before emitting any signals, so that if a slot
    // calls startProcess() again we don't have a stale m_process pointer.
    m_process->deleteLater();
    m_process = nullptr;

    if (m_cancelled)
        return; // user-initiated cancel, no error

    if (status == QProcess::CrashExit || exitCode != 0) {
        emit errorOccurred(tr("yt-dlp exited with code %1").arg(exitCode));
        return;
    }

    // For download mode, emit completion now that the process exited successfully
    if (m_mode == Mode::Download) {
        // Find the actual output file
        if (QFileInfo::exists(m_expectedOutputPath)) {
            emit downloadFinished(m_expectedOutputPath);
        } else {
            // yt-dlp may have adjusted the extension; search for it
            const QFileInfo fi(m_expectedOutputPath);
            const QString base = fi.path() + QStringLiteral("/") + fi.completeBaseName();
            const QDir dir(fi.path());
            const QStringList filters = {fi.completeBaseName() + QStringLiteral(".*")};
            QStringList candidates;
            for (const auto& f : dir.entryList(filters, QDir::Files)) {
                const QString full = fi.path() + QStringLiteral("/") + f;
                if (!full.endsWith(QStringLiteral(".part")) &&
                    !f.contains(QStringLiteral(".f")))
                    candidates << full;
            }
            if (!candidates.isEmpty()) {
                // Pick the most recently modified
                QString actual = candidates.first();
                QDateTime newest;
                for (const auto& c : candidates) {
                    QDateTime mod = QFileInfo(c).lastModified();
                    if (!newest.isValid() || mod > newest) {
                        newest = mod;
                        actual = c;
                    }
                }
                emit downloadFinished(actual);
            } else {
                emit errorOccurred(tr("Download completed but file not found: %1")
                                       .arg(m_expectedOutputPath));
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Parsing
// ─────────────────────────────────────────────────────────────────────────────

void FetchWorker::parseMetadataJson(const QByteArray& json)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.trimmed(), &err);
    if (err.error != QJsonParseError::NoError) {
        emit errorOccurred(tr("Failed to parse yt-dlp metadata: %1").arg(err.errorString()));
        return;
    }

    const QJsonObject info = doc.object();

    const QString title     = info.value(QStringLiteral("title")).toString(QStringLiteral("Unknown"));
    const double  duration  = info.value(QStringLiteral("duration")).toDouble(0);
    const QString thumbnail = info.value(QStringLiteral("thumbnail")).toString();
    const QJsonArray formats = info.value(QStringLiteral("formats")).toArray();

    QList<StreamInfo> streams;

    for (const auto& fmtVal : formats) {
        const QJsonObject fmt = fmtVal.toObject();

        const QString fmtId  = fmt.value(QStringLiteral("format_id")).toString();
        const QString ext    = fmt.value(QStringLiteral("ext")).toString();
        const QString vcodec = fmt.value(QStringLiteral("vcodec")).toString(QStringLiteral("none"));
        const QString acodec = fmt.value(QStringLiteral("acodec")).toString(QStringLiteral("none"));
        const int height     = fmt.value(QStringLiteral("height")).toInt(0);
        const int fps        = fmt.value(QStringLiteral("fps")).toInt(0);
        const double tbr     = fmt.value(QStringLiteral("tbr")).toDouble(0);
        const double abr     = fmt.value(QStringLiteral("abr")).toDouble(0);

        qint64 filesize = fmt.value(QStringLiteral("filesize")).toInteger(0);
        if (filesize == 0)
            filesize = fmt.value(QStringLiteral("filesize_approx")).toInteger(0);

        const bool isAudioOnly = (vcodec == QStringLiteral("none") || vcodec.isNull()) &&
                                 (acodec != QStringLiteral("none") && !acodec.isNull());
        const bool isVideo     = (vcodec != QStringLiteral("none") && !vcodec.isNull());

        if (!isAudioOnly && !isVideo)
            continue;

        StreamInfo si;
        si.formatId    = fmtId;
        si.ext         = ext;
        si.filesize    = filesize;
        si.fps         = fps;
        si.isAudioOnly = isAudioOnly;
        si.duration    = duration;
        si.title       = title;
        si.thumbnail   = thumbnail;

        if (isAudioOnly) {
            si.resolution = QStringLiteral("audio only");
            si.codec      = acodec.split(QStringLiteral(".")).first();
            si.bitrate    = abr > 0 ? static_cast<int>(abr) : static_cast<int>(tbr);
        } else {
            si.resolution = height > 0 ? QStringLiteral("%1p").arg(height)
                                       : fmt.value(QStringLiteral("format_note")).toString(QStringLiteral("?"));
            si.codec      = vcodec.split(QStringLiteral(".")).first();
            si.bitrate    = static_cast<int>(tbr);
        }

        streams.append(si);
    }

    // Sort: video by height descending, then audio by bitrate descending
    std::sort(streams.begin(), streams.end(), [](const StreamInfo& a, const StreamInfo& b) {
        if (a.isAudioOnly != b.isAudioOnly)
            return !a.isAudioOnly; // videos first

        if (a.isAudioOnly) // both audio
            return a.bitrate > b.bitrate;

        // Both video — compare height
        auto extractHeight = [](const QString& res) -> int {
            QString h = res;
            h.remove(QStringLiteral("p"));
            bool ok;
            int val = h.toInt(&ok);
            return ok ? val : 0;
        };
        return extractHeight(a.resolution) > extractHeight(b.resolution);
    });

    emit metadataReady(streams, title, duration, thumbnail);
}

void FetchWorker::parseProgressLine(const QByteArray& line)
{
    // Our progress template: "download:EZPROG  45.2%  1.5MiB/s  00:30"
    // We look for the EZPROG marker to distinguish our lines from yt-dlp noise.
    const QString str = QString::fromUtf8(line).trimmed();

    if (!str.startsWith(QStringLiteral("EZPROG")))
        return;

    // Remove the prefix and split
    const QString payload = str.mid(QStringLiteral("EZPROG").length()).trimmed();
    const QStringList parts = payload.split(QChar(' '), Qt::SkipEmptyParts);

    if (parts.size() < 3)
        return;

    // Parse percent (e.g. "45.2%" or " 100%")
    QString pctStr = parts[0].trimmed();
    pctStr.remove(QChar('%'));
    bool ok = false;
    double percent = pctStr.toDouble(&ok);
    if (!ok) percent = 0.0;

    const QString speed = parts[1].trimmed();
    const QString eta   = parts[2].trimmed();

    emit downloadProgress(percent, speed, eta);
}
