# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest


class AutofillTestResult(unittest.TextTestResult):
  """A test result class that can print formatted text results to a stream.

  Used by AutofillTestRunner.
  """

  def startTest(self, test):
    """Called when a test is started.
    """
    super(unittest.TextTestResult, self).startTest(test)
    if self.showAll:
      self.stream.write('Running ')
      self.stream.write(self.getDescription(test))
      self.stream.write('\n')
      self.stream.flush()

  def addFailure(self, test, err):
    """Logs a test failure as part of the specified test.

    Overloaded to not include the stack trace.

    Args:
      err: A tuple of values as returned by sys.exc_info().
    """
    err = (None, err[1], None)
    # self.failures.append((test, str(exception)))
    # self._mirrorOutput = True
    super(AutofillTestResult, self).addFailure(test, err)


class AutofillTestRunner(unittest.TextTestRunner):
  """An autofill test runner class that displays results in textual form.

  It prints out the names of tests as they are run, errors as they
  occur, and a summary of the results at the end of the test run.
  """
  resultclass = AutofillTestResult
