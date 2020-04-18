#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Logging related tools."""

import logging
import os
import subprocess
import sys

ROOT_LOGGER = logging.getLogger()
ANNOTATOR_LOGGER = None
CONSOLE_LOGGER = None


class StreamFlushHandler(logging.StreamHandler):
  """Simple stream handler which always flushes the output before outputting."""
  def __init__(self, stream=None, direct_write=False, flush_stream=sys.stdout):
    logging.StreamHandler.__init__(self, stream=stream)
    self._direct_write = direct_write
    self._flush_stream = flush_stream

  def emit(self, record):
    if self._flush_stream:
      self._flush_stream.flush()

    if self._direct_write:
      msg = self.format(record)
      self.stream.write(msg)
    else:
      logging.StreamHandler.emit(self, record)


class AnnotatorFormatter(logging.Formatter):
  """Formatter which can convert annotator messages to regular messages."""
  def __init__(self, enable_annotator):
    logging.Formatter.__init__(self)
    self._enable_annotator = enable_annotator

  def format(self, record):
    message = record.getMessage()
    if not self._enable_annotator:
      message = message.replace('@', '%')

    return message


class LevelBasedFormatter(logging.Formatter):
  """Formatter that can output different formats based on the level."""
  def __init__(self, fmt=None, datefmt=None,
               debug_fmt=None, info_fmt=None, warn_fmt=None, error_fmt=None):
    logging.Formatter.__init__(self, fmt=fmt, datefmt=datefmt)
    self._debug_fmt = debug_fmt
    self._info_fmt = info_fmt
    self._warn_fmt = warn_fmt
    self._error_fmt = error_fmt

  def format(self, record):
    log_format = None
    if record.levelno <= logging.DEBUG:
      log_format = self._debug_fmt
    elif record.levelno <= logging.INFO:
      log_format = self._info_fmt
    elif record.levelno <= logging.WARN:
      log_format = self._warn_fmt
    else:
      log_format = self._error_fmt

    if log_format:
      record.message = record.getMessage()
      return log_format % record.__dict__

    return logging.Formatter.format(self, record)


def SetupLogging(verbose, log_file=None, quiet=False, no_annotator=False):
  """Set up python logging.

  Args:
    verbose: If True, log to stderr at DEBUG level and write subprocess output
             to stdout. Otherwise log to stderr at INFO level and do not print
             subprocess output unless there is an error.
    log_file: If not None, will record the root, annotator, and console logger
              into a single file. File will be overwritten if it already exists.
    quiet: If True, log to stderr at WARNING level only. Only valid if verbose
           is False.
    no_annotator: If True, only emit scrubbed annotator tags to the console.
                  Tags still go to the log.
  """
  # Since one of our handlers always wants debug, set the global level to debug.
  logging.getLogger().setLevel(logging.DEBUG)
  console_handler = logging.StreamHandler(stream=sys.stdout)
  console_handler.setFormatter(
      logging.Formatter(fmt='%(levelname)s: %(message)s'))

  if verbose:
    log_level = logging.DEBUG
  elif quiet:
    log_level = logging.WARN
  else:
    log_level = logging.INFO
  console_handler.setLevel(log_level)
  logging.getLogger().addHandler(console_handler)

  SetupAnnotatorLogger(not no_annotator)
  SetupConsoleLogger(log_level)
  SetupFileLogHandler(log_file)


def SetupAnnotatorLogger(enable_annotator):
  global ANNOTATOR_LOGGER
  ANNOTATOR_LOGGER = logging.getLogger('annotator')
  ANNOTATOR_LOGGER.propagate = False
  ANNOTATOR_LOGGER.setLevel(logging.INFO)

  annotator_formatter = AnnotatorFormatter(enable_annotator)
  annotator_handler = StreamFlushHandler(stream=sys.stderr)
  annotator_handler.setFormatter(annotator_formatter)
  ANNOTATOR_LOGGER.addHandler(annotator_handler)


def SetupConsoleLogger(log_level):
  global CONSOLE_LOGGER
  CONSOLE_LOGGER = logging.getLogger('console')
  CONSOLE_LOGGER.propagate = False
  CONSOLE_LOGGER.setLevel(logging.DEBUG)

  formatter = logging.Formatter(fmt='%(message)s')
  console_handler = StreamFlushHandler(stream=sys.stdout,
                                       direct_write=False,
                                       flush_stream=None)
  console_handler.setFormatter(formatter)
  console_handler.setLevel(log_level)
  CONSOLE_LOGGER.addHandler(console_handler)


def GetConsoleLogger():
  """Returns the console logger or the root logger if not initialized."""
  return CONSOLE_LOGGER or ROOT_LOGGER


def GetAnnotatorLogger():
  """Returns the annotator logger or the console logger if not initialized."""
  return ANNOTATOR_LOGGER or GetConsoleLogger()


def SetupFileLogHandler(log_file=None):
  if log_file:
    file_handler = logging.FileHandler(log_file, mode='w')
    file_handler.setFormatter(
        logging.Formatter(fmt='%(levelname)s: %(message)s'))
    file_handler.setLevel(logging.DEBUG)
    ROOT_LOGGER.addHandler(file_handler)
    ANNOTATOR_LOGGER.addHandler(file_handler)
    CONSOLE_LOGGER.addHandler(file_handler)


def WriteAnnotatorLine(text):
  """Flush stdout and print a message to stderr, also log.

  Buildbot annotator messages must be at the beginning of a line, and we want to
  ensure that any output from the script or from subprocesses appears in the
  correct order wrt BUILD_STEP messages. So we flush stdout before printing all
  buildbot messages here.

  Leading and trailing newlines are added.
  """
  GetAnnotatorLogger().info(text)


def CheckCall(command, stdout=None, logger=None, **kwargs):
  """Modulate command output level based on logging level.

  If a logging file handle is set, always emit all output to it.
  If the log level is set at debug or lower, also emit all output to stdout.
  Otherwise, only emit output on error.
  Args:
    command: Command to run.
    stdout (optional): File name to redirect stdout to.
    **kwargs: Keyword args.
  """
  # If no logger was passed in, default to the console logger.
  if logger is None:
    logger = GetConsoleLogger()

  cwd = os.path.abspath(kwargs.get('cwd', os.getcwd()))
  logger.info('Running: subprocess.check_call(%r, cwd=%r)' % (command, cwd))

  if stdout is None:
    # Interleave stdout and stderr together and log that.
    p = subprocess.Popen(command,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT,
                         **kwargs)
    output = p.stdout
  else:
    p = subprocess.Popen(command,
                         stdout=open(stdout, 'w'),
                         stderr=subprocess.PIPE,
                         **kwargs)
    output = p.stderr

  # Capture the output as it comes and emit it immediately.
  line = output.readline()
  while line:
    logger.info(line.rstrip())
    line = output.readline()

  if p.wait() != 0:
    raise subprocess.CalledProcessError(cmd=command, returncode=p.returncode)

  # Flush stdout so it does not get interleaved with future log or buildbot
  # output which goes to stderr.
  sys.stdout.flush()


def CheckOutput(command, logger=None, **kwargs):
  """Capture stdout from a command, while logging its stderr.

  This is essentially subprocess.check_output, but stderr is
  handled the same way as in log_tools.CheckCall.
  Args:
    command: Command to run.
    **kwargs: Keyword args.
  """
  # If no logger was passed in, default to the console logger.
  if logger is None:
    logger = GetConsoleLogger()

  cwd = os.path.abspath(kwargs.get('cwd', os.getcwd()))
  logger.info('Running: subprocess.check_output(%r, cwd=%r)' % (command, cwd))

  p = subprocess.Popen(command,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE,
                       **kwargs)

  # Assume the output will not be huge or take a long time to come, so it
  # is viable to just consume it all synchronously before logging anything.
  # TODO(mcgrathr): Shovel stderr bits asynchronously if that ever seems
  # worth the hair.
  stdout_text, stderr_text = p.communicate()

  if stderr_text:
    logger.info(stderr_text.rstrip())

  if p.wait() != 0:
    raise subprocess.CalledProcessError(cmd=command, returncode=p.returncode)

  # Flush stdout so it does not get interleaved with future log or buildbot
  # output which goes to stderr.
  sys.stdout.flush()

  logger.info('Result: %r' % stdout_text)
  return stdout_text
