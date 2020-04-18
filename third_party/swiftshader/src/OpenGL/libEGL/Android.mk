LOCAL_PATH:= $(call my-dir)

COMMON_CFLAGS := \
	-DLOG_TAG=\"libEGL_swiftshader\" \
	-std=c++11 \
	-DEGLAPI= \
	-DEGL_EGLEXT_PROTOTYPES \
	-Wall \
	-Werror \
	-Wno-format \
	-Wno-unused-function \
	-Wno-unused-parameter \
	-Wno-implicit-exception-spec-mismatch \
	-Wno-overloaded-virtual \
	-Wno-attributes \
	-Wno-unknown-attributes \
	-Wno-unknown-warning-option \
	-DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) \
	-DNO_SANITIZE_FUNCTION=

ifneq (16,${PLATFORM_SDK_VERSION})
COMMON_CFLAGS += -Xclang -fuse-init-array
else
COMMON_CFLAGS += -D__STDC_INT64__
endif

COMMON_SRC_FILES := \
	Config.cpp \
	Display.cpp \
	Surface.cpp \
	libEGL.cpp \
	main.cpp

COMMON_C_INCLUDES := \
	bionic \
	$(LOCAL_PATH)/../../../include \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../../

COMMON_SHARED_LIBRARIES := \
	libdl \
	liblog \
	libcutils \
	libhardware

# Project Treble is introduced from Oreo MR1
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 27 && echo OreoMR1),OreoMR1)
COMMON_SHARED_LIBRARIES += libnativewindow
COMMON_STATIC_LIBRARIES += libarect
COMMON_HEADER_LIBRARIES += libnativebase_headers
endif

# gralloc1 is introduced from N MR1
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 25 && echo NMR1),NMR1)
COMMON_CFLAGS += -DHAVE_GRALLOC1
COMMON_SHARED_LIBRARIES += libsync
endif

# Marshmallow does not have stlport, but comes with libc++ by default
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 23 && echo PreMarshmallow),PreMarshmallow)
COMMON_SHARED_LIBRARIES += libstlport
COMMON_C_INCLUDES += external/stlport/stlport
endif

COMMON_LDFLAGS := \
	-Wl,--version-script=$(LOCAL_PATH)/libEGL.lds \
	-Wl,--gc-sections \
	-Wl,--hash-style=sysv

include $(CLEAR_VARS)
LOCAL_MODULE := libEGL_swiftshader_debug

ifeq ($(TARGET_TRANSLATE_2ND_ARCH),true)
LOCAL_MULTILIB := first
endif

ifeq (HasRelativePath,$(shell test $(PLATFORM_SDK_VERSION) -ge 21 && echo HasRelativePath))
LOCAL_MODULE_RELATIVE_PATH := egl
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/egl
endif

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_CLANG := true
LOCAL_SRC_FILES := $(COMMON_SRC_FILES)
LOCAL_C_INCLUDES += $(COMMON_C_INCLUDES)
LOCAL_STATIC_LIBRARIES += swiftshader_top_debug $(COMMON_STATIC_LIBRARIES)
LOCAL_SHARED_LIBRARIES += $(COMMON_SHARED_LIBRARIES)
LOCAL_HEADER_LIBRARIES := $(COMMON_HEADER_LIBRARIES)
LOCAL_LDFLAGS += $(COMMON_LDFLAGS)
LOCAL_CFLAGS += $(COMMON_CFLAGS) -UNDEBUG -g -O0
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libEGL_swiftshader

ifeq ($(TARGET_TRANSLATE_2ND_ARCH),true)
LOCAL_MULTILIB := first
endif

ifeq (HasRelativePath,$(shell test $(PLATFORM_SDK_VERSION) -ge 21 && echo HasRelativePath))
LOCAL_MODULE_RELATIVE_PATH := egl
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/egl
endif

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_CLANG := true
LOCAL_SRC_FILES := $(COMMON_SRC_FILES)
LOCAL_C_INCLUDES += $(COMMON_C_INCLUDES)
LOCAL_STATIC_LIBRARIES += swiftshader_top_release $(COMMON_STATIC_LIBRARIES)
LOCAL_SHARED_LIBRARIES += $(COMMON_SHARED_LIBRARIES)
LOCAL_HEADER_LIBRARIES := $(COMMON_HEADER_LIBRARIES)
LOCAL_LDFLAGS += $(COMMON_LDFLAGS)
LOCAL_CFLAGS += $(COMMON_CFLAGS) -DANGLE_DISABLE_TRACE
include $(BUILD_SHARED_LIBRARY)
