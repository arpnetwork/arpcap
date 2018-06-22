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

#include <filter.h>

#include <stddef.h>

static Filter *first_filter = NULL;
static Filter *last_filter = NULL;

#define REGISTER_FILTER(x)                    \
    {                                         \
      extern Filter x##_filter;      \
      register_filter(&x##_filter); \
    }

static void register_filter(Filter *filter);

void filter_register_all()
{
  REGISTER_FILTER(av);
  REGISTER_FILTER(cap);
  REGISTER_FILTER(file);
  REGISTER_FILTER(pipe);
  REGISTER_FILTER(repeat);
  REGISTER_FILTER(stat);
}

Filter *find_filter(const char *name)
{
  Filter *p = first_filter;
  while (p != NULL && strcmp(p->name, name) != 0) p = p->next;

  return p;
}

void register_filter(Filter *filter)
{
  if (last_filter != NULL)
  {
    last_filter->next = filter;
    last_filter = filter;
  }
  else
  {
    first_filter = last_filter = filter;
  }
}
