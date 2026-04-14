#include "DependencyChecker.h"

#include <QProcess>
#include <QtConcurrent>
#include <QDebug>

DependencyChecker::DependencyChecker(QObject* parent)
    : QObject(parent)
{}

void DependencyChecker::checkAll()
{
    // Run all checks in a thread-pool thread so the UI stays responsive.
    // We capture `this` safely because the future is fire-and-forget and the
    // signals are delivered on the receiver's thread via Qt::AutoConnection.
    [[maybe_unused]] auto future = QtConcurrent::run([this]() {
        QStringList missing;

        for (const auto& cmd : m_required) {
            if (!isCommandAvailable(cmd)) {
                qWarning() << "[DepCheck] MISSING required:" << cmd;
                missing << cmd;
            } else {
                qDebug() << "[DepCheck] Found:" << cmd;
            }
        }

        for (const auto& cmd : m_optional) {
            if (!isCommandAvailable(cmd))
                qWarning() << "[DepCheck] MISSING optional:" << cmd;
            else
                qDebug() << "[DepCheck] Found optional:" << cmd;
        }

        if (missing.isEmpty())
            emit allFound();
        else
            emit missingDeps(missing);
    });
}

bool DependencyChecker::isCommandAvailable(const QString& cmd)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(QStringLiteral("which"), {cmd});

    if (!proc.waitForFinished(3000))
        return false;

    return proc.exitCode() == 0;
}
