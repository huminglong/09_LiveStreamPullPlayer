#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H

#include <QMetaType>

struct PlayerStats
{
    int videoQueueSize = 0;
    int audioQueueSize = 0;
    double incomingBitrateKbps = 0.0;
    double jitterBufferMs = 0.0;
};

Q_DECLARE_METATYPE(PlayerStats)

#endif // PLAYERSTATS_H
