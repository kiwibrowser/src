# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

GRALLOC = gralloc.cros.so

SRCS    = $(wildcard *.cc)
SRCS   += $(wildcard ../*.c)

SRCS   += $(wildcard gralloc0/*.cc)

SOURCES = $(filter-out ../gbm%, $(SRCS))
PKG_CONFIG ?= pkg-config

VPATH = $(dir $(SOURCES))
LIBDRM_CFLAGS := $(shell $(PKG_CONFIG) --cflags libdrm)
LIBDRM_LIBS := $(shell $(PKG_CONFIG) --libs libdrm)

CPPFLAGS += -Wall -fPIC -Werror -flto $(LIBDRM_CFLAGS)
CXXFLAGS += -std=c++14
CFLAGS   += -std=c99
LIBS     += -shared -lcutils -lhardware -lsync $(LIBDRM_LIBS)

ifdef DRV_AMDGPU
	LIBS += -lamdgpuaddr
endif

OBJS =  $(foreach source, $(SOURCES), $(addsuffix .o, $(basename $(source))))

OBJECTS = $(addprefix $(TARGET_DIR), $(notdir $(OBJS)))
LIBRARY = $(addprefix $(TARGET_DIR), $(GRALLOC))

.PHONY: all clean

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)

clean:
	$(RM) $(LIBRARY)
	$(RM) $(OBJECTS)

$(LIBRARY):
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(TARGET_DIR)%.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@ -MMD

$(TARGET_DIR)%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@ -MMD
