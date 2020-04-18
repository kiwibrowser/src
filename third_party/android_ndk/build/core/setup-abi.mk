# Copyright (C) 2009 The Android Open Source Project
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

# this file is included multiple times by build/core/setup-app.mk
#

$(call ndk_log,Building application '$(NDK_APP_NAME)' for ABI '$(TARGET_ARCH_ABI)')

TARGET_ARCH := $(strip $(NDK_ABI.$(TARGET_ARCH_ABI).arch))
ifndef TARGET_ARCH
    $(call __ndk_info,ERROR: The $(TARGET_ARCH_ABI) ABI has no associated architecture!)
    $(call __ndk_error,Aborting...)
endif

TARGET_OUT := $(NDK_APP_OUT)/$(_app)/$(TARGET_ARCH_ABI)

# For x86 and mips: the minimal platform level is android-9
TARGET_PLATFORM_SAVED := $(TARGET_PLATFORM)

# For 64-bit ABIs: the minimal platform level is android-21
ifneq ($(filter $(NDK_KNOWN_DEVICE_ABI64S),$(TARGET_ARCH_ABI)),)
$(foreach _plat,3 4 5 8 9 10 11 12 13 14 15 16 17 18 19 20,\
    $(eval TARGET_PLATFORM := $$(subst android-$(_plat),android-21,$$(TARGET_PLATFORM)))\
)
endif

TARGET_PLATFORM_LEVEL := $(strip $(subst android-,,$(TARGET_PLATFORM)))
ifneq (,$(call gte,$(TARGET_PLATFORM_LEVEL),$(NDK_FIRST_PIE_PLATFORM_LEVEL)))
    TARGET_PIE := true
    $(call ndk_log,  Enabling -fPIE for TARGET_PLATFORM $(TARGET_PLATFORM))
else
    TARGET_PIE := false
endif

# Separate the debug and release objects. This prevents rebuilding
# everything when you switch between these two modes. For projects
# with lots of C++ sources, this can be a considerable time saver.
ifeq ($(NDK_APP_OPTIM),debug)
TARGET_OBJS := $(TARGET_OUT)/objs-debug
else
TARGET_OBJS := $(TARGET_OUT)/objs
endif

TARGET_GDB_SETUP := $(TARGET_OUT)/setup.gdb

# RS triple
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  RS_TRIPLE := armv7-none-linux-gnueabi
endif
ifeq ($(TARGET_ARCH_ABI),armeabi)
  RS_TRIPLE := arm-none-linux-gnueabi
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  RS_TRIPLE := aarch64-linux-android
endif
ifeq ($(TARGET_ARCH_ABI),mips)
  RS_TRIPLE := mipsel-unknown-linux
endif
ifeq ($(TARGET_ARCH_ABI),x86)
  RS_TRIPLE := i686-unknown-linux
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
  RS_TRIPLE := x86_64-unknown-linux
endif

include $(BUILD_SYSTEM)/setup-toolchain.mk

# Restore TARGET_PLATFORM, see above.
TARGET_PLATFORM := $(TARGET_PLATFORM_SAVED)
