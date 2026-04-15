#pragma once

#include <QString>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace Utils {

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
