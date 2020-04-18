# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

include common.mk

PC_DEPS = libdrm
PC_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PC_DEPS))
PC_LIBS := $(shell $(PKG_CONFIG) --libs $(PC_DEPS))

CPPFLAGS += -D_GNU_SOURCE=1
CFLAGS += -std=c99 -Wall -Wsign-compare -Wpointer-arith -Wcast-qual \
	  -Wcast-align -D_GNU_SOURCE=1 -D_FILE_OFFSET_BITS=64

ifdef DRV_AMDGPU
	CFLAGS += $(shell $(PKG_CONFIG) --cflags libdrm_amdgpu)
	LDLIBS += -lamdgpuaddr
endif
ifdef DRV_EXYNOS
	CFLAGS += $(shell $(PKG_CONFIG) --cflags libdrm_exynos)
endif
ifdef DRV_I915
	CFLAGS += $(shell $(PKG_CONFIG) --cflags libdrm_intel)
endif
ifdef DRV_MESON
	CFLAGS += $(shell $(PKG_CONFIG) --cflags libdrm_meson)
endif
ifdef DRV_RADEON
	CFLAGS += $(shell $(PKG_CONFIG) --cflags libdrm_radeon)
endif
ifdef DRV_ROCKCHIP
	CFLAGS += $(shell $(PKG_CONFIG) --cflags libdrm_rockchip)
endif

CPPFLAGS += $(PC_CFLAGS)
LDLIBS += $(PC_LIBS)

LIBDIR ?= /usr/lib/

GBM_VERSION_MAJOR := 1
MINIGBM_VERSION := $(GBM_VERSION_MAJOR).0.0
MINIGBM_FILENAME := libminigbm.so.$(MINIGBM_VERSION)

CC_LIBRARY($(MINIGBM_FILENAME)): LDFLAGS += -Wl,-soname,libgbm.so.$(GBM_VERSION_MAJOR)
CC_LIBRARY($(MINIGBM_FILENAME)): $(C_OBJECTS)

all: CC_LIBRARY($(MINIGBM_FILENAME))

clean: CLEAN($(MINIGBM_FILENAME))

install: all
	mkdir -p $(DESTDIR)/$(LIBDIR)
	install -D -m 755 $(OUT)/$(MINIGBM_FILENAME) $(DESTDIR)/$(LIBDIR)
	ln -sf $(MINIGBM_FILENAME) $(DESTDIR)/$(LIBDIR)/libgbm.so
	ln -sf $(MINIGBM_FILENAME) $(DESTDIR)/$(LIBDIR)/libgbm.so.$(GBM_VERSION_MAJOR)
	install -D -m 0644 $(SRC)/gbm.pc $(DESTDIR)$(LIBDIR)/pkgconfig/gbm.pc
	install -D -m 0644 $(SRC)/gbm.h $(DESTDIR)/usr/include/gbm.h
