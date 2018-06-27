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

#include "cap.h"

#include <libyuv.h>
#include <ScreenCapture.h>
#include <utils.h>

#include <chrono>
#include <condition_variable>
#include <mutex>

#define RGBA_BPP    4

#define LOGE(format, ...) fprintf(stderr, "[ARPCAP] " format "\n", ##__VA_ARGS__)

struct Cap {
  int width;
  int height;

  AVPacket *pkt;
  uint8_t *data[3];
  int linesize[3];
};

class FrameRunner
{
  public:
    FrameRunner(int framerate);

    void onFrameAvailable(uint64_t frameNumber, int64_t timestamp);

    int lock(ARPFrameBuffer *fb);
    void release();

  private:
    int     mFramerate;
    int     mPendingFrames;
    int     mPendingReleased;
    bool    mLocked;

    int64_t mFrameDelay;
    int64_t mLastUpdated;

    std::mutex              mMutex;
    std::condition_variable mCondition;
};

static FrameRunner *sFRunner = nullptr;

FrameRunner::FrameRunner(int framerate) :
    mFramerate(framerate),
    mPendingFrames(0),
    mPendingReleased(0),
    mLocked(false),
    mFrameDelay(1000000000 / framerate),
    mLastUpdated(0)
{
}

void FrameRunner::onFrameAvailable(uint64_t frameNumber, int64_t timestamp) {
    std::unique_lock<std::mutex> lock(mMutex);

    mPendingFrames++;

    ARPFrameBuffer fb;
    while (mPendingFrames > 1 || (mPendingFrames > 0 && (timestamp - mLastUpdated) < mFrameDelay)) {
        if (!mLocked) {
            int res = arpcap_acquire_frame_buffer(&fb); assert(res == 0);
            arpcap_release_frame_buffer();
        } else {
            mPendingReleased++;
        }
        mPendingFrames--;
    }

    if (mPendingFrames > 0) {
        mLastUpdated = timestamp;
        mCondition.notify_one();
    }
}

int FrameRunner::lock(ARPFrameBuffer *fb) {
    std::unique_lock<std::mutex> lock(mMutex);

    auto pred = [this] {
        return mPendingFrames > 0;
    };
    if (mCondition.wait_for(lock, std::chrono::milliseconds(10), pred)) {
        mLocked = true;
        arpcap_acquire_frame_buffer(fb);
        return mPendingFrames--;;
    } else {
        return 0;
    }
}

void FrameRunner::release() {
    std::unique_lock<std::mutex> lock(mMutex);

    if (mLocked) {
        arpcap_release_frame_buffer();

        ARPFrameBuffer fb;
        while (mPendingReleased > 0) {
            int res = arpcap_acquire_frame_buffer(&fb); assert(res == 0);
            arpcap_release_frame_buffer();
            mPendingReleased--;
        }

        mLocked = false;
    }
}

Cap *cap_open(int width, int height, int framerate) {
    arpcap_init();

    if (width == 0 && height == 0)
    {
        ARPDisplayInfo info;
        if (arpcap_get_display_info(&info) == 0)
        {
            width = info.width;
            height = info.height;
        }
    }

    sFRunner = new FrameRunner(framerate);

    auto cb = [](uint64_t frameNumber, int64_t timestamp) {
        sFRunner->onFrameAvailable(frameNumber, timestamp);
    };
    int res = arpcap_create(width, height, cb);
    if (res != 0) {
        LOGE("Unable to create display.");
        delete sFRunner;
        sFRunner = nullptr;
        arpcap_fini();

        return nullptr;
    }

    Cap *cap = new Cap();
    cap->pkt = nullptr;

    return cap;
}

int cap_read(Cap *cap, AVPacket *pkt) {
    ARPFrameBuffer fb;
    if (sFRunner->lock(&fb) == 0) {
        return AVERROR(EAGAIN);
    }

    if (cap->pkt == nullptr) {
        int width = fb.width;
        int height = fb.height;
        cap->width = width;
        cap->height = height;
        cap->pkt = av_packet_alloc();
        av_new_packet(cap->pkt, width * height * 3 / 2);
        cap->data[0] = cap->pkt->data;
        cap->data[1] = cap->data[0] + width * height;
        cap->data[2] = cap->data[1] + width * height / 4;
        cap->linesize[0] = width;
        cap->linesize[1] = cap->linesize[2] = width / 2;
    }

    int res = libyuv::ABGRToI420(
            (uint8_t *)fb.data,
            fb.stride * RGBA_BPP,
            cap->data[0],
            cap->linesize[0],
            cap->data[1],
            cap->linesize[1],
            cap->data[2],
            cap->linesize[2],
            cap->width,
            cap->height);
    if (res < 0) {
        LOGE("Unable to convert frame to yuv.");
        sFRunner->release();
        return -1;
    }

    sFRunner->release();

    av_packet_ref(pkt, cap->pkt);
    pkt->stream_index = AVMEDIA_TYPE_VIDEO;
    PKT_MKSIZE(pkt, cap->width, cap->height);

    return 0;
}

int cap_close(Cap *cap) {
    arpcap_destroy();

    if (cap->pkt != nullptr)
    {
        av_packet_free(&cap->pkt);
    }
    delete cap;

    delete sFRunner;
    sFRunner = nullptr;

    arpcap_fini();

    return 0;
}
