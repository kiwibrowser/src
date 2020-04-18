# Copyright (C) 2015 The Android Open Source Project
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
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := CtsDeqpTestCases

LOCAL_MODULE_TAGS := optional

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

LOCAL_SDK_VERSION := 22

LOCAL_SRC_FILES := $(call all-java-files-under, runner/src)
LOCAL_JAVA_LIBRARIES := cts-tradefed compatibility-host-util tradefed

DEQP_CASELISTS:=$(sort $(patsubst master/%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find -L master -maxdepth 1 -name "*.txt") \
  ))
LOCAL_COMPATIBILITY_SUPPORT_FILES := $(foreach file, $(DEQP_CASELISTS), $(LOCAL_PATH)/master/$(file):$(file))
LOCAL_COMPATIBILITY_SUPPORT_FILES += $(LOCAL_PATH)/nyc/vk-master.txt:nyc-vk-master.txt
LOCAL_COMPATIBILITY_SUPPORT_FILES += $(LOCAL_PATH)/nyc/gles31-master.txt:nyc-gles31-master.txt
LOCAL_COMPATIBILITY_SUPPORT_FILES += $(LOCAL_PATH)/nyc/egl-master.txt:nyc-egl-master.txt

include $(BUILD_HOST_JAVA_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
