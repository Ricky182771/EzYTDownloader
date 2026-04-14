#pragma once

#include <QString>

/// Holds metadata for a single available stream from a YouTube video.
struct StreamInfo {
    QString formatId;       // yt-dlp format id, e.g. "137"
    QString resolution;     // "1080p", "720p", "480p", "audio only"
    QString ext;            // Original extension: "webm", "mp4", …
    qint64  filesize  = 0;  // Approximate size in bytes
    int     bitrate   = 0;  // kbps
    int     fps       = 0;  // Frames per second (0 = unknown)
    QString codec;          // "avc1", "vp9", "opus", …
    bool    isAudioOnly = false;
    double  duration  = 0.0;// Seconds
    QString title;          // Video title
    QString thumbnail;      // Thumbnail URL

    /// Human-readable file size string
    static QString humanSize(qint64 bytes) {
        if (bytes <= 0) return QString();
        if (bytes < 1024)
            return QStringLiteral("%1 B").arg(bytes);
        if (bytes < 1024 * 1024)
            return QStringLiteral("%1 KB").arg(bytes / 1024);
        const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        if (mb < 1024.0)
            return QStringLiteral("%1 MB").arg(mb, 0, 'f', 1);
        const double gb = mb / 1024.0;
        return QStringLiteral("%1 GB").arg(gb, 0, 'f', 2);
    }

    /// Pretty label for combo box display
    QString label() const {
        if (isAudioOnly) {
            // e.g. "audio only  •  128 kbps  •  opus  •  3.3 MB"
            const QString sizeStr = filesize > 0
                ? QStringLiteral("  •  ~%1").arg(humanSize(filesize))
                : QString();
            return QStringLiteral("%1  •  %2 kbps  •  %3%4")
                .arg(resolution, QString::number(bitrate), codec, sizeStr);
        }

        // Video: "1080p 60  •  avc1  •  ~77.2 MB"  or  "720p  •  vp9  •  ~16.9 MB"
        QString res = resolution;
        if (fps > 30)
            res += QStringLiteral(" %1").arg(fps);

        const QString sizeStr = filesize > 0
            ? QStringLiteral("  •  ~%1").arg(humanSize(filesize))
            : QString();
        return QStringLiteral("%1  •  %2%3")
            .arg(res, codec, sizeStr);
    }
};
