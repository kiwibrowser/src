# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Sleep loop."""

from __future__ import absolute_import
from __future__ import print_function

import time

from chromite.lib import cros_logging as logging

logger = logging.getLogger(__name__)


class SleepLoop(object):
  """Sleep loop."""

  def __init__(self, callback, interval=60):
    """Initialize instance.

    Args:
      callback: Function to call on each loop.
      interval: Time between loops in seconds.
    """
    self._callback = callback
    self._interval = interval

  def loop_once(self):
    """Do actions for a single loop."""
    try:
      self._callback()
    except Exception:
      logger.exception(u'Error during loop.')

  def loop_forever(self):
    while True:
      self.loop_once()
      _force_sleep(self._interval)


def _force_sleep(secs):
  """Force sleep for at least the given number of seconds."""
  now = time.time()
  finished_time = now + secs
  while now < finished_time:
    remaining = finished_time - now
    logger.debug(u'Sleeping for %d, %d remaining', secs, remaining)
    time.sleep(remaining)
    now = time.time()
