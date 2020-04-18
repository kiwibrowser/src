# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

hfs_raw_images.tar.bz2: hfs_plus.img hfsx_case_sensitive.img
	tar cjf $@ $^

MAKE_HFS=./make_hfs.sh

hfs_plus.img: $(MAKE_HFS)
	rm -f $@
	$(MAKE_HFS) HFS+ 1024 $@

hfsx_case_sensitive.img: $(MAKE_HFS)
	rm -f $@
	# 8*1024 = 8192
	$(MAKE_HFS) hfsx 8192 $@
