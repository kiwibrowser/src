# This file is dual licensed under the MIT and the University of Illinois Open
# Source Licenses. See LICENSE.TXT for details.

LOCAL_PATH := $(call my-dir)

# Normally, we distribute the NDK with prebuilt binaries of libc++
# in $LOCAL_PATH/libs/<abi>/. However,
#

LIBCXX_FORCE_REBUILD := $(strip $(LIBCXX_FORCE_REBUILD))
ifndef LIBCXX_FORCE_REBUILD
  ifeq (,$(strip $(wildcard $(LOCAL_PATH)/libs/$(TARGET_ARCH_ABI)/libc++_static$(TARGET_LIB_EXTENSION))))
    $(call __ndk_info,WARNING: Rebuilding libc++ libraries from sources!)
    $(call __ndk_info,You might want to use $$NDK/build/tools/build-cxx-stl.sh --stl=libc++)
    $(call __ndk_info,in order to build prebuilt versions to speed up your builds!)
    LIBCXX_FORCE_REBUILD := true
  endif
endif

libcxx_includes := $(LOCAL_PATH)/include
libcxx_export_includes := $(libcxx_includes)
libcxx_sources := \
    algorithm.cpp \
    any.cpp \
    bind.cpp \
    chrono.cpp \
    condition_variable.cpp \
    debug.cpp \
    exception.cpp \
    functional.cpp \
    future.cpp \
    hash.cpp \
    ios.cpp \
    iostream.cpp \
    locale.cpp \
    memory.cpp \
    mutex.cpp \
    new.cpp \
    optional.cpp \
    random.cpp \
    regex.cpp \
    shared_mutex.cpp \
    stdexcept.cpp \
    string.cpp \
    strstream.cpp \
    system_error.cpp \
    thread.cpp \
    typeinfo.cpp \
    utility.cpp \
    valarray.cpp \
    variant.cpp \
    vector.cpp \

libcxx_sources := $(libcxx_sources:%=src/%)

libcxx_export_cxxflags :=

ifeq (,$(filter clang%,$(NDK_TOOLCHAIN_VERSION)))
# Add -fno-strict-aliasing because __list_imp::_end_ breaks TBAA rules by declaring
# simply as __list_node_base then casted to __list_node derived from that.  See
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61571 for details
libcxx_export_cxxflags += -fno-strict-aliasing
endif

libcxx_cxxflags := \
    -std=c++1z \
    -DLIBCXX_BUILDING_LIBCXXABI \
    -D_LIBCPP_BUILDING_LIBRARY \
    -D_LIBCPP_DISABLE_NEW_DELETE_DEFINITIONS \
    -D__STDC_FORMAT_MACROS \
    $(libcxx_export_cxxflags) \

libcxx_ldflags :=
libcxx_export_ldflags :=
# Need to make sure the unwinder is always linked with hidden visibility.
ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
    libcxx_ldflags += -Wl,--exclude-libs,libunwind.a
    libcxx_export_ldflags += -Wl,--exclude-libs,libunwind.a
endif

ifneq ($(LIBCXX_FORCE_REBUILD),true)

$(call ndk_log,Using prebuilt libc++ libraries)

libcxxabi_c_includes := $(LOCAL_PATH)/../llvm-libc++abi/include

include $(CLEAR_VARS)
LOCAL_MODULE := c++_static
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE)$(TARGET_LIB_EXTENSION)
LOCAL_EXPORT_C_INCLUDES := $(libcxx_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(libcxx_export_cxxflags)
LOCAL_EXPORT_LDFLAGS := $(libcxx_export_ldflags)

# We use the LLVM unwinder for all the 32-bit ARM targets.
ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
    LOCAL_EXPORT_STATIC_LIBRARIES += libunwind
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_shared
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE)$(TARGET_SONAME_EXTENSION)
LOCAL_EXPORT_C_INCLUDES := \
    $(libcxx_export_includes) \
    $(libcxxabi_c_includes) \

# This doesn't affect the prebuilt itself since this is a prebuilt library, but
# the build system needs to know about the dependency so we can sort the
# exported includes properly.
LOCAL_STATIC_LIBRARIES := libandroid_support
LOCAL_EXPORT_CPPFLAGS := $(libcxx_export_cxxflags)
LOCAL_EXPORT_LDFLAGS := $(libcxx_export_ldflags)

# We use the LLVM unwinder for all the 32-bit ARM targets.
ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
    LOCAL_EXPORT_STATIC_LIBRARIES += libunwind
endif
include $(PREBUILT_SHARED_LIBRARY)

ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
# We define this module here rather than in a separate cxx-stl/libunwind because
# we don't actually want to make the API available (yet).
include $(CLEAR_VARS)
LOCAL_MODULE := libunwind
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/$(LOCAL_MODULE)$(TARGET_LIB_EXTENSION)
include $(PREBUILT_STATIC_LIBRARY)
endif

$(call import-module, cxx-stl/llvm-libc++abi)

else
# LIBCXX_FORCE_REBUILD == true

$(call ndk_log,Rebuilding libc++ libraries from sources)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_static
LOCAL_SRC_FILES := $(libcxx_sources)
LOCAL_C_INCLUDES := $(libcxx_includes)
LOCAL_CPPFLAGS := $(libcxx_cxxflags)
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(libcxx_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(libcxx_export_cxxflags)
LOCAL_EXPORT_LDFLAGS := $(libcxx_export_ldflags)
LOCAL_STATIC_LIBRARIES := libc++abi android_support

# We use the LLVM unwinder for all the 32-bit ARM targets.
ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
    LOCAL_STATIC_LIBRARIES += libunwind
    LOCAL_EXPORT_STATIC_LIBRARIES += libunwind
endif

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_shared
LOCAL_WHOLE_STATIC_LIBRARIES := c++_static libc++abi
LOCAL_EXPORT_C_INCLUDES := $(libcxx_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(libcxx_export_cxxflags)
LOCAL_EXPORT_LDFLAGS := $(libcxx_export_ldflags)
LOCAL_STATIC_LIBRARIES := android_support
LOCAL_LDFLAGS := $(libcxx_ldflags)
# Use --as-needed to strip the DT_NEEDED on libstdc++.so (bionic's) that the
# driver always links for C++ but we don't use.
# See https://github.com/android-ndk/ndk/issues/105
LOCAL_LDFLAGS += -Wl,--as-needed

# We use the LLVM unwinder for all the 32-bit ARM targets.
ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
    LOCAL_STATIC_LIBRARIES += libunwind
    LOCAL_EXPORT_STATIC_LIBRARIES += libunwind
endif

# But only need -latomic for armeabi.
ifeq ($(TARGET_ARCH_ABI),armeabi)
    LOCAL_LDLIBS += -latomic
endif
include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(LOCAL_PATH)/../..)
$(call import-module, external/libcxxabi)

endif # LIBCXX_FORCE_REBUILD == true

$(call import-module, android/support)
