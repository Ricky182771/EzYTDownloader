#include "FetchWorker.h"
#include "Utils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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
    startProcess({QStringLiteral("--info"), url});
}

void FetchWorker::downloadStream(const QString& url, const QString& formatId,
                                  const QString& outputPath, bool mergeAudio)
{
    QStringList args = {
        QStringLiteral("--download"), url,
        QStringLiteral("--format"),   formatId,
        QStringLiteral("--output"),   outputPath,
    };

    if (mergeAudio)
        args << QStringLiteral("--merge-audio");

    startProcess(args);
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

void FetchWorker::startProcess(const QStringList& args)
{
    // Fully clean up any prior process — disconnect its signals so a delayed
    // 'finished' from the old process can never touch the new one.
    cancel();

    const QString script = Utils::findPythonScript();
    if (script.isEmpty()) {
        emit errorOccurred(tr("Cannot find fetch.py script."));
        return;
    }

    m_cancelled    = false;
    m_stdoutBuffer.clear();

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

    QStringList fullArgs;
    fullArgs << script << args;

    qDebug() << "[FetchWorker] Starting: python3" << fullArgs;
    m_process->start(QStringLiteral("python3"), fullArgs);

    if (!m_process->waitForStarted(5000)) {
        emit errorOccurred(tr("Failed to start python3 process."));
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void FetchWorker::onReadyReadStdout()
{
    if (!m_process) return;
    m_stdoutBuffer += m_process->readAllStandardOutput();

    // Process complete lines — split on both \n and \r since yt-dlp
    // may use \r for its native progress even with quiet=True.
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
            parseJsonLine(line);
    }
}

void FetchWorker::onReadyReadStderr()
{
    if (!m_process) return;
    const QByteArray err = m_process->readAllStandardError();
    if (!err.trimmed().isEmpty())
        qWarning() << "[FetchWorker] stderr:" << err.trimmed();
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

    // Process any remaining data in buffer
    m_stdoutBuffer += m_process->readAllStandardOutput();
    while (true) {
        const int idx = m_stdoutBuffer.indexOf('\n');
        if (idx < 0)
            break;
        const QByteArray line = m_stdoutBuffer.left(idx).trimmed();
        m_stdoutBuffer.remove(0, idx + 1);
        if (!line.isEmpty())
            parseJsonLine(line);
    }
    // Also try the remaining partial line
    if (!m_stdoutBuffer.trimmed().isEmpty())
        parseJsonLine(m_stdoutBuffer.trimmed());
    m_stdoutBuffer.clear();

    // Clean up the process before emitting any signals, so that if a slot
    // calls startProcess() again we don't have a stale m_process pointer.
    m_process->deleteLater();
    m_process = nullptr;

    if (m_cancelled)
        return; // user-initiated cancel, no error

    if (status == QProcess::CrashExit || exitCode != 0) {
        emit errorOccurred(tr("Python process exited with code %1").arg(exitCode));
    }
}

void FetchWorker::parseJsonLine(const QByteArray& line)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[FetchWorker] Bad JSON:" << line;
        return;
    }

    const QJsonObject obj  = doc.object();
    const QString     type = obj.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("metadata")) {
        const QString title     = obj.value(QStringLiteral("title")).toString();
        const double  duration  = obj.value(QStringLiteral("duration")).toDouble();
        const QString thumbnail = obj.value(QStringLiteral("thumbnail")).toString();
        const QJsonArray arr    = obj.value(QStringLiteral("streams")).toArray();

        QList<StreamInfo> streams;
        for (const auto& val : arr) {
            const QJsonObject s = val.toObject();
            StreamInfo si;
            si.formatId    = s.value(QStringLiteral("formatId")).toString();
            si.resolution  = s.value(QStringLiteral("resolution")).toString();
            si.ext         = s.value(QStringLiteral("ext")).toString();
            si.filesize    = s.value(QStringLiteral("filesize")).toInteger();
            si.bitrate     = s.value(QStringLiteral("bitrate")).toInt();
            si.fps         = s.value(QStringLiteral("fps")).toInt();
            si.codec       = s.value(QStringLiteral("codec")).toString();
            si.isAudioOnly = s.value(QStringLiteral("isAudioOnly")).toBool();
            si.duration    = duration;
            si.title       = title;
            si.thumbnail   = thumbnail;
            streams.append(si);
        }

        emit metadataReady(streams, title, duration, thumbnail);

    } else if (type == QStringLiteral("progress")) {
        const double  pct   = obj.value(QStringLiteral("percent")).toDouble();
        const QString speed = obj.value(QStringLiteral("speed")).toString();
        const QString eta   = obj.value(QStringLiteral("eta")).toString();
        emit downloadProgress(pct, speed, eta);

    } else if (type == QStringLiteral("complete")) {
        const QString path = obj.value(QStringLiteral("path")).toString();
        emit downloadFinished(path);

    } else if (type == QStringLiteral("error")) {
        const QString msg = obj.value(QStringLiteral("message")).toString();
        emit errorOccurred(msg);
    }
}
