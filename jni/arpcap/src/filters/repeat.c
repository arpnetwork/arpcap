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

#include <libavutil/time.h>

#include <assert.h>

#define REPEAT_INTERVAL 80000

typedef struct {
  AVPacket *pkt;

  int64_t interval;
  int64_t last_ts;
} RepeatContext;

static int repeat_init(TranscodeContext *ctx, int type)
{
  (void) type;

  RepeatContext *repeat = (RepeatContext *) ctx->priv_data;
  repeat->interval = FFMAX(1000000 / ctx->param.framerate * 2, REPEAT_INTERVAL);

  return 0;
}

static int repeat_fini(TranscodeContext *ctx)
{
  RepeatContext *repeat = (RepeatContext *) ctx->priv_data;

  av_packet_free(&repeat->pkt);

  return 0;
}

static int repeat_apply(TranscodeContext *ctx, AVPacket *pkt)
{
  RepeatContext *repeat = (RepeatContext *) ctx->priv_data;

  int64_t now = av_gettime();
  if (pkt != NULL && pkt->data != NULL)
  {
    if (repeat->pkt != NULL) av_packet_free(&repeat->pkt);

    repeat->pkt = av_packet_clone(pkt);
    repeat->last_ts = now;

    return 0;
  }
  else
  {
    if (repeat->pkt != NULL && pkt != NULL &&
        now - repeat->last_ts >= repeat->interval)
    {
      av_packet_ref(pkt, repeat->pkt);
      repeat->last_ts = now;

      return 0;
    }
    else
    {
      return AVERROR(EAGAIN);
    }
  }
}

Filter repeat_filter = {
  .name = "repeat",
  .priv_data_size = sizeof (RepeatContext),
  .init = repeat_init,
  .fini = repeat_fini,
  .apply = repeat_apply
};
