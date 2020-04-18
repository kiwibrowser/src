LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := cpufeatures
LOCAL_SRC_FILES := cpu-features.c
LOCAL_CFLAGS := -Wall -Wextra -Werror
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_LDLIBS := -ldl
include $(BUILD_STATIC_LIBRARY)
