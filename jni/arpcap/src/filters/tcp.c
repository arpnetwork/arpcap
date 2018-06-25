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
#include <strings.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

typedef struct {
  int fd;
  int package;
} TcpContext;

static ssize_t write_data(TcpContext *tcp, const void *buf, size_t nbyte);

static int tcp_init(TranscodeContext *ctx, int type)
{
  assert(type == FT_OUTPUT);

  TcpContext *tcp = (TcpContext *) ctx->priv_data;

  tcp->package = ctx->param.package;

  char tmp[BUFSIZ];
  sscanf(ctx->output, "tcp://%s", tmp);
  char *lasts = NULL;
  char *ip = strtok_r(tmp, ":", &lasts);
  char *port = strtok_r(NULL, ":", &lasts);
  if (ip == NULL || port == NULL)
  {
    return -1;
  }

  struct sockaddr_in addr;
  tcp->fd = socket(AF_INET, SOCK_STREAM, 0);
  assert(tcp->fd >= 0);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(port));
  addr.sin_addr.s_addr = inet_addr(ip);
  bzero(&addr.sin_zero, 8);

  // Turns Nagle's algorithm off
  int on = 1;
  setsockopt(tcp->fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

  int ret = connect(tcp->fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
  if (ret < 0)
  {
    fprintf(stderr, "connect to %s:%s failed.\n", ip, port);
  }

  return ret;
}

static int tcp_fini(TranscodeContext *ctx)
{
  TcpContext *tcp = (TcpContext *) ctx->priv_data;

  close(tcp->fd);

  return 0;
}

static int tcp_apply(TranscodeContext *ctx, AVPacket *pkt)
{
  TcpContext *tcp = (TcpContext *) ctx->priv_data;

  if (pkt != NULL && pkt->data != NULL)
  {
    int extradata_size = 0;
    uint8_t *extradata = av_packet_get_side_data(pkt,
                                                 AV_PKT_DATA_NEW_EXTRADATA,
                                                 &extradata_size);
    if (extradata != NULL)
    {
      write_data(tcp, extradata, extradata_size);
    }
    write_data(tcp, pkt->data, pkt->size);

    av_packet_unref(pkt);

    return 0;
  }
  else
  {
    return AVERROR(EAGAIN);
  }
}

ssize_t write_data(TcpContext *tcp, const void *buf, size_t nbyte)
{
  if (tcp->package)
  {
    if (write_fully(tcp->fd, &nbyte, 4) < 0)
    {
      return -1;
    }
  }

  return write_fully(tcp->fd, buf, nbyte);
}

Filter tcp_filter = {
  .name = "tcp",
  .priv_data_size = sizeof (TcpContext),
  .init = tcp_init,
  .fini = tcp_fini,
  .apply = tcp_apply
};
