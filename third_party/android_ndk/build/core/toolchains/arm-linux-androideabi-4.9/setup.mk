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

# this file is used to prepare the NDK to build with the arm gcc-4.9
# toolchain any number of source files
#
# its purpose is to define (or re-define) templates used to build
# various sources into target object files, libraries or executables.
#
# Note that this file may end up being parsed several times in future
# revisions of the NDK.
#

TARGET_CFLAGS := \
    -fpic \
    -ffunction-sections \
    -funwind-tables \
    -fstack-protector-strong \
    -no-canonical-prefixes \

# Always enable debug info. We strip binaries when needed.
TARGET_CFLAGS += -g

TARGET_LDFLAGS := -no-canonical-prefixes

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    TARGET_CFLAGS += \
        -march=armv7-a \
        -mfpu=vfpv3-d16 \
        -mfloat-abi=softfp \

    TARGET_LDFLAGS += \
        -march=armv7-a \
        -Wl,--fix-cortex-a8 \

else ifeq ($(TARGET_ARCH_ABI),armeabi)
    TARGET_CFLAGS += \
        -march=armv5te \
        -mtune=xscale \
        -msoft-float \

else
    $(call __ndk_error,Unsupported ABI: $(TARGET_ARCH_ABI))
endif

TARGET_CFLAGS.neon := -mfpu=neon

TARGET_arm_release_CFLAGS := \
    -marm \
    -O2 \
    -DNDEBUG \

TARGET_thumb_release_CFLAGS := \
    -mthumb \
    -Os \
    -DNDEBUG \

TARGET_arm_debug_CFLAGS := \
    -marm \
    -O0 \
    -UNDEBUG \

TARGET_thumb_debug_CFLAGS := \
    -mthumb \
    -O0 \
    -UNDEBUG \

# This function will be called to determine the target CFLAGS used to build
# a C or Assembler source file, based on its tags.
#
TARGET-process-src-files-tags = \
$(eval __arm_sources := $(call get-src-files-with-tag,arm)) \
$(eval __thumb_sources := $(call get-src-files-without-tag,arm)) \
$(eval __debug_sources := $(call get-src-files-with-tag,debug)) \
$(eval __release_sources := $(call get-src-files-without-tag,debug)) \
$(call set-src-files-target-cflags, \
    $(call set_intersection,$(__arm_sources),$(__debug_sources)), \
    $(TARGET_arm_debug_CFLAGS)) \
$(call set-src-files-target-cflags,\
    $(call set_intersection,$(__arm_sources),$(__release_sources)),\
    $(TARGET_arm_release_CFLAGS)) \
$(call set-src-files-target-cflags,\
    $(call set_intersection,$(__thumb_sources),$(__debug_sources)),\
    $(TARGET_thumb_debug_CFLAGS)) \
$(call set-src-files-target-cflags,\
    $(call set_intersection,$(__thumb_sources),$(__release_sources)),\
    $(TARGET_thumb_release_CFLAGS)) \
$(call add-src-files-target-cflags,\
    $(call get-src-files-with-tag,neon),\
    $(TARGET_CFLAGS.neon)) \
$(call set-src-files-text,$(__arm_sources),arm) \
$(call set-src-files-text,$(__thumb_sources),thumb)
