# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Commandline entry point for workqueue daemon.

usage:
    workqueue_server [ options ]

The server will serve requests for copies of Chrome OS payload files,
limiting the work to a fixed number of outstanding copies at any one
time.

Available options:

    --logdir: all command logging will be written to a file located in
      this directory.  Old log files will be rotated in this directory.
    --spool:  The spool directory to be used for communication with
      clients.
    --max-tasks:  The maximum number of copies that can be in service
      at any one time.  Work in excess of this number will block until
      the limit can be satisfied.
    --interval:  The polling interval used by the server, in seconds.
    --debug:  In place of copying payloads, requests to the server are
      to sleep according to a parameter passed to the server.
"""


from __future__ import print_function

import argparse
import os
import sys
import time

from chromite.lib import cros_logging as logging
from chromite.lib import ts_mon_config
from chromite.lib.workqueue import copy_handler
from chromite.lib.workqueue import service
from chromite.lib.workqueue import tasks
from chromite.lib.workqueue import throttle
from logging import handlers


_DEFAULT_LOG_DIR = '/var/log/devserver'
_LOG_FILENAME = 'workqueue.log'


def _SleepHandler(request_data):
  """Simple handler for use by the `--debug` option."""
  t0 = time.time()
  time.sleep(float(request_data))
  return (t0, time.time())


def _ParseArguments(argv):
  """Parse the command line arguments."""
  parser = argparse.ArgumentParser(
      prog=os.path.basename(sys.argv[0]),
      description='Server for provision task throttling work queue.')
  parser.add_argument('--logdir', metavar='DIR',
                      default=_DEFAULT_LOG_DIR,
                      help='Directory where logs will be written')
  parser.add_argument('--spool', metavar='DIR',
                      default=throttle.DEFAULT_SPOOL_DIR,
                      help='Spool directory for the work queue')
  parser.add_argument('--max-tasks', type=int, metavar='NUM',
                      default=4,
                      help='Maxiumum allowed running tasks')
  parser.add_argument('--interval', type=float, metavar='SECONDS',
                      default=2.0,
                      help='Wait time between polling ticks')
  parser.add_argument('--debug', action='store_true',
                      help='Use a debug service instead of the copy service')
  return parser.parse_args(argv)


def _SetupLogging(options):
  """Set up default logging for the command.

  Removes any pre-installed logging handlers, and installs a single
  `TimedRotatingFileHandler` that will write to the default log file
  in the log directory specified on the command line.

  Args:
    options: Results of parsing the command line; used to obtain the
      log directory path.
  """
  logger = logging.getLogger()
  for h in list(logger.handlers):
    logger.removeHandler(h)
  logfile = os.path.join(options.logdir, _LOG_FILENAME)
  handler = handlers.TimedRotatingFileHandler(logfile, when='W4',
                                              backupCount=13)
  handler.setFormatter(copy_handler.LOG_FORMATTER)
  logger.setLevel(logging.DEBUG)
  logger.addHandler(handler)


def _GetTaskHandler(options):
  """Get the task handler function, based on `options.debug`."""
  return (copy_handler.CopyPayload
          if not options.debug else _SleepHandler)


def main(argv):
  options = _ParseArguments(argv)
  manager = tasks.ProcessPoolTaskManager(options.max_tasks,
                                         _GetTaskHandler(options),
                                         options.interval)
  queue = service.WorkQueueServer(options.spool)
  try:
    # The ordering for logging setup here matters (alas):
    # The `ts_mon` setup starts a subprocess that makes logging
    # calls, and TimedRotatingFileHandler isn't multiprocess safe.
    # So, we need for the `ts_mon` child and this process to write
    # to different logs.  The gory details are in crbug.com/774597.
    #
    # This is a hack, really.  If you're studying this comment
    # because you have to clean up my mess, I'm truly and profoundly
    # sorry.  But still I wouldn't change a thing...
    #     https://www.youtube.com/watch?v=fFtGfyruroU

    with ts_mon_config.SetupTsMonGlobalState(
        'provision_workqueue', indirect=True):
      _SetupLogging(options)
      logging.info('Work queue service starts')
      logging.info('  Spool dir is %s', options.spool)
      logging.info('  Maximum of %d concurrent tasks', options.max_tasks)
      logging.info('  Time per tick is %.3f seconds', options.interval)
      queue.ProcessRequests(manager)
  except KeyboardInterrupt:
    pass
  finally:
    manager.Close()
