# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import unittest

from selenium.common.exceptions import TimeoutException

# Local Imports
from autofill_task.exceptions import ExpectationFailure
from .flow import AutofillTestFlow

class AutofillTestCase(unittest.TestCase):
  """Wraps a single autofill test flow for use with the unittest library.

    task_class: AutofillTask to use for the test.
    profile: Dict of profile data that acts as the master source for
      validating autofill behaviour.
    debug: Whether debug output should be printed (False if not specified).
  """
  def __init__(self, task_class, user_data_dir, profile, chrome_binary=None,
               debug=False):
    super(AutofillTestCase, self).__init__('run')
    self._flow = AutofillTestFlow(task_class, profile, debug=debug)
    self._user_data_dir = user_data_dir
    self._chrome_binary = chrome_binary
    self._debug = debug

  def __str__(self):
    return str(self._flow)

  def run(self, result):
    result.startTest(self)

    try:
      self._flow.run(self._user_data_dir, chrome_binary=self._chrome_binary)
    except KeyboardInterrupt:
      raise
    except (TimeoutException, ExpectationFailure):
      result.addFailure(self, sys.exc_info())
    except:
      result.addError(self, sys.exc_info())
    else:
      result.addSuccess(self)
    finally:
      result.stopTest(self)
