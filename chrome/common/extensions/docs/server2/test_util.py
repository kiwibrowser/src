# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function

from extensions_paths import SERVER2
import logging
import os
import sys


def CaptureLogging(f):
  '''Call the function |f|, capturing any logging output generated. |f| must
  take no arguments. Returns a list of LogRecords that were emitted.
  '''
  output = []
  class Capture(object):
    def filter(self, record):
      output.append(record)

  cf = Capture()
  logging.getLogger('').addFilter(cf)
  f()
  logging.getLogger('').removeFilter(cf)

  return output


def EnableLogging(name):
  '''Returns the output of the log with |name| to stdout.
  '''

  return _ReplaceLogging(name, lambda message, *args: print(message % args))


def DisableLogging(name):
  '''Disables the log with |name| for the duration of the decorated function.
  '''
  return _ReplaceLogging(name, lambda _, *args: None)


def _ReplaceLogging(name, replacement):
  def decorator(fn):
    def impl(*args, **optargs):
      saved = getattr(logging, name)
      setattr(logging, name, replacement)
      try:
        return fn(*args, **optargs)
      finally:
        setattr(logging, name, saved)
    return impl
  return decorator


def ChromiumPath(*path):
  abspath = os.path.join(
      sys.path[0], '..', '..', '..', '..', '..', *path)
  # os.path.relpath kills any trailing '/'.
  return os.path.relpath(abspath) + ('/' if abspath.endswith('/') else '')


def Server2Path(*path):
  return ChromiumPath(SERVER2, *path)


def ReadFile(*path, **read_args):
  with open(ChromiumPath(*path), **read_args) as f:
    return f.read()
