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

#include <ScreenCapture.h>

void arpcap_init() {
}

void arpcap_fini() {
}

int arpcap_get_display_info(ARPDisplayInfo *info) {
    return 0;
}

int arpcap_create(
    uint32_t paddingTop, uint32_t paddingBottom, uint32_t width, uint32_t height, arp_callback cb)
{
    return 0;
}

void arpcap_destroy() {
}

int arpcap_acquire_frame_buffer(ARPFrameBuffer *fb) {
    return 0;
}

void arpcap_release_frame_buffer() {
}
