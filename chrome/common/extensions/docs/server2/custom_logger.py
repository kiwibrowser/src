# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from environment import IsAppEngine


class CustomLogger(object):
  '''Wraps logging methods to include a prefix and flush immediately.
  The flushing is important because logging is often done from jobs
  which may time out, thus losing unflushed logs.
  '''
  def __init__(self, prefix):
    self._prefix = prefix

  def info(self, msg, *args):    self._log(logging.info, msg, args)
  def warning(self, msg, *args): self._log(logging.warning, msg, args)
  def error(self, msg, *args):   self._log(logging.error, msg, args)

  def _log(self, logfn, msg, args):
    try:
      logfn('%s: %s' % (self._prefix, msg), *args)
    finally:
      self.flush()

  if IsAppEngine():
    from google.appengine.api.logservice import logservice
    def flush(self):
      logservice.flush()
  else:
    def flush(self):
      pass
