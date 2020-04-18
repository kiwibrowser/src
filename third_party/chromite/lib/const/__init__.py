# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function

import datetime


# This is to work around a Python bug:  The first call to
# datetime.datetime.strptime() within the Python VM can fail if it
# happens in a multi-threaded context.  To work around that, we force a
# "safe" call here.  For more details, see:
#     https://bugs.python.org/issue7980
#     https://crbug.com/710182
#
datetime.datetime.strptime(datetime.datetime.now().strftime('%Y'), '%Y')
