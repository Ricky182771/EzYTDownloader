#pragma once

#include <QObject>
#include <QStringList>

/// Verifies that required external commands (yt-dlp, ffmpeg) are reachable on
/// the system PATH.  Runs each check via a short-lived QProcess inside
/// QtConcurrent::run so the UI stays responsive.
class DependencyChecker : public QObject {
    Q_OBJECT

public:
    explicit DependencyChecker(QObject* parent = nullptr);

    /// Kick off the background check.  Emits either allFound() or
    /// missingDeps(list) when done.
    void checkAll();

signals:
    void allFound();
    void missingDeps(const QStringList& missing);

private:
    /// Synchronously run `which <cmd>` and return true if exit code == 0.
    static bool isCommandAvailable(const QString& cmd);

    QStringList m_required = {
        QStringLiteral("yt-dlp"),
        QStringLiteral("ffmpeg"),
    };

    QStringList m_optional = {
        QStringLiteral("curl"),
    };
};
