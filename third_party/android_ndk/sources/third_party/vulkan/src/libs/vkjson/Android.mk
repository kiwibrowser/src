# Copyright 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

vkjson_sources := \
	vkjson.cc \
	vkjson_instance.cc \
	../../loader/cJSON.c

# Static library for platform use
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CPPFLAGS := -std=c++11 \
	-Wno-sign-compare

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)/../../loader

LOCAL_SRC_FILES := $(vkjson_sources)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_MODULE := libvkjson

include $(BUILD_STATIC_LIBRARY)

# Static library for NDK use (CTS)
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CPPFLAGS := -std=c++11 \
	-Wno-sign-compare

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)/../../loader

LOCAL_SRC_FILES := $(vkjson_sources)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_SDK_VERSION := 24
LOCAL_NDK_STL_VARIANT := c++_static
LOCAL_MODULE := libvkjson_ndk

include $(BUILD_STATIC_LIBRARY)

vkjson_sources :=
