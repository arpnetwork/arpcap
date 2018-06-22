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

#ifndef ARP_TRANSCODE_H_
#define ARP_TRANSCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <filter.h>
#include <utils.h>

#include <libavformat/avformat.h>

#include <pthread.h>

#define MAX_FILTERS   8

enum StreamType
{
  ST_VIDEO = 0,
  ST_AUDIO,
};

enum CodecID
{
  CODEC_H264 = 0,
  CODEC_AAC
};

typedef struct TranscodeParam {
  int width;
  int height;
  enum CodecID codec;
  int crf;
  int bitrate;
  int framerate;
  int package;
} TranscodeParam;

typedef struct TranscodeContext {
  int type;

  TranscodeParam param;

  char *output;

  char *filter_graph;

  Filter *filters[MAX_FILTERS];
  int nb_filters;

  void *filter_data[MAX_FILTERS];
  void *priv_data;

  pthread_t thread;
} TranscodeContext;

#ifdef __cplusplus
}
#endif

#endif  // ARP_TRANSCODE_H_
