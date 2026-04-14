#pragma once

#include <QString>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace Utils {

/// Resolve the path to fetch.py, checking dev-tree first, then install prefix.
inline QString findPythonScript()
{
    // 1. Dev-mode: <source>/python/fetch.py
    const QString devPath = QStringLiteral(SOURCE_DIR) + QStringLiteral("/python/fetch.py");
    if (QFileInfo::exists(devPath))
        return devPath;

    // 2. Relative to binary: ../python/fetch.py  (in-tree build)
    const QString binDir  = QCoreApplication::applicationDirPath();
    const QString relPath = binDir + QStringLiteral("/../python/fetch.py");
    if (QFileInfo::exists(relPath))
        return QFileInfo(relPath).canonicalFilePath();

    // 3. Installed: <prefix>/share/EzYTDownloader/python/fetch.py
    const QString installed = binDir + QStringLiteral("/../share/EzYTDownloader/python/fetch.py");
    if (QFileInfo::exists(installed))
        return QFileInfo(installed).canonicalFilePath();

    return {};
}

/// Resolve the path to install_deps.sh
inline QString findBashScript()
{
    const QString devPath = QStringLiteral(SOURCE_DIR) + QStringLiteral("/scripts/install_deps.sh");
    if (QFileInfo::exists(devPath))
        return devPath;

    const QString binDir  = QCoreApplication::applicationDirPath();
    const QString relPath = binDir + QStringLiteral("/../scripts/install_deps.sh");
    if (QFileInfo::exists(relPath))
        return QFileInfo(relPath).canonicalFilePath();

    const QString installed = binDir + QStringLiteral("/../share/EzYTDownloader/scripts/install_deps.sh");
    if (QFileInfo::exists(installed))
        return QFileInfo(installed).canonicalFilePath();

    return {};
}

/// Detect the best available terminal emulator on this system.
/// Returns the full path, or empty string if none found.
inline QString detectTerminal()
{
    // Check $TERMINAL env first
    const QByteArray envTerm = qgetenv("TERMINAL");
    if (!envTerm.isEmpty()) {
        const QString path = QStandardPaths::findExecutable(QString::fromLocal8Bit(envTerm));
        if (!path.isEmpty())
            return path;
    }

    // Common terminals in preference order
    static const QStringList candidates = {
        QStringLiteral("konsole"),
        QStringLiteral("gnome-terminal"),
        QStringLiteral("xfce4-terminal"),
        QStringLiteral("mate-terminal"),
        QStringLiteral("lxterminal"),
        QStringLiteral("alacritty"),
        QStringLiteral("kitty"),
        QStringLiteral("x-terminal-emulator"),
        QStringLiteral("xterm"),
    };

    for (const auto& name : candidates) {
        const QString path = QStandardPaths::findExecutable(name);
        if (!path.isEmpty())
            return path;
    }

    return {};
}

/// Return a temporary directory for in-progress downloads.
inline QString tempDownloadDir()
{
    const QString tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                        + QStringLiteral("/EzYTDownloader");
    QDir().mkpath(tmp);
    return tmp;
}

/// Config directory: ~/.config/EzYTDownloader/
inline QString configDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                        + QStringLiteral("/EzYTDownloader");
    QDir().mkpath(dir);
    return dir;
}

} // namespace Utils
