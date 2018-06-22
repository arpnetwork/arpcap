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

#include <transcode.h>

#include <assert.h>

#include <libavformat/avformat.h>
#include <libavutil/time.h>

#define INFO_INTERVAL 1000 * 1000

typedef struct {
  int fps;

  int frames;
  int64_t total_size;
  int64_t i_total_size;
  int64_t pts;
  int64_t last;

  int us, secs, mins;
  double bitrate;
  double i_bitrate;
  double max_bitrate;
} StatContext;

static void stat_info(StatContext *stat, AVPacket *pkt);

static int stat_init(TranscodeContext *ctx, int type)
{
  assert(type == FT_FILTER);

  StatContext *stat = (StatContext *) ctx->priv_data;

  stat->fps = ctx->param.framerate;
  stat->bitrate = 0.0;
  stat->i_bitrate = 0.0;
  stat->max_bitrate = 0.0;

  return 0;
}

static int stat_fini(TranscodeContext *ctx)
{
  StatContext *stat = (StatContext *) ctx->priv_data;

  stat_info(stat, NULL);

  return 0;
}

static int stat_apply(TranscodeContext *ctx, AVPacket *pkt)
{
  StatContext *stat = (StatContext *) ctx->priv_data;

  if (pkt != NULL && pkt->data != NULL)
  {
    stat_info(stat, pkt);

    return 0;
  }
  else
  {
    return AVERROR(EAGAIN);
  }
}

static void stat_info(StatContext *stat, AVPacket *pkt)
{
  if (pkt != NULL)
  {
    stat->frames++;
    stat->total_size += pkt->size;
    stat->i_total_size += pkt->size;
    if (pkt->pts == AV_NOPTS_VALUE)
    {
      pkt->pts = stat->frames * 1000;
    }
    stat->pts = av_rescale_q(pkt->pts, av_make_q(1, stat->fps * 1000), AV_TIME_BASE_Q);
  }

  int64_t now = av_gettime();
  if (stat->last == 0)
  {
    stat->last = now;
  }
  else if (pkt == NULL || now - stat->last >= INFO_INTERVAL)
  {
    stat->secs = FFABS(stat->pts) / AV_TIME_BASE;
    stat->us = FFABS(stat->pts) % AV_TIME_BASE;
    stat->mins = stat->secs / 60;
    stat->secs %= 60;
    stat->mins %= 60;
    stat->bitrate = stat->pts > 0 ? stat->total_size * 8 / (stat->pts / 1000.0) : -1;
    stat->i_bitrate = stat->i_total_size * 8 / ((now - stat->last) / 1000.0);
    if (stat->i_bitrate > stat->max_bitrate)
    {
       stat->max_bitrate = stat->i_bitrate;
    }

    fprintf(stderr, "\rf=%5d s=%6.2fMB t=%02d:%02d.%02d b=%7.1fkb/s %6.1f(%6.1f)kb/s",
            stat->frames, stat->total_size / 1024.0 / 1024.0,
            stat->mins, stat->secs, (100 * stat->us) / AV_TIME_BASE,
            stat->bitrate, stat->i_bitrate, stat->max_bitrate);

    stat->i_total_size = 0;
    stat->last = now;
  }
}

Filter stat_filter = {
  .name = "stat",
  .priv_data_size = sizeof (StatContext),
  .init = stat_init,
  .fini = stat_fini,
  .apply = stat_apply
};
