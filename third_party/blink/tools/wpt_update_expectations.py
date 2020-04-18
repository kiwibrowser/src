#!/usr/bin/env vpython
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

from blinkpy.common import host
from blinkpy.w3c.wpt_expectations_updater import WPTExpectationsUpdater


if __name__ == "__main__":
    updater = WPTExpectationsUpdater(host.Host())
    sys.exit(updater.run(sys.argv[1:]))
