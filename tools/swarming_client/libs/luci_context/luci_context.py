# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Implements a client library for reading and writing LUCI_CONTEXT compatible
files.

Due to arcane details of the UNIX process model and environment variables, this
library is unfortunately NOT threadsafe; there's no way to have multiple
LUCI_CONTEXTS live in a process safely at the same time. As such, this library
will raise an exception if any attempt is made to use it improperly (for example
by having multiple threads call 'write' at the same time).

See ../LUCI_CONTEXT.md for details on the LUCI_CONTEXT concept/protocol."""

import contextlib
import copy
import json
import logging
import os
import sys
import tempfile
import threading

_LOGGER = logging.getLogger(__name__)

# _ENV_KEY is the environment variable that we look for to find out where the
# LUCI context file is.
_ENV_KEY = 'LUCI_CONTEXT'

# _CUR_CONTEXT contains the cached LUCI Context that is currently available to
# read. A value of None indicates that the value has not yet been populated.
_CUR_CONTEXT = None
_CUR_CONTEXT_LOCK = threading.Lock()

# Write lock is a recursive mutex which is taken when using the write() method.
# This allows the same thread to
_WRITE_LOCK = threading.RLock()


@contextlib.contextmanager
def _tf(data, data_raw=False, workdir=None):
  tf = tempfile.NamedTemporaryFile(prefix='luci_ctx.', suffix='.json',
                                   delete=False, dir=workdir)
  _LOGGER.debug('Writing LUCI_CONTEXT file %r', tf.name)
  try:
    if not data_raw:
      json.dump(data, tf)
    else:
      # for testing, allows malformed json
      tf.write(data)
    tf.close()  # close it so that winders subprocesses can read it.
    yield tf.name
  finally:
    try:
      os.unlink(tf.name)
    except OSError as ex:
      _LOGGER.error(
        'Failed to delete written LUCI_CONTEXT file %r: %s', tf.name, ex)


def _to_utf8(obj):
  if isinstance(obj, dict):
    return {_to_utf8(key): _to_utf8(value) for key, value in obj.iteritems()}
  if isinstance(obj, list):
    return [_to_utf8(item) for item in obj]
  if isinstance(obj, unicode):
    return obj.encode('utf-8')
  return obj


class MultipleLUCIContextException(Exception):
  def __init__(self):
    super(MultipleLUCIContextException, self).__init__(
      'Attempted to write LUCI_CONTEXT in multiple threads')


def _check_ok(data):
  if not isinstance(data, dict):
    _LOGGER.error(
      'LUCI_CONTEXT does not contain a dict: %s', type(data).__name__)
    return False

  bad = False
  for k, v in data.iteritems():
    if not isinstance(v, dict):
      bad = True
      _LOGGER.error(
        'LUCI_CONTEXT[%r] is not a dict: %s', k, type(v).__name__)

  return not bad


# this is a separate function from _read_full for testing purposes.
def _initial_load():
  global _CUR_CONTEXT
  to_assign = {}

  ctx_path = os.environ.get(_ENV_KEY)
  if ctx_path:
    ctx_path = ctx_path.decode(sys.getfilesystemencoding())
    _LOGGER.debug('Loading LUCI_CONTEXT: %r', ctx_path)
    try:
      with open(ctx_path, 'r') as f:
        loaded = _to_utf8(json.load(f))
        if _check_ok(loaded):
          to_assign = loaded
    except OSError as ex:
      _LOGGER.error('LUCI_CONTEXT failed to open: %s', ex)
    except IOError as ex:
      _LOGGER.error('LUCI_CONTEXT failed to read: %s', ex)
    except ValueError as ex:
      _LOGGER.error('LUCI_CONTEXT failed to decode: %s', ex)

  _CUR_CONTEXT = to_assign


def _read_full():
  # double-check because I'm a hopeless diehard.
  if _CUR_CONTEXT is None:
    with _CUR_CONTEXT_LOCK:
      if _CUR_CONTEXT is None:
        _initial_load()
  return _CUR_CONTEXT


def _mutate(section_values):
  new_val = read_full()
  for section, value in section_values.iteritems():
    if value is None:
      new_val.pop(section, None)
    elif isinstance(value, dict):
      new_val[section] = value
    else:
      raise ValueError(
        'Bad type for LUCI_CONTEXT[%r]: %s', section, type(value).__name__)
  return new_val


def read_full():
  """Returns a copy of the entire current contents of the LUCI_CONTEXT as
  a dict.
  """
  return copy.deepcopy(_read_full())


def read(section_key):
  """Reads from the given section key. Returns the data in the section or None
  if the data doesn't exist.

  Args:
    section_key (str) - The top-level key to read from the LUCI_CONTEXT.

  Returns:
    A copy of the requested section data (as a dict), or None if the section was
    not present.

  Example:
    Given a LUCI_CONTEXT of:
      {
        "swarming": {
          "secret_bytes": <bytes>
        },
        "other_service": {
          "nested": {
            "key": "something"
          }
        }
      }

    read('swarming') -> {'secret_bytes': <bytes>}
    read('doesnt_exist') -> None
  """
  return copy.deepcopy(_read_full().get(section_key, None))


@contextlib.contextmanager
def write(_tmpdir=None, **section_values):
  """Write is a contextmanager which will write all of the provided section
  details to a new context, copying over the values from any unmentioned
  sections. The new context file will be set in os.environ. When the
  contextmanager exits, it will attempt to delete the context file.

  Since each call to write produces a new context file on disk, it's beneficial
  to group edits together into a single call to write when possible.

  Calls to read*() within the context of a call to write will read from the
  written value. This written value is stored on a per-thread basis.

  NOTE: Because environment variables are per-process and not per-thread, it is
  an error to call write() from multiple threads simultaneously. If this is
  done, this function raises an exception.

  Args:
    _tmpdir (str) - an optional directory to use for the newly written
      LUCI_CONTEXT file.
    section_values (str -> value) - A mapping of section_key to the new value
      for that section. A value of None will remove that section. Non-None
      values must be of the type 'dict', and must be json serializable.

  Raises:
    MultipleLUCIContextException if called from multiple threads
    simulataneously.

  Example:
    Given a LUCI_CONTEXT of:
      {
        "swarming": {
          "secret_bytes": <bytes>
        },
        "other_service": {
          ...
        }
      }

    with write(swarming=None): ...    # deletes 'swarming'
    with write(something={...}): ...  # sets 'something' section to {...}
  """
  # If there are no edits, just pass-through
  if not section_values:
    yield
    return

  new_val = _mutate(section_values)

  global _CUR_CONTEXT
  got_lock = _WRITE_LOCK.acquire(blocking=False)
  if not got_lock:
    raise MultipleLUCIContextException()
  try:
    with _tf(new_val, workdir=_tmpdir) as name:
      try:
        old_value = _CUR_CONTEXT
        old_envvar = os.environ.get(_ENV_KEY, None)

        os.environ[_ENV_KEY] = name.encode(sys.getfilesystemencoding())
        _CUR_CONTEXT = new_val
        yield
      finally:
        _CUR_CONTEXT = old_value
        if old_envvar is None:
          del os.environ[_ENV_KEY]
        else:
          os.environ[_ENV_KEY] = old_envvar
  finally:
    _WRITE_LOCK.release()


@contextlib.contextmanager
def stage(_tmpdir=None, **section_values):
  """Prepares and writes new LUCI_CONTEXT file, but doesn't replace the env var.

  This is useful when launching new process asynchronously in new LUCI_CONTEXT
  environment. In this case, modifying the environment of the current process
  (like 'write' does) may be harmful.

  Calls the body with a path to the new LUCI_CONTEXT file or None if
  'section_values' is empty (meaning, no changes have been made).
  """
  if not section_values:
    yield None
    return
  with _tf(_mutate(section_values), workdir=_tmpdir) as name:
    yield name
