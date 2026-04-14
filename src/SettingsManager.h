#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QSize>
#include <QPoint>

/// Manages application settings persisted as JSON in ~/.config/EzYTDownloader/config.json
class SettingsManager : public QObject {
    Q_OBJECT

public:
    static SettingsManager& instance();

    void load();
    void save();

    // ── Getters ──────────────────────────────────────────────────────────
    QString lastDirectory()  const;
    QString lastFormat()     const;   // "mp4", "mkv", "mp3"
    QString lastResolution() const;
    QString lastBitrate()    const;   // "128k", "192k", "256k", "320k"
    QSize   windowSize()     const;
    QPoint  windowPos()      const;

    // ── Setters ──────────────────────────────────────────────────────────
    void setLastDirectory(const QString& dir);
    void setLastFormat(const QString& fmt);
    void setLastResolution(const QString& res);
    void setLastBitrate(const QString& br);
    void setWindowSize(const QSize& size);
    void setWindowPos(const QPoint& pos);

private:
    SettingsManager();
    ~SettingsManager() override = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QString     m_configPath;
    QJsonObject m_data;
};
