LOCAL_PATH := $(abspath $(call my-dir))
include $(CLEAR_VARS)

LOCAL_MODULE := libarpcap-shared
LOCAL_MODULE_FILENAME := libarpcap

LOCAL_SRC_FILES := \
	dummy/ScreenCapture.cpp \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include \

include $(BUILD_SHARED_LIBRARY)
