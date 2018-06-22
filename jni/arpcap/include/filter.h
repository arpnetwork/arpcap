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

#ifndef ARP_FILTER_H_
#define ARP_FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

typedef struct TranscodeContext TranscodeContext;

/*
 * Filter Types
 */
#define FT_FILTER 0
#define FT_INPUT  1
#define FT_OUTPUT 2

typedef struct Filter {
  const char *name;
  int priv_data_size;
  struct Filter *next;

  int (*init)(TranscodeContext *ctx, int type);
  int (*fini)(TranscodeContext *ctx);
  int (*apply)(TranscodeContext *ctx, AVPacket *pkt);
} Filter;

void filter_register_all();

Filter *find_filter(const char *name);

#ifdef __cplusplus
}
#endif

#endif  // ARP_FILTER_H_
