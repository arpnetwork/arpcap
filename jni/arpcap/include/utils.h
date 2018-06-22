/*
 * Copyright 2018 ARP Network
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ARP_UTILS_H_
#define ARP_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#define PKT_MKSIZE(pkt, w, h) \
  do  \
  { \
    (pkt)->pos = ((int64_t) (w) << 32) | (h); \
  } while (0)
#define PKT_WIDTH(pkt)        ((pkt)->pos >> 32)
#define PKT_HEIGHT(pkt)       ((pkt)->pos & 0xFFFF)

int new_packet_from_data(AVPacket *pkt, uint8_t *data, int size);
int new_packet_from_frame(AVPacket *pkt, AVFrame *frame);
int new_frame_from_packet(AVFrame *frame, AVPacket *pkt);

#ifdef __cplusplus
}
#endif

#endif  // ARP_UTILS_H_
