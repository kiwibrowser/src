# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ifeq ($(strip $(BOARD_USES_MINIGBM)), true)

MINIGBM_GRALLOC_MK := $(call my-dir)/Android.gralloc.mk
LOCAL_PATH := $(call my-dir)
intel_drivers := i915 i965
include $(CLEAR_VARS)

SUBDIRS := cros_gralloc

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libdrm

LOCAL_SRC_FILES := \
	amdgpu.c \
	cirrus.c \
	drv.c \
	evdi.c \
	exynos.c \
	helpers.c \
	i915.c \
	marvell.c \
	mediatek.c \
	meson.c \
	msm.c \
	nouveau.c \
	rockchip.c \
	tegra.c \
	udl.c \
	vc4.c \
	vgem.c \
	virtio_gpu.c

include $(MINIGBM_GRALLOC_MK)

LOCAL_CPPFLAGS += -std=c++14 -D_GNU_SOURCE=1 -D_FILE_OFFSET_BITS=64
LOCAL_CFLAGS += -Wall -Wsign-compare -Wpointer-arith \
		-Wcast-qual -Wcast-align \
		-D_GNU_SOURCE=1 -D_FILE_OFFSET_BITS=64

ifneq ($(filter $(intel_drivers), $(BOARD_GPU_DRIVERS)),)
LOCAL_CPPFLAGS += -DDRV_I915
LOCAL_CFLAGS += -DDRV_I915
LOCAL_SHARED_LIBRARIES += libdrm_intel
endif

LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional
# The preferred path for vendor HALs is /vendor/lib/hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)
include $(BUILD_SHARED_LIBRARY)

#endif
