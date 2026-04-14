#include "SettingsManager.h"
#include "Utils.h"

#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
SettingsManager& SettingsManager::instance()
{
    static SettingsManager s;
    return s;
}

SettingsManager::SettingsManager()
{
    m_configPath = Utils::configDir() + QStringLiteral("/config.json");
    load();
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistence
// ─────────────────────────────────────────────────────────────────────────────
void SettingsManager::load()
{
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[Settings] No config file found, using defaults.";
        return;
    }

    const QByteArray raw = file.readAll();
    file.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[Settings] JSON parse error:" << err.errorString();
        return;
    }

    m_data = doc.object();
    qDebug() << "[Settings] Loaded config from" << m_configPath;
}

void SettingsManager::save()
{
    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[Settings] Could not write config:" << m_configPath;
        return;
    }

    file.write(QJsonDocument(m_data).toJson(QJsonDocument::Indented));
    file.close();
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────
QString SettingsManager::lastDirectory() const
{
    return m_data.value(QStringLiteral("lastDirectory"))
        .toString(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
}

QString SettingsManager::lastFormat() const
{
    return m_data.value(QStringLiteral("lastFormat")).toString(QStringLiteral("mp4"));
}

QString SettingsManager::lastResolution() const
{
    return m_data.value(QStringLiteral("lastResolution")).toString(QStringLiteral("1080p"));
}

QString SettingsManager::lastBitrate() const
{
    return m_data.value(QStringLiteral("lastBitrate")).toString(QStringLiteral("320k"));
}

QSize SettingsManager::windowSize() const
{
    const int w = m_data.value(QStringLiteral("windowWidth")).toInt(680);
    const int h = m_data.value(QStringLiteral("windowHeight")).toInt(580);
    return {w, h};
}

QPoint SettingsManager::windowPos() const
{
    const int x = m_data.value(QStringLiteral("windowX")).toInt(-1);
    const int y = m_data.value(QStringLiteral("windowY")).toInt(-1);
    return {x, y};
}

// ─────────────────────────────────────────────────────────────────────────────
// Setters
// ─────────────────────────────────────────────────────────────────────────────
void SettingsManager::setLastDirectory(const QString& dir)
{
    m_data[QStringLiteral("lastDirectory")] = dir;
}

void SettingsManager::setLastFormat(const QString& fmt)
{
    m_data[QStringLiteral("lastFormat")] = fmt;
}

void SettingsManager::setLastResolution(const QString& res)
{
    m_data[QStringLiteral("lastResolution")] = res;
}

void SettingsManager::setLastBitrate(const QString& br)
{
    m_data[QStringLiteral("lastBitrate")] = br;
}

void SettingsManager::setWindowSize(const QSize& size)
{
    m_data[QStringLiteral("windowWidth")]  = size.width();
    m_data[QStringLiteral("windowHeight")] = size.height();
}

void SettingsManager::setWindowPos(const QPoint& pos)
{
    m_data[QStringLiteral("windowX")] = pos.x();
    m_data[QStringLiteral("windowY")] = pos.y();
}
