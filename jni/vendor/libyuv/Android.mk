LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libyuv
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := lib/$(TARGET_ARCH_ABI)/libyuv_internal.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libyuv_neon
LOCAL_SRC_FILES := lib/$(TARGET_ARCH_ABI)/libyuv_neon.a
include $(PREBUILT_STATIC_LIBRARY)
