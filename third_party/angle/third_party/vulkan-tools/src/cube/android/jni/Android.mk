# Copyright 2015 The Android Open Source Project
# Copyright (C) 2015 Valve Corporation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(abspath $(call my-dir))
SRC_DIR := $(LOCAL_PATH)/../../..
DEMO_DIR := $(SRC_DIR)/cube

include $(CLEAR_VARS)
LOCAL_MODULE := VkCube
LOCAL_SRC_FILES += $(DEMO_DIR)/cube.c \
                   $(SRC_DIR)/common/vulkan_wrapper.cpp \
                   $(SRC_DIR)/common/android_util.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/build-android/third_party/Vulkan-Headers/include \
                    $(DEMO_DIR)/android/include \
                    $(SRC_DIR)/libs \
                    $(SRC_DIR)/common \
                    $(SRC_DIR)/build-android/generated/include
LOCAL_CFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR --include=$(SRC_DIR)/common/vulkan_wrapper.h
LOCAL_WHOLE_STATIC_LIBRARIES += android_native_app_glue
LOCAL_LDLIBS    := -llog -landroid
LOCAL_LDFLAGS   := -u ANativeActivity_onCreate
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
