# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

include common.mk

LIBDIR ?= /usr/lib

all: CC_LIBRARY(src/libevdev.so.0) CC_LIBRARY(src/libevdev_hollow.so.0)
clean: CLEAN(src/libevdev.so.0) CLEAN(src/libevdev_hollow.so.0)
install: install-lib install-header

in-place: CC_LIBRARY(src/libevdev_hollow.so.0)
clean-in-place: clean
setup-in-place: setup-lib-in-place setup-header-in-place