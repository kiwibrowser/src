#
# Copyright (C) 2016 The Android Open Source Project
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

libcxxabi_src_files := \
    src/abort_message.cpp \
    src/cxa_aux_runtime.cpp \
    src/cxa_default_handlers.cpp \
    src/cxa_demangle.cpp \
    src/cxa_exception.cpp \
    src/cxa_exception_storage.cpp \
    src/cxa_guard.cpp \
    src/cxa_handlers.cpp \
    src/cxa_personality.cpp \
    src/cxa_thread_atexit.cpp \
    src/cxa_unexpected.cpp \
    src/cxa_vector.cpp \
    src/cxa_virtual.cpp \
    src/fallback_malloc.cpp \
    src/private_typeinfo.cpp \
    src/stdlib_exception.cpp \
    src/stdlib_new_delete.cpp \
    src/stdlib_stdexcept.cpp \
    src/stdlib_typeinfo.cpp \

libcxxabi_includes := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/../libunwind_llvm/include \
    $(LOCAL_PATH)/../libcxx/include \
    $(LOCAL_PATH)/../../ndk/sources/android/support/include

libcxxabi_cflags := -D__STDC_FORMAT_MACROS
libcxxabi_cppflags := -std=c++11 -Wno-unknown-attributes

ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
    use_llvm_unwinder := true
    libcxxabi_cppflags += -DLIBCXXABI_USE_LLVM_UNWINDER=1
else
    use_llvm_unwinder := false
    libcxxabi_cppflags += -DLIBCXXABI_USE_LLVM_UNWINDER=0
endif

ifneq ($(LIBCXX_FORCE_REBUILD),true) # Using prebuilt

include $(CLEAR_VARS)
LOCAL_MODULE := libc++abi
LOCAL_SRC_FILES := ../llvm-libc++/libs/$(TARGET_ARCH_ABI)/$(LOCAL_MODULE)$(TARGET_LIB_EXTENSION)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

else # Building

include $(CLEAR_VARS)
LOCAL_MODULE := libc++abi
LOCAL_SRC_FILES := $(libcxxabi_src_files)
LOCAL_C_INCLUDES := $(libcxxabi_includes)
LOCAL_CPPFLAGS := $(libcxxabi_cppflags)
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_STATIC_LIBRARIES := android_support

# Unlike the platform build, ndk-build will actually perform dependency checking
# on static libraries and topologically sort them to determine link order.
# Though there is no link step, without this we may link libunwind before
# libc++abi, which won't succeed.
ifeq ($(use_llvm_unwinder),true)
    LOCAL_STATIC_LIBRARIES += libunwind
endif
include $(BUILD_STATIC_LIBRARY)

$(call import-add-path, $(LOCAL_PATH)/../..)
$(call import-module, external/libunwind_llvm)

endif # Prebuilt/building

$(call import-module, android/support)
