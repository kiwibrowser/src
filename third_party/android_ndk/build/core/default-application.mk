# Copyright (C) 2010 The Android Open Source Project
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

# This is the default Application.mk that is being used for applications
# that don't provide $PROJECT_PATH/jni/Application.mk
#
APP_PROJECT_PATH := $(NDK_PROJECT_PATH)

# We expect the build script to be located here
ifndef APP_BUILD_SCRIPT
  ifeq (null,$(NDK_PROJECT_PATH))
    $(call __ndk_info,NDK_PROJECT_PATH==null.  Please explicitly set APP_BUILD_SCRIPT.)
    $(call __ndk_error,Aborting.)
  endif
  APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/jni/Android.mk
endif