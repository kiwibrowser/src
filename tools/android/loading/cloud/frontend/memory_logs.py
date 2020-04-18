# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from StringIO import StringIO


class MemoryLogs(object):
  """Collects logs in memory."""

  def __init__(self, logger):
    self._logger = logger
    self._log_buffer = StringIO()
    self._log_handler = logging.StreamHandler(self._log_buffer)
    formatter = logging.Formatter("[%(asctime)s][%(levelname)s] %(message)s",
                                  "%y-%m-%d %H:%M:%S")
    self._log_handler.setFormatter(formatter)

  def Start(self):
    """Starts collecting the logs."""
    self._logger.addHandler(self._log_handler)

  def Flush(self):
    """Stops collecting the logs and returns the logs collected since Start()
    was called.
    """
    self._logger.removeHandler(self._log_handler)
    self._log_handler.flush()
    self._log_buffer.flush()
    result = self._log_buffer.getvalue()
    self._log_buffer.truncate(0)
    return result

