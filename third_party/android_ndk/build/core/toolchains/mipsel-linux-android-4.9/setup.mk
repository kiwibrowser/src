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

# this file is used to prepare the NDK to build 32-bit mips with the mips64el gcc-4.9
# toolchain any number of source files
#
# its purpose is to define (or re-define) templates used to build
# various sources into target object files, libraries or executables.
#
# Note that this file may end up being parsed several times in future
# revisions of the NDK.
#

#
# Override the toolchain prefix
#
TOOLCHAIN_NAME := mips64el-linux-android
BINUTILS_ROOT := $(call get-binutils-root,$(NDK_ROOT),$(TOOLCHAIN_NAME))
TOOLCHAIN_ROOT := $(call get-toolchain-root,$(TOOLCHAIN_NAME)-4.9)
TOOLCHAIN_PREFIX := $(TOOLCHAIN_ROOT)/bin/$(TOOLCHAIN_NAME)-


# CFLAGS, C_INCLUDES, and LDFLAGS
#

TARGET_CFLAGS := \
    -mips32 \
    -fpic \
    -ffunction-sections \
    -funwind-tables \
    -fstack-protector-strong \
    -fmessage-length=0 \
    -no-canonical-prefixes \

# Always enable debug info. We strip binaries when needed.
TARGET_CFLAGS += -g

TARGET_LDFLAGS := -no-canonical-prefixes -mips32

TARGET_mips_release_CFLAGS := \
    -O2 \
    -DNDEBUG \

TARGET_mips_debug_CFLAGS := \
    -O0 \
    -UNDEBUG \

# This function will be called to determine the target CFLAGS used to build
# a C or Assembler source file, based on its tags.
TARGET-process-src-files-tags = \
$(eval __debug_sources := $(call get-src-files-with-tag,debug)) \
$(eval __release_sources := $(call get-src-files-without-tag,debug)) \
$(call set-src-files-target-cflags, \
    $(__debug_sources),\
    $(TARGET_mips_debug_CFLAGS)) \
$(call set-src-files-target-cflags,\
    $(__release_sources),\
    $(TARGET_mips_release_CFLAGS)) \
