#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Legion-based comm server test."""

import argparse
import logging
import os
import sys

# Map the testing directory so we can import legion.legion_test_case.
TESTING_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    '..', '..', '..', '..', 'testing')
sys.path.append(TESTING_DIR)

from legion import legion_test_case
from legion.lib import common_lib


class CommServerTestController(legion_test_case.TestCase):
  """A simple example controller for a test."""

  @classmethod
  def CreateTestTask(cls):
    """Create a new task."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--task-hash')
    parser.add_argument('--os', default='Ubuntu-14.04')
    args, _ = parser.parse_known_args()

    task = cls.CreateTask(
        isolated_hash=args.task_hash,
        dimensions={'os': args.os, 'pool': 'default'},
        idle_timeout_secs=90,
        connection_timeout_secs=90,
        verbosity=logging.DEBUG)
    task.Create()
    return task

  @classmethod
  def setUpClass(cls):
    """Creates the task machines and waits until they connect."""
    cls.task = cls.CreateTestTask()
    cls.task.WaitForConnection()

  def testCommServerTest(self):
    cmd = [
        'python',
        'task.py',
        '--address', str(common_lib.MY_IP),
        '--port', str(self.comm_server.port)
        ]
    process = self.task.Process(cmd)
    process.Wait()
    retcode = process.GetReturncode()
    if retcode != 0:
      logging.info('STDOUT:\n%s', process.ReadStdout())
      logging.info('STDERR:\n%s', process.ReadStderr())
    self.assertEqual(retcode, 0)
    # Add a success logging statement to make the logs a little easier to read.
    logging.info('Success')


if __name__ == '__main__':
  legion_test_case.main()
