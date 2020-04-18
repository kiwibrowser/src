# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Basic infrastructure for implementing retries."""

from __future__ import print_function

import functools
import random
import sys
import time

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging


def _CreateExceptionRetryHandler(exception):
  """Returns a retry handler for given exception(s).

  Please see WithRetry class document for details.
  """
  if not (isinstance(exception, type) and issubclass(exception, Exception) or
          (isinstance(exception, tuple) and
           all(issubclass(e, Exception) for e in exception))):
    raise TypeError('exceptions should be an exception (or tuple), not %r' %
                    exception)
  return lambda exc: isinstance(exc, exception)


class _RetryDelayStrategy(object):
  """The strategy of the delay between each retry attempts.

  Please see WithRetry class document for details.
  """

  def __init__(self, sleep=0, backoff_factor=1, jitter=0):
    if sleep < 0:
      raise ValueError('sleep must be >= 0: %s' % sleep)

    if backoff_factor < 1:
      raise ValueError('backoff_factor must be 1 or greater: %s'
                       % backoff_factor)

    if jitter < 0:
      raise ValueError('jitter must be >= 0: %s' % jitter)

    self._sleep = sleep
    self._backoff_factor = backoff_factor
    self._jitter = jitter

  def Sleep(self, attempt):
    """Sleep to delay the current retry."""
    assert attempt >= 1, 'Expect attempt is always positive: %s' % attempt
    if self._backoff_factor > 1:
      sleep_duration = self._sleep * self._backoff_factor ** (attempt - 1)
    else:
      sleep_duration = self._sleep * attempt

    # If |jitter| is set, add a random jitter sleep.
    jitter = random.uniform(.5 * self._jitter, 1.5 * self._jitter)
    total = sleep_duration + jitter
    if total:
      logging.debug('Retrying in %f (%f + jitter %f) seconds ...',
                    total, sleep_duration, jitter)
      time.sleep(total)


class WithRetry(object):
  """Decorator to handle retry on exception.

  Example:

  @WithRetry(max_retry=3):
  def _run():
    ... do something ...
  _run()

  If _run() raises an exception, it retries at most three times.

  Retrying strategy.

  If the decorated function throws an Exception instance, then this class
  checks whether the retry should be continued or not based on the given
  |handler| or |exception| as follows.
  - If |handler| is given, which should be a callback which takes an exception
    and returns bool, calls it with the thrown exception.
    If the |handler| returns True, retry will be continued. Otherwise no
    further retry will be made, and an exception will be raised.
  - If |exception| is given, which is an exception class or a tuple of
    exception classes, iff the thrown exception is a instance of the given
    exception class(es) (or its subclass), continues to retry. Otherwise no
    further retry will be made, and an exception will be raised.
  - If neither is given, just continues to retry on any Exception instance.
  - Note: it is not allowed to specify both |handler| and |exception| at once.

  Delay strategy.

  Between for each attempt, some delay can be set, as follows.
  - If |sleep| is given, the delay between the first and second attempts is
    |sleep| secs.
  - The delay between the second and third attempts, and later, depends on
    |sleep| and |backoff_factor|.
    - If |backoff_factor| is not given, the delay will be linearly increased,
      as |sleep| * (number of attempts). E.g., if |sleep| is 1, the delays
      will be 1, 2, 3, 4, 5, ... and so on.
    - If |backoff_factor| is given, the delay will be exponentially increased,
      as |sleep| * |backoff_factor| ** (number of attempts - 1). E.g., if
      |sleep| is 1, and |backoff_factor| is 2, the delay will be,
      1, 2, 4, 8, 16, ... and so on
  - Note: Keep in mind that, if |backoff_factor| is not given, the total
    delay time will be triangular value of |max_retry| multiplied by the
    |sleep| value. E.g., |max_retry| is 5, and |sleep| is 10, will be
    T5 (i.e. 5 + 4 + 3 + 2 + 1) times 10 = 150 seconds total. Rather than
    use a large sleep value, you should lean more towards large retries
    and lower sleep intervals, or by utilizing |backoff_factor|.
  - In addition, for each delay, random duration of the delay can be added,
    as 'jitter'. (Often, this helps to avoid consecutive conflicting situation)
    |jitter| is specifies the duration of jitter delay, randomized up to
    50% in either direction.

  Parameter details are as follows:

  max_retry: A positive integer representing how many times to retry the
    command before giving up.  Worst case, the command is invoked
    (max_retry + 1) times before failing.
  handler, exception: Please see above for details.
  log_all_retries: when True, logs all retries.
  sleep, backoff_factor, jitter: Please see above for details.
  raise_first_exception_on_failure: determines which excecption is raised upon
    failure after retries. If True, the first exception that was encountered.
    Otherwise, the final one.
  exception_to_raise: Optional exception type. If given, raises its instance,
    instead of the one raised from the retry body.
  status_callback: Optional callback invoked after each call of |functor|. It
    takes two arguments: |attempt| which is the index of the last attempt
    (0-based), and |success| representing whether the last attempt was
    successfully done or not. If the callback raises an exception, no further
    retry will be made, and the exception will be propagated to the caller.
  """

  def __init__(self,
               max_retry, handler=None, exception=None, log_all_retries=False,
               sleep=0, backoff_factor=1, jitter=0,
               raise_first_exception_on_failure=True, exception_to_raise=None,
               status_callback=None):
    if max_retry < 0:
      raise ValueError('max_retry needs to be zero or more: %d' % max_retry)
    self._max_retry = max_retry

    if handler is not None and exception is not None:
      raise ValueError('handler and exception cannot be specified at once')
    self._handler = (
        handler or _CreateExceptionRetryHandler(exception or Exception))

    self._log_all_retries = log_all_retries
    self._retry_delay = _RetryDelayStrategy(sleep, backoff_factor, jitter)
    self._raise_first_exception_on_failure = raise_first_exception_on_failure
    self._exception_to_raise = exception_to_raise
    self._status_callback = status_callback or (lambda attempt, success: None)

  def __call__(self, func):
    @functools.wraps(func)
    def _Wrapper(*args, **kwargs):
      fname = getattr(func, '__qualname__',
                      getattr(func, '__name__', '<nameless>'))
      exc_info = None
      for attempt in xrange(self._max_retry + 1):
        if attempt:
          self._retry_delay.Sleep(attempt)

        if attempt and self._log_all_retries:
          logging.debug('Retrying %s (attempt %d)', fname, attempt + 1)

        try:
          ret = func(*args, **kwargs)
        except Exception as e:
          # Note we're not snagging BaseException, so
          # MemoryError/KeyboardInterrupt and friends don't enter this except
          # block.

          # If raise_first_exception_on_failure, we intentionally ignore
          # any failures in later attempts since we'll throw the original
          # failure if all retries fail.
          if exc_info is None or not self._raise_first_exception_on_failure:
            exc_info = sys.exc_info()

          try:
            self._status_callback(attempt, False)
          except Exception:
            # In case callback raises an exception, quit the retry.
            # For further investigation, log the original exception here.
            logging.error('Ending retry due to Exception raised by a callback. '
                          'Original exception raised during the attempt is '
                          'as follows: ',
                          exc_info=exc_info)
            # Reraise the exception raised from the status_callback.
            raise

          if not self._handler(e):
            logging.debug('ending retries with error: %s(%s)', e.__class__, e)
            break
          logging.debug('%s(%s)', e.__class__, e)
        else:
          # Run callback in outside of try's main block, in order to avoid
          # accidental capture of an Exception which may be raised in callback.
          self._status_callback(attempt, True)
          return ret

      # Did not return, meaning all attempts failed. Raise the exception.
      if self._exception_to_raise:
        raise self._exception_to_raise('%s: %s' % (exc_info[0], exc_info[1]))
      raise exc_info[0], exc_info[1], exc_info[2]
    return _Wrapper


def GenericRetry(handler, max_retry, functor, *args, **kwargs):
  """Generic retry loop w/ optional break out depending on exceptions.

  Runs functor(*args, **(kwargs excluding params for retry)) as a retry body.

  Please see WithRetry for details about retrying parameters.
  """
  # Note: the default value needs to be matched with the ones of WithRetry's
  # ctor.
  log_all_retries = kwargs.pop('log_all_retries', False)
  delay_sec = kwargs.pop('delay_sec', 0)
  sleep = kwargs.pop('sleep', 0)
  backoff_factor = kwargs.pop('backoff_factor', 1)
  status_callback = kwargs.pop('status_callback', None)
  raise_first_exception_on_failure = kwargs.pop(
      'raise_first_exception_on_failure', True)
  exception_to_raise = kwargs.pop('exception_to_raise', None)

  @WithRetry(
      max_retry=max_retry, handler=handler, log_all_retries=log_all_retries,
      sleep=sleep, backoff_factor=backoff_factor, jitter=delay_sec,
      raise_first_exception_on_failure=raise_first_exception_on_failure,
      exception_to_raise=exception_to_raise,
      status_callback=status_callback)
  def _run():
    return functor(*args, **kwargs)
  return _run()


def RetryException(exception, max_retry, functor, *args, **kwargs):
  """Convenience wrapper for GenericRetry based on exceptions.

  Runs functor(*args, **(kwargs excluding params for retry)) as a retry body.

  Please see WithRetry for details about retrying parameters.
  """
  log_all_retries = kwargs.pop('log_all_retries', False)
  delay_sec = kwargs.pop('delay_sec', 0)
  sleep = kwargs.pop('sleep', 0)
  backoff_factor = kwargs.pop('backoff_factor', 1)
  status_callback = kwargs.pop('status_callback', None)
  raise_first_exception_on_failure = kwargs.pop(
      'raise_first_exception_on_failure', True)
  exception_to_raise = kwargs.pop('exception_to_raise', None)

  @WithRetry(
      max_retry=max_retry, exception=exception,
      log_all_retries=log_all_retries,
      sleep=sleep, backoff_factor=backoff_factor, jitter=delay_sec,
      raise_first_exception_on_failure=raise_first_exception_on_failure,
      exception_to_raise=exception_to_raise,
      status_callback=status_callback)
  def _run():
    return functor(*args, **kwargs)
  return _run()


def RetryCommand(functor, max_retry, *args, **kwargs):
  """Wrapper for RunCommand that will retry a command

  Args:
    functor: RunCommand function to run; retries will only occur on
      RunCommandError exceptions being thrown.
    max_retry: A positive integer representing how many times to retry
      the command before giving up.  Worst case, the command is invoked
      (max_retry + 1) times before failing.
    sleep: Optional keyword.  Multiplier for how long to sleep between
      retries; will delay (1*sleep) the first time, then (2*sleep),
      continuing via attempt * sleep.
    retry_on: If provided, we will retry on any exit codes in the given list.
      Note: A process will exit with a negative exit code if it is killed by a
      signal. By default, we retry on all non-negative exit codes.
    error_check: Optional callback to check the error output.  Return None to
      fall back to |retry_on|, or True/False to set the retry directly.
    log_retries: Whether to log a warning when retriable errors occur.
    args: Positional args passed to RunCommand; see RunCommand for specifics.
    kwargs: Optional args passed to RunCommand; see RunCommand for specifics.

  Returns:
    A CommandResult object.

  Raises:
    Exception:  Raises RunCommandError on error with optional error_message.
  """
  values = kwargs.pop('retry_on', None)
  error_check = kwargs.pop('error_check', lambda x: None)
  log_retries = kwargs.pop('log_retries', True)

  def ShouldRetry(exc):
    """Return whether we should retry on a given exception."""
    if not ShouldRetryCommandCommon(exc):
      return False
    if values is None and exc.result.returncode < 0:
      logging.info('Child process received signal %d; not retrying.',
                   -exc.result.returncode)
      return False

    ret = error_check(exc)
    if ret is not None:
      return ret

    if values is None or exc.result.returncode in values:
      if log_retries:
        logging.warning('Command failed with retriable error.\n%s', exc)
      return True
    return False

  return GenericRetry(ShouldRetry, max_retry, functor, *args, **kwargs)


def ShouldRetryCommandCommon(exc):
  """Returns whether any RunCommand should retry on a given exception."""
  if not isinstance(exc, cros_build_lib.RunCommandError):
    return False
  if exc.result.returncode is None:
    logging.error('Child process failed to launch; not retrying:\n'
                  'command: %s', exc.result.cmdstr)
    return False
  return True


def RunCommandWithRetries(max_retry, *args, **kwargs):
  """Wrapper for RunCommand that will retry a command

  Args:
    max_retry: See RetryCommand and RunCommand.
    *args: See RetryCommand and RunCommand.
    **kwargs: See RetryCommand and RunCommand.

  Returns:
    A CommandResult object.

  Raises:
    Exception:  Raises RunCommandError on error with optional error_message.
  """
  return RetryCommand(cros_build_lib.RunCommand, max_retry, *args, **kwargs)


class DownloadError(Exception):
  """Fetching file via curl failed"""


def RunCurl(curl_args, *args, **kwargs):
  """Runs curl and wraps around all necessary hacks.

  Args:
    curl_args: Command line to pass to curl. Must be list of str.
    *args, **kwargs: See RunCommandWithRetries and RunCommand.
      Note that retry_on, error_check, sleep, backoff_factor cannot be
      overwritten.

  Returns:
    A CommandResult object.

  Raises:
    DownloadError: Whenever curl fails for any reason.
  """
  cmd = ['curl'] + curl_args

  # These values were discerned via scraping the curl manpage; they're all
  # retry related (dns failed, timeout occurred, etc, see  the manpage for
  # exact specifics of each).
  # Note we allow 22 to deal w/ 500's- they're thrown by google storage
  # occasionally.  This is also thrown when getting 4xx, but curl doesn't
  # make it easy to differentiate between them.
  # Note we allow 35 to deal w/ Unknown SSL Protocol error, thrown by
  # google storage occasionally.
  # Finally, we do not use curl's --retry option since it generally doesn't
  # actually retry anything; code 18 for example, it will not retry on.
  retriable_exits = frozenset([5, 6, 7, 15, 18, 22, 26, 28, 35, 52, 56])

  def _CheckExit(exc):
    """Filter out specific error codes when getting exit 22

    Curl will exit(22) for a wide range of HTTP codes -- both the 4xx and 5xx
    set.  For the 4xx, we don't want to retry.  We have to look at the output.
    """
    if exc.result.returncode == 22:
      return '404 Not Found' not in exc.result.error

    # We'll let the common exit code filter do the right thing.
    return None

  try:
    return RunCommandWithRetries(
        10, cmd, retry_on=retriable_exits, error_check=_CheckExit,
        sleep=3, backoff_factor=1.6, *args, **kwargs)
  except cros_build_lib.RunCommandError as e:
    if e.result.returncode in (51, 58, 60):
      # These are the return codes of failing certs as per 'man curl'.
      raise DownloadError(
          'Download failed with certificate error? Try "sudo c_rehash".')
    raise DownloadError('Curl failed w/ exit code %i: %s' %
                        (e.result.returncode, e.result.error))
