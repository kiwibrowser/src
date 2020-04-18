# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Send system monitoring data to the timeseries monitoring API."""

from __future__ import absolute_import
from __future__ import print_function

from chromite.scripts.sysmon import mainlib

if __name__ == '__main__':
  mainlib.main()
