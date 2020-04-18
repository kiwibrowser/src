LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE_TAGS := optional
LOCAL_PACKAGE_NAME := GLMark2

LOCAL_JNI_SHARED_LIBRARIES := libglmark2-android

include $(BUILD_PACKAGE)

include $(LOCAL_PATH)/jni/Android.mk
