# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
include common.mk

CFLAGS += -std=gnu99 -fvisibility=default
CPPFLAGS += -I$(SRC)/include

CC_LIBRARY(src/libevdev_hollow.so.0): CFLAGS += -DEVDEV_HOLLOW
CC_LIBRARY(src/libevdev_hollow.so.0): src/libevdev.o \
	src/libevdev_mt.o \
	src/libevdev_event.o

CC_LIBRARY(src/libevdev.so.0): src/libevdev.o \
	src/libevdev_mt.o \
	src/libevdev_event.o

install-lib: CC_LIBRARY(src/libevdev.so.0) CC_LIBRARY(src/libevdev_hollow.so.0)
	install -D -m 0755 src/libevdev.so.0 $(DESTDIR)$(LIBDIR)/libevdev.so.0
	ln -f -s libevdev.so.0 $(DESTDIR)$(LIBDIR)/libevdev.so
	install -D -m 0755 src/libevdev_hollow.so.0 \
		$(DESTDIR)$(LIBDIR)/libevdev_hollow.so.0
	ln -f -s libevdev_hollow.so.0 $(DESTDIR)$(LIBDIR)/libevdev_hollow.so
	install -D -m 0644 $(SRC)/libevdev-cros.pc \
			$(DESTDIR)$(LIBDIR)/pkgconfig/libevdev-cros.pc

setup-lib-in-place:
	mkdir -p $(SRC)/in-place || true
	ln -sf $(SRC)/src/libevdev_hollow.so.0 $(SRC)/in-place/libevdev_hollow.so
	ln -sf $(SRC)/src/libevdev_hollow.so.0 $(SRC)/in-place/libevdev_hollow.so.0
