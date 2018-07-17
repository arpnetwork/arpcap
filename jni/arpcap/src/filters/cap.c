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

#include <cap.h>
#include <transcode.h>

#include <libavutil/imgutils.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <assert.h>

typedef struct {
  Cap *cap;
} CapContext;

static int cap_init(TranscodeContext *ctx, int type)
{
  assert(type == FT_INPUT);
  assert(ctx->type == ST_VIDEO);

  CapContext *cap = (CapContext *) ctx->priv_data;

  cap->cap = cap_open(
    ctx->param.top, ctx->param.bottom,
    ctx->param.width, ctx->param.height, ctx->param.framerate);

  return 0;
}

static int cap_fini(TranscodeContext *ctx)
{
  CapContext *cap = (CapContext *) ctx->priv_data;

  cap_close(cap->cap);

  return 0;
}

static int cap_apply(TranscodeContext *ctx, AVPacket *pkt)
{
  assert(pkt != NULL && pkt->data == NULL);

  CapContext *cap = (CapContext *) ctx->priv_data;
  return cap_read(cap->cap, pkt);
}

Filter cap_filter = {
  .name = "cap",
  .priv_data_size = sizeof (CapContext),
  .init = cap_init,
  .fini = cap_fini,
  .apply = cap_apply
};
