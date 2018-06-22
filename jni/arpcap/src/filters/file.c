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

typedef struct {
  FILE *fp;
} FileContext;

typedef struct {
  FILE *fp;
  int ref;
} FileRef;

static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

static FileRef file_ref = { NULL, 0 };

static int file_init(TranscodeContext *ctx, int type)
{
  assert(type == FT_OUTPUT);

  FileContext *file = (FileContext *) ctx->priv_data;

  char path[PATH_MAX];
  sscanf(ctx->output, "file://%s", path);

  pthread_mutex_lock(&file_mutex);
  if (file_ref.fp == NULL)
  {
    file_ref.fp = fopen(path, "wb"); assert(file_ref.fp != NULL);
  }
  file->fp = file_ref.fp;
  file_ref.ref++;
  pthread_mutex_unlock(&file_mutex);

  return 0;
}

static int file_fini(TranscodeContext *ctx)
{
  FileContext *file = (FileContext *) ctx->priv_data;

  pthread_mutex_lock(&file_mutex);
  file->fp = NULL;
  file_ref.ref--;
  if (file_ref.ref == 0)
  {
    fclose(file_ref.fp);
    file_ref.fp = NULL;
  }
  pthread_mutex_unlock(&file_mutex);

  return 0;
}

static int file_apply(TranscodeContext *ctx, AVPacket *pkt)
{
  FileContext *file = (FileContext *) ctx->priv_data;

  if (pkt != NULL && pkt->data != NULL)
  {
    pthread_mutex_lock(&file_mutex);
    int extradata_size = 0;
    uint8_t *extradata = av_packet_get_side_data(pkt,
                                                 AV_PKT_DATA_NEW_EXTRADATA,
                                                 &extradata_size);
    if (extradata != NULL)
    {
      fwrite(extradata, extradata_size, 1, file->fp);
    }
    fwrite(pkt->data, pkt->size, 1, file->fp);
    fflush(file->fp);
    pthread_mutex_unlock(&file_mutex);

    av_packet_unref(pkt);

    return 0;
  }
  else
  {
    return AVERROR(EAGAIN);
  }
}

Filter file_filter = {
  .name = "file",
  .priv_data_size = sizeof (FileContext),
  .init = file_init,
  .fini = file_fini,
  .apply = file_apply
};