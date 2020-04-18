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

LOCAL_PATH := $(call my-dir)
SRC_DIR := ../..
LAYER_DIR := ../generated

# specific for NDK build
SHADERC_DIR := $(SRC_DIR)/../shaderc
GLSLANG_DIR := $(SHADERC_DIR)/glslang
SPIRV_TOOLS_DIR := $(SHADERC_DIR)/spirv-tools

include $(CLEAR_VARS)
LOCAL_MODULE := layer_utils
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_config.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_extension_utils.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_utils.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SRC_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/layers \
                    $(LOCAL_PATH)/$(SRC_DIR)/loader
LOCAL_CPPFLAGS += -std=c++11 -Wall -Werror -Wno-unused-function -Wno-unused-const-variable -mxgot
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR -DVK_PROTOTYPES -fvisibility=hidden
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_core_validation
LOCAL_SRC_FILES += $(SRC_DIR)/layers/core_validation.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/descriptor_sets.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_table.cpp
LOCAL_SRC_FILES += $(LAYER_DIR)/include/vk_safe_struct.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SRC_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/layers \
                    $(LOCAL_PATH)/$(LAYER_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/loader
LOCAL_STATIC_LIBRARIES += layer_utils glslang SPIRV-Tools
LOCAL_CPPFLAGS += -std=c++11 -Wall -Werror -Wno-unused-function -Wno-unused-const-variable -mxgot
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR -DVK_PROTOTYPES -fvisibility=hidden
LOCAL_LDLIBS    := -llog
LOCAL_LDFLAGS   += -Wl,-Bsymbolic
LOCAL_LDFLAGS   += -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_image
LOCAL_SRC_FILES += $(SRC_DIR)/layers/image.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SRC_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/layers \
                    $(LOCAL_PATH)/$(LAYER_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/loader
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -std=c++11 -Wall -Werror -Wno-unused-function -Wno-unused-const-variable -mxgot
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR -DVK_PROTOTYPES -fvisibility=hidden
LOCAL_LDLIBS    := -llog
LOCAL_LDFLAGS   += -Wl,-Bsymbolic
LOCAL_LDFLAGS   += -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_parameter_validation
LOCAL_SRC_FILES += $(SRC_DIR)/layers/parameter_validation.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SRC_DIR)/include \
                    $(LOCAL_PATH)/$(LAYER_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/layers \
                    $(LOCAL_PATH)/$(SRC_DIR)/loader
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -std=c++11 -Wall -Werror -Wno-unused-function -Wno-unused-const-variable -mxgot
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR -DVK_PROTOTYPES -fvisibility=hidden
LOCAL_LDLIBS    := -llog
LOCAL_LDFLAGS   += -Wl,-Bsymbolic
LOCAL_LDFLAGS   += -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_object_tracker
LOCAL_SRC_FILES += $(SRC_DIR)/layers/object_tracker.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SRC_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/layers \
                    $(LOCAL_PATH)/$(LAYER_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/loader
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -std=c++11 -Wall -Werror -Wno-unused-function -Wno-unused-const-variable -mxgot
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR -DVK_PROTOTYPES -fvisibility=hidden
LOCAL_LDLIBS    := -llog
LOCAL_LDFLAGS   += -Wl,-Bsymbolic
LOCAL_LDFLAGS   += -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_threading
LOCAL_SRC_FILES += $(SRC_DIR)/layers/threading.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SRC_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/layers \
                    $(LOCAL_PATH)/$(LAYER_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/loader
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -std=c++11 -Wall -Werror -Wno-unused-function -Wno-unused-const-variable -mxgot
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR -DVK_PROTOTYPES -fvisibility=hidden
LOCAL_LDLIBS    := -llog
LOCAL_LDFLAGS   += -Wl,-Bsymbolic
LOCAL_LDFLAGS   += -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_unique_objects
LOCAL_SRC_FILES += $(SRC_DIR)/layers/unique_objects.cpp
LOCAL_SRC_FILES += $(LAYER_DIR)/include/vk_safe_struct.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SRC_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/layers \
                    $(LOCAL_PATH)/$(LAYER_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/loader
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -std=c++11 -Wall -Werror -Wno-unused-function -Wno-unused-const-variable -mxgot
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR -DVK_PROTOTYPES -fvisibility=hidden
LOCAL_LDLIBS    := -llog
LOCAL_LDFLAGS   += -Wl,-Bsymbolic
LOCAL_LDFLAGS   += -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_swapchain
LOCAL_SRC_FILES += $(SRC_DIR)/layers/swapchain.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SRC_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/layers \
                    $(LOCAL_PATH)/$(LAYER_DIR)/include \
                    $(LOCAL_PATH)/$(SRC_DIR)/loader
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -std=c++11 -Wall -Werror -Wno-unused-function -Wno-unused-const-variable -mxgot
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR -DVK_PROTOTYPES -fvisibility=hidden
LOCAL_LDLIBS    := -llog
LOCAL_LDFLAGS   += -Wl,-Bsymbolic
LOCAL_LDFLAGS   += -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
$(call import-module,third_party/googletest)
$(call import-module,third_party/shaderc)

