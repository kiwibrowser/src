#!/bin/sh
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
find \
	'(' -name '*.[ch]' -or -name '*.cc' ')' \
	-not -name 'gbm.h' -not -name 'virgl_hw.h' \
	-exec clang-format -style=file -i {} +
