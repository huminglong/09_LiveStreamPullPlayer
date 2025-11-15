/**
 * @file playerstats.h
 * @brief 定义播放器统计信息结构体。
 * @mainfunctions
 *   - 无
 * @mainclasses
 *   - PlayerStats
 */

#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H

#include <QMetaType>

 /**
  * @brief PlayerStats 描述当前缓冲、码率与抖动数据。
  */
struct PlayerStats {
  int videoQueueSize = 0;
  int audioQueueSize = 0;
  double incomingBitrateKbps = 0.0;
  double jitterBufferMs = 0.0;
  int droppedVideoFrames = 0;  // 累计丢弃的视频包数
};

Q_DECLARE_METATYPE(PlayerStats)

#endif // PLAYERSTATS_H
