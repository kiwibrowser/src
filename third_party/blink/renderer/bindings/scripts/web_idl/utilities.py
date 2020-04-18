# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


def assert_no_extra_args(kwargs):
    if kwargs:
        raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())
