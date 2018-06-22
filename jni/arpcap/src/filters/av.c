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

#include <libavutil/opt.h>

#include <assert.h>

typedef struct {
  AVCodecContext *codec;
  int64_t next_pts;
} AVContext;

static AVCodecContext *open_encoder(
    int type, TranscodeParam *param, int width, int height);
static AVCodecContext *open_h264_encoder(
    int width, int height, TranscodeParam *param, int fmp4);

static int av_fini(TranscodeContext *ctx)
{
  AVContext *av = (AVContext *) ctx->priv_data;

  if (av->codec != NULL)
  {
    avcodec_close(av->codec);
  }

  return 0;
}

static int av_apply(TranscodeContext *ctx, AVPacket *pkt)
{
  AVContext *av = (AVContext *) ctx->priv_data;

  if (pkt == NULL || pkt->data == NULL) return AVERROR(EAGAIN);

  AVFrame *frame = av_frame_alloc();
  new_frame_from_packet(frame, pkt);

  if (av->codec == NULL)
  {
    av->codec = open_encoder(ctx->type, &ctx->param, frame->width, frame->height);
    assert(av->codec != NULL);
    av->next_pts = 0;
  }

  frame->pts = av->next_pts;
  int ret = avcodec_send_frame(av->codec, frame);
  if (ret >= 0)
  {
    ret = avcodec_receive_packet(av->codec, pkt);
    if (ret >= 0)
    {
      pkt->stream_index = ctx->type;
      PKT_MKSIZE(pkt, frame->width, frame->height);
      if (pkt->duration == 0)
      {
        pkt->duration = 1000;
      }

      if (av->next_pts == 0 && av->codec->extradata_size > 0)
      {
        fprintf(stderr, "Found %d bytes extradata.\n", av->codec->extradata_size);
        uint8_t *extradata = av_packet_new_side_data(pkt,
                                                    AV_PKT_DATA_NEW_EXTRADATA,
                                                    av->codec->extradata_size);
        memcpy(extradata, av->codec->extradata, av->codec->extradata_size);
      }

      av->next_pts += (pkt->duration > 0) ? pkt->duration : 1000;
    }
  }

  av_frame_free(&frame);

  return ret;
}

AVCodecContext *open_encoder(int type, TranscodeParam *param, int width, int height)
{
  AVCodecContext *codec = NULL;

  if (param->codec == CODEC_H264) {
    codec = open_h264_encoder(width, height, param, 0);
  }

  return codec;
}

AVCodecContext *open_h264_encoder(
    int width, int height, TranscodeParam *param, int fmp4)
{
  AVCodec *codec = NULL;
  AVCodecContext *ctx = NULL;
  int ret = 0;

  codec = avcodec_find_encoder(AV_CODEC_ID_H264); assert(codec != NULL);
  ctx = avcodec_alloc_context3(codec); assert(ctx != NULL);

  ctx->width = width;
  ctx->height = height;
  ctx->gop_size = 100;
  ctx->max_b_frames = 0;
  if (param->bitrate > 0)
  {
    ctx->rc_buffer_size = param->bitrate * 1000;
    ctx->rc_max_rate = param->bitrate * 1000;
  }
  ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  if (fmp4)
  {
    ctx->framerate = av_make_q(1, param->framerate + 1);
  }
  else
  {
    ctx->framerate = av_make_q(1, param->framerate);
  }
  ctx->time_base = av_mul_q(ctx->framerate, av_make_q(1, 1000));
  ctx->ticks_per_frame = 1000;
  if (fmp4)
  {
    ctx->gop_size = 1;
    ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  av_opt_set(ctx->priv_data, "profile", "high", 0);
  av_opt_set(ctx->priv_data, "level", "5.2", 0);
  av_opt_set(ctx->priv_data, "preset", "veryfast", 0);
  av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
  if (param->crf > 0)
  {
    char crf[BUFSIZ];
    snprintf(crf, BUFSIZ, "%d", param->crf);
    av_opt_set(ctx->priv_data, "crf", crf, 0);
  }

  ret = avcodec_open2(ctx, codec, NULL); assert(ret >= 0);

  return ctx;
}

Filter av_filter = {
  .name = "av",
  .priv_data_size = sizeof (AVContext),
  .init = NULL,
  .fini = av_fini,
  .apply = av_apply
};
