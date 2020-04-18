# Copyright (C) 2014 The Android Open Source Project
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

# this file is used to prepare the NDK to build with the arm64 gcc-4.9
# toolchain any number of source files
#
# its purpose is to define (or re-define) templates used to build
# various sources into target object files, libraries or executables.
#
# Note that this file may end up being parsed several times in future
# revisions of the NDK.
#

TOOLCHAIN_NAME   := aarch64-linux-android-4.9
TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREBUILT_ROOT)/bin/aarch64-linux-android-

TARGET_CFLAGS := \
    -fpic \
    -ffunction-sections \
    -funwind-tables \
    -fstack-protector-strong \
    -no-canonical-prefixes \

# Always enable debug info. We strip binaries when needed.
TARGET_CFLAGS += -g

TARGET_LDFLAGS := -no-canonical-prefixes

TARGET_arm64_release_CFLAGS := \
    -O2 \
    -DNDEBUG \

TARGET_arm64_debug_CFLAGS := \
    -O0 \
    -UNDEBUG \

# This function will be called to determine the target CFLAGS used to build
# a C or Assembler source file, based on its tags.
#
TARGET-process-src-files-tags = \
$(eval __debug_sources := $(call get-src-files-with-tag,debug)) \
$(eval __release_sources := $(call get-src-files-without-tag,debug)) \
$(call set-src-files-target-cflags, $(__debug_sources), $(TARGET_arm64_debug_CFLAGS)) \
$(call set-src-files-target-cflags, $(__release_sources),$(TARGET_arm64_release_CFLAGS)) \
