# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.buildbot.builder import Builders
from common.buildbot.slave import Slaves


PENDING = None
SUCCESS = 0
WARNING = 1
FAILURE = 2
EXCEPTION = 4
SLAVE_LOST = 5
