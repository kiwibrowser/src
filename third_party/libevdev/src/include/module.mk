# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

include common.mk

install-header: CC_LIBRARY(src/libevdev.so.0)
	set -e; \
	for h in libevdev.h libevdev_event.h libevdev_mt.h libevdev_log.h; do \
		install -D -m 0644 $(SRC)/include/libevdev/$$h \
			$(DESTDIR)/usr/include/libevdev/$$h; \
	done


setup-header-in-place:
	mkdir -p $(SRC)/in-place || true
	ln -sfn $(SRC)/include/libevdev $(SRC)/in-place/libevdev
