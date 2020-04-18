# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import json
import logging
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time


def VerboseCompileRegexOrAbort(regex):
  """Compiles a user-provided regular expression, exits the program on error."""
  try:
    return re.compile(regex)
  except re.error as e:
    sys.stderr.write('invalid regex: {}\n{}\n'.format(regex, e))
    sys.exit(2)


def PollFor(condition, condition_name, interval=5):
  """Polls for a function to return true.

  Args:
    condition: Function to wait its return to be True.
    condition_name: The condition's name used for logging.
    interval: Periods to wait between tries in seconds.

  Returns:
    What condition has returned to stop waiting.
  """
  while True:
    result = condition()
    logging.info('Polling condition %s is %s' % (
        condition_name, 'met' if result else 'not met'))
    if result:
      return result
    time.sleep(interval)


def SerializeAttributesToJsonDict(json_dict, instance, attributes):
  """Adds the |attributes| from |instance| to a |json_dict|.

  Args:
    json_dict: (dict) Dict to update.
    instance: (object) instance to take the values from.
    attributes: ([str]) List of attributes to serialize.

  Returns:
    json_dict
  """
  json_dict.update({attr: getattr(instance, attr) for attr in attributes})
  return json_dict


def DeserializeAttributesFromJsonDict(json_dict, instance, attributes):
  """Sets a list of |attributes| in |instance| according to their value in
    |json_dict|.

  Args:
    json_dict: (dict) Dict containing values dumped by
               SerializeAttributesToJsonDict.
    instance: (object) instance to modify.
    attributes: ([str]) List of attributes to set.

  Raises:
    AttributeError if one of the attribute doesn't exist in |instance|.

  Returns:
    instance
  """
  for attr in attributes:
    getattr(instance, attr) # To raise AttributeError if attr doesn't exist.
    setattr(instance, attr, json_dict[attr])
  return instance


@contextlib.contextmanager
def TemporaryDirectory(suffix='', prefix='tmp'):
  """Returns a freshly-created directory that gets automatically deleted after
  usage.
  """
  name = tempfile.mkdtemp(suffix=suffix, prefix=prefix)
  try:
    yield name
  finally:
    shutil.rmtree(name)


def EnsureParentDirectoryExists(path):
  """Verifies that the parent directory exists or creates it if missing."""
  parent_directory_path = os.path.abspath(os.path.dirname(path))
  if not os.path.isdir(parent_directory_path):
    os.makedirs(parent_directory_path)


def GetCommandLineForLogging(cmd, env_diff=None):
  """Get command line string.

  Args:
    cmd: Command line argument
    env_diff: Environment modification for the command line.

  Returns:
    Command line string.
  """
  cmd_str = ''
  if env_diff:
    for key, value in env_diff.iteritems():
      cmd_str += '{}={} '.format(key, value)
  return cmd_str + subprocess.list2cmdline(cmd)


# TimeoutError inherit from BaseException to pass through DevUtils' retries
# decorator that catches only exceptions inheriting from Exception.
class TimeoutError(BaseException):
  pass


# If this exception is ever raised, then might be better to replace this
# implementation with Thread.join(timeout=XXX).
class TimeoutCollisionError(Exception):
  pass


@contextlib.contextmanager
def TimeoutScope(seconds, error_name):
  """Raises TimeoutError if the with statement is finished within |seconds|."""
  assert seconds > 0
  def _signal_callback(signum, frame):
    del signum, frame # unused.
    raise TimeoutError(error_name)

  try:
    signal.signal(signal.SIGALRM, _signal_callback)
    if signal.alarm(seconds) != 0:
      raise TimeoutCollisionError(
          'Discarding an alarm that was scheduled before.')
    yield
  finally:
    signal.alarm(0)
    if signal.getsignal(signal.SIGALRM) != _signal_callback:
      raise TimeoutCollisionError('Looks like there is a signal.signal(signal.'
          'SIGALRM) made within the with statement.')
