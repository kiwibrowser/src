# Copyright (C) 2014-2015 The Android Open Source Project
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

include $(CLEAR_VARS)

# don't include this package in any target ??????
LOCAL_MODULE_TAGS := optional
# and when built explicitly put it in the data partition
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_MODULE_TAGS := tests

LOCAL_COMPATIBILITY_SUITE := cts vts

LOCAL_SRC_FILES := $(call all-java-files-under,src)
LOCAL_JNI_SHARED_LIBRARIES := libdeqp

LOCAL_ASSET_DIR := \
	$(LOCAL_PATH)/../../data \
	$(LOCAL_PATH)/../../external/vulkancts/data \
	$(LOCAL_PATH)/../../../../prebuilts/deqp/spirv \
	$(LOCAL_PATH)/../../external/graphicsfuzz/data

LOCAL_PACKAGE_NAME := com.drawelements.deqp
LOCAL_MULTILIB := both

# We could go down all the way to API-13 for 32bit. 22 is required for 64bit ARM.
LOCAL_SDK_VERSION := 22

include $(BUILD_PACKAGE)
