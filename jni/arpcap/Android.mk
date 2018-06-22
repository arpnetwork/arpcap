LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := arpcap

LOCAL_SRC_FILES := \
	src/filters/av.c \
	src/filters/cap.c \
	src/filters/file.c \
	src/filters/pipe.c \
	src/filters/repeat.c \
	src/filters/stat.c \
	src/filters/tcp.c \
	src/cap.cpp \
	src/filter.c \
	src/utils.c \
	src/arpcap.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \

LOCAL_STATIC_LIBRARIES := \
	libyuv \
	libyuv_neon \
	libavcodec \
	libavutil \
	libx264 \

LOCAL_SHARED_LIBRARIES := \
	arpcap-shared \

include $(BUILD_EXECUTABLE)
