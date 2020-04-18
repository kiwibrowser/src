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

APP_ABI := armeabi-v7a arm64-v8a x86 x86_64
APP_PLATFORM := android-23
APP_STL := c++_static
APP_MODULES := VkCube
APP_CPPFLAGS += -std=c++11 -fexceptions -Wall -Werror -Wextra -Wno-unused-parameter -DVK_NO_PROTOTYES -DGLM_FORCE_RADIANS
APP_CFLAGS += -Wall -Werror -Wextra -Wno-unused-parameter -DVK_NO_PROTOTYES -DGLM_FORCE_RADIANS
NDK_TOOLCHAIN_VERSION := clang
