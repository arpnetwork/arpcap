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

#include <file.h>
#include <transcode.h>

#include <assert.h>
#include <unistd.h>

typedef FileContext PipeContext;

static int pipe_init(TranscodeContext *ctx, int type)
{
  assert(type == FT_OUTPUT);

  PipeContext *pipe = (PipeContext *) ctx->priv_data;
  sscanf(ctx->output, "pipe://%d", &pipe->fd);
  pipe->package = ctx->param.package;

  if (pipe->fd == STDOUT_FILENO)
  {
    setbuf(stdout, NULL);
  }

  return 0;
}

static int pipe_apply(TranscodeContext *ctx, AVPacket *pkt)
{
  PipeContext *pipe = (PipeContext *) ctx->priv_data;

  if (pkt != NULL && pkt->data != NULL)
  {
    int extradata_size = 0;
    uint8_t *extradata = av_packet_get_side_data(pkt,
                                                 AV_PKT_DATA_NEW_EXTRADATA,
                                                 &extradata_size);
    if (extradata != NULL)
    {
      write_data(pipe, extradata, extradata_size);
    }
    write_data(pipe, pkt->data, pkt->size);

    av_packet_unref(pkt);

    return 0;
  }
  else
  {
    return AVERROR(EAGAIN);
  }
}

Filter pipe_filter = {
  .name = "pipe",
  .priv_data_size = sizeof (PipeContext),
  .init = pipe_init,
  .fini = NULL,
  .apply = pipe_apply
};
