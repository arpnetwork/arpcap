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

#include "utils.h"

#include <libavutil/imgutils.h>

#include <assert.h>
#include <unistd.h>

static int new_frame_from_data(AVFrame *frame, uint8_t *data, int size);

int new_packet_from_data(AVPacket *pkt, uint8_t *data, int size)
{
  assert(pkt != NULL && pkt->data == NULL);
  assert(data != NULL && size > 0);

  av_new_packet(pkt, size);
  memcpy(pkt->data, data, size);

  return size;
}

int new_packet_from_frame(AVPacket *pkt, AVFrame *frame)
{
  int size = av_image_get_buffer_size(frame->format, frame->width, frame->height, 1);
  av_new_packet(pkt, size);
  av_image_copy_to_buffer(pkt->data, size,
                          (const uint8_t * const*) frame->data, frame->linesize,
                          frame->format, frame->width, frame->height, 1);

  return 0;
}

int new_frame_from_packet(AVFrame *frame, AVPacket *pkt)
{
  if (pkt->stream_index == AVMEDIA_TYPE_VIDEO) // Video
  {
    frame->width  = PKT_WIDTH(pkt);
    frame->height = PKT_HEIGHT(pkt);
    frame->format = AV_PIX_FMT_YUV420P;
  }
  else  // Audio
  {
    frame->channels = 2;
    frame->channel_layout = av_get_default_channel_layout(frame->channels);
    frame->format = AV_SAMPLE_FMT_S16;
    frame->sample_rate = 44100;
    frame->nb_samples =
        pkt->size / (av_get_bytes_per_sample(frame->format) * frame->channels);
  }
  new_frame_from_data(frame, pkt->data, pkt->size);
  av_packet_unref(pkt);

  return 0;
}

int new_frame_from_data(AVFrame *frame, uint8_t *data, int size)
{
  (void) size;

  av_frame_get_buffer(frame, 32);

  uint8_t *src_data[4];
  int src_linesize[4];
  if (frame->nb_samples == 0) // Video
  {
    av_image_fill_arrays(src_data, src_linesize, data,
                         frame->format, frame->width, frame->height, 4);
    av_image_copy(frame->data, frame->linesize,
                  (const uint8_t **) src_data, src_linesize,
                  frame->format, frame->width, frame->height);
  }
  else  // Audio
  {
    av_samples_fill_arrays(src_data, src_linesize, data,
                           frame->channels, frame->nb_samples, frame->format, 4);
    av_samples_copy(frame->data, src_data,
                    0, 0, frame->nb_samples, frame->channels, frame->format);
  }

  return 0;
}
