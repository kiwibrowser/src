# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Autofill task automation library.
"""

import abc

# Local imports
from .soft_task import SoftTask


class AutofillTask(SoftTask):
  """Extendable autofill task that provides soft/hard assertion functionality.

  The task consists of a script (list of Actions) that are to be executed when
  run.

  Attributes:
    profile_data: Dict of profile data that acts as the master source for
      validating autofill behaviour.
    debug: Whether debug output should be printed (False if not specified).
  """

  script = []

  def __init__(self, profile_data, debug=False):
    super(AutofillTask, self).__init__()
    self._profile_data = profile_data
    self._debug = debug

  def __str__(self):
    return self.__class__.__name__

  @abc.abstractmethod
  def _create_script(self):
    """Creates a script (list of Actions) to execute.

    Note: Subclasses must implement this method.

    Raises:
      NotImplementedError: Subclass did not implement the method.
    """
    raise NotImplementedError()

  def set_up(self):
    """Sets up the task by creating the action script.

    Raises:
      NotImplementedError: Subclass did not implement _create_script()
    """
    self._create_script()

  def tear_down(self):
    """Tears down the task running environment.

    Any persistent changes made by set_up() must be reversed here.
    """
    pass

  def run(self, driver):
    """Sets up, attempts to execute the task, and always tears down.

    Args:
      driver: ChromeDriver instance to use for action execution.

    Raises:
      Exception: Task execution failed.
    """
    self._driver = driver
    super(AutofillTask, self).run()

  def _run_task(self):
    """Executes the script defined by the subclass.

    Raises:
      Exception: Script execution failed.
    """
    for step in self.script:
      step.Apply(self._driver, self, self._debug)

    self.assert_expectations()

  def profile_data(self, field):
    if field in self._profile_data:
      return self._profile_data[field]
    else:
      return ''
