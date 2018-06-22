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

#include <libavcodec/avcodec.h>
#include <libavformat/avio.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>

#include <assert.h>
#include <string.h>

#include <getopt.h>
#include <signal.h>

#define DEFAULT_FRAMERATE 15

static void print_usage_and_exit(const char *cmd);
static char *parse_protocol_name(const char *addr);

static int  filters_init(TranscodeContext *ctx);
static void filters_fini(TranscodeContext *ctx);
static int  init_filters(TranscodeContext *ctx);
static int  add_filter(TranscodeContext *ctx, const char *name);
static int  apply_filters(TranscodeContext *ctx, AVPacket *pkt);
static int  apply_filter(TranscodeContext *ctx, int index, AVPacket *pkt);

static void *av_thread(void *opaque);
static void sigroutine(int signum);

static int aborted = 0;

int main(int argc, char *argv[])
{
  int verbose = 0;

  TranscodeParam param;
  memset(&param, 0, sizeof (param));
  param.framerate = DEFAULT_FRAMERATE;

  struct option long_options[] = {
      { "bitrate",          required_argument, NULL, 'b' },
      { "crf",              required_argument, NULL, 'c' },
      { "video-size",       required_argument, NULL, 's' },
      { "framerate",        required_argument, NULL, 'r' },
      { "package",          no_argument,       NULL, 'p' },
      { "verbose",          no_argument,       NULL, 'v' },
      { NULL,               0,                 NULL, 0   }
  };

  int c = 0;
  while (1)
  {
    int option_index = 0;

    c = getopt_long(argc, argv, "b:c:s:r:pv", long_options, &option_index);
    if (c == -1)
    {
      break;
    }

    switch (c)
    {
    case 'b':
      param.bitrate = atoi(optarg);
      break;

    case 'c':
      param.crf = atoi(optarg);
      break;

    case 's':
      sscanf(optarg, "%dx%d", &param.width, &param.height);
      break;

    case 'r':
      param.framerate = atoi(optarg);
      break;

    case 'p':
      param.package = 1;
      break;

    case 'v':
      verbose = 1;
      break;

    default:
      print_usage_and_exit(argv[0]);
      break;
    }
  }

  if (argc - optind != 1)
  {
    print_usage_and_exit(argv[0]);
  }

  char *output = argv[optind];

  filter_register_all();

  char names[BUFSIZ];
  char *oname = parse_protocol_name(output);
  if (oname == NULL)
  {
    fprintf(stderr, "invalid output.\n");
    free(oname);
    exit(-1);
  }

  signal(SIGINT, sigroutine);
  signal(SIGTERM, sigroutine);

  TranscodeContext video = {
    .type = ST_VIDEO,
    .param = param,
    .output = output
  };

  int rc = 0;
  snprintf(names, BUFSIZ, "cap:repeat:av:%s:%s",
           verbose ? "stat" : "",
           oname);
  video.filter_graph = strdup(names);
  if (verbose)
  {
    fprintf(stderr, "use video filters '%s'.\n", names);
  }

  rc = init_filters(&video); assert(rc >= 0);
  pthread_create(&video.thread, NULL, av_thread, &video);
  pthread_join(video.thread, NULL);

  free(video.filter_graph);

  fprintf(stderr, "\nCompleted.\n");

  return 0;
}

void print_usage_and_exit(const char *cmd)
{
  fprintf(stderr, "\
Usage: %s [OPTION] OUTPUT\n\
  -b, --bitrate=BITRATE         video bitrate (h264)\n\
  -c, --crf=CRF                 video crf (0 - 53) [23] (h264)\n\
  -s, --video-size=WxH          video size (WxH)\n\
  -r, --framerate=RATE          video framerate [15]\n\
  -p, --package                 package output\n\
      --verbose                 verbose output\n", cmd);
  exit(-1);
}

char *parse_protocol_name(const char *addr)
{
  char *str = strdup(addr);
  char *protocol = strtok(str, ":");
  if (protocol == NULL)
  {
    free(str);
  }

  return protocol;
}

int filters_init(TranscodeContext *ctx)
{
  int ret = -1;
  int type = FT_FILTER;

  for (int i = 0; i < ctx->nb_filters; i++)
  {
    Filter *filter = ctx->filters[i];
    // Filter context
    ctx->priv_data = ctx->filter_data[i] = av_mallocz(filter->priv_data_size);
    if (i == 0)
    {
      type = FT_INPUT;
    }
    else if (i == ctx->nb_filters - 1)
    {
      type = FT_OUTPUT;
    }
    else
    {
      type = FT_FILTER;
    }
    ret = (filter->init != NULL) ? filter->init(ctx, type) : 0;
    if (ret < 0)
    {
      // cleanup
      for (int j = 0; j < i; j++)
      {
        filter = ctx->filters[j];
        ctx->priv_data = ctx->filter_data[i];
        if (filter->fini != NULL) filter->fini(ctx);
        av_freep(ctx->filter_data + j);
      }

      break;
    }
  }
  ctx->priv_data = NULL;

  return ret;
}

void filters_fini(TranscodeContext *ctx)
{

  for (int i = 0; i < ctx->nb_filters; i++)
  {
    Filter *filter = ctx->filters[i];
    ctx->priv_data = ctx->filter_data[i];
    if (filter->fini != NULL) filter->fini(ctx);
    av_freep(ctx->filter_data + i);
  }
}

int init_filters(TranscodeContext *ctx)
{
  int ret = -1;

  memset(ctx->filters, 0, sizeof (ctx->filters));
  ctx->nb_filters = 0;
  memset(ctx->filter_data, 0, sizeof (ctx->filter_data));
  ctx->priv_data = NULL;

  char *name = NULL;
  char *names = strdup(ctx->filter_graph);
  for (name = strtok(names, ":"); name != NULL; name = strtok(NULL, ":"))
  {
    ret = add_filter(ctx, name);
    if (ret < 0)
    {
      break;
    }
  }
  free(names);

  return ret;
}

int add_filter(TranscodeContext *ctx, const char *name)
{
  if (ctx->nb_filters == MAX_FILTERS) return -1;

  Filter *filter = find_filter(name); assert(filter != NULL);
  ctx->filters[ctx->nb_filters] = filter;
  ctx->nb_filters++;

  return 0;
}

int apply_filters(TranscodeContext *ctx, AVPacket *pkt)
{
  int ret = -1;

  for (int i = 0; i < ctx->nb_filters; i++)
  {
    ret = apply_filter(ctx, i, pkt);
    if (ret < 0 && ret != AVERROR(EAGAIN))
    {
      break;
    }
  }

  return ret;
}

int apply_filter(TranscodeContext *ctx, int index, AVPacket *pkt)
{
  int ret = -1;

  Filter *filter = ctx->filters[index];
  ctx->priv_data = ctx->filter_data[index];
  ret = filter->apply(ctx, pkt);
  ctx->priv_data = NULL;

  return ret;
}

void *av_thread(void *opaque)
{
  TranscodeContext *ctx = (TranscodeContext *) opaque;

  int ret = filters_init(ctx); assert(ret >= 0);

  AVPacket pkt;
  pkt.data = NULL;
  pkt.size = 0;
  av_init_packet(&pkt);
  while (!aborted)
  {
    ret = apply_filters(ctx, &pkt);
    if (ret == AVERROR(EAGAIN))
    {
      continue;
    }
    else if (ret < 0)
    {
      break;
    }
  }

  filters_fini(ctx);

  aborted = 1;

  return NULL;
}

void sigroutine(int signum)
{
  (void) signum;

  aborted = 1;
}
