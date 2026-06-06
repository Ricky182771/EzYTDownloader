#pragma once

#include <QString>

struct PlaylistEntry {
    int     index    = 0;   // 1-based position in playlist
    QString id;
    QString url;
    QString title;
    QString thumbnail;
    double  duration = 0.0;
};
