# Mesa 3-D graphics library
#
# Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
# Copyright (C) 2010-2011 LunarG Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

# BOARD_GPU_DRIVERS should be defined.  The valid values are
#
#   classic drivers: i915 i965
#   gallium drivers: swrast i915g nouveau r300g r600g radeonsi vmwgfx
#
# The main target is libGLES_mesa.  For each classic driver enabled, a DRI
# module will also be built.  DRI modules will be loaded by libGLES_mesa.

MESA_TOP := $(call my-dir)
MESA_COMMON_MK := $(MESA_TOP)/Android.common.mk
MESA_PYTHON2 := python

DRM_TOP := external/drm
DRM_GRALLOC_TOP := hardware/drm_gralloc

classic_drivers := i915 i965
gallium_drivers := swrast i915g nouveau r300g r600g radeonsi vmwgfx

MESA_GPU_DRIVERS := $(strip $(BOARD_GPU_DRIVERS))

# warn about invalid drivers
invalid_drivers := $(filter-out \
	$(classic_drivers) $(gallium_drivers), $(MESA_GPU_DRIVERS))
ifneq ($(invalid_drivers),)
$(warning invalid GPU drivers: $(invalid_drivers))
# tidy up
MESA_GPU_DRIVERS := $(filter-out $(invalid_drivers), $(MESA_GPU_DRIVERS))
endif

# host and target must be the same arch to generate matypes.h
ifeq ($(TARGET_ARCH),$(HOST_ARCH))
MESA_ENABLE_ASM := true
else
MESA_ENABLE_ASM := false
endif

ifneq ($(filter $(classic_drivers), $(MESA_GPU_DRIVERS)),)
MESA_BUILD_CLASSIC := true
else
MESA_BUILD_CLASSIC := false
endif

ifneq ($(filter $(gallium_drivers), $(MESA_GPU_DRIVERS)),)
MESA_BUILD_GALLIUM := true
else
MESA_BUILD_GALLIUM := false
endif

# add subdirectories
ifneq ($(strip $(MESA_GPU_DRIVERS)),)

SUBDIRS := \
	src/mapi \
	src/glsl \
	src/mesa \
	src/egl/main

ifeq ($(strip $(MESA_BUILD_CLASSIC)),true)
SUBDIRS += \
	src/egl/drivers/dri2 \
	src/mesa/drivers/dri
endif

ifeq ($(strip $(MESA_BUILD_GALLIUM)),true)
SUBDIRS += src/gallium
endif

mkfiles := $(patsubst %,$(MESA_TOP)/%/Android.mk,$(SUBDIRS))
include $(mkfiles)

endif
