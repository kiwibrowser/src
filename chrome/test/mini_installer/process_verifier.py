# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import chrome_helper
import verifier


class ProcessVerifier(verifier.Verifier):
  """Verifies that the running processes match the expectation dictionaries."""

  def _VerifyExpectation(self, expectation_name, expectation,
                         variable_expander):
    """Overridden from verifier.Verifier.

    This method will throw an AssertionError if process state doesn't match the
    |expectation|.

    Args:
      expectation_name: Path to the process being verified. It is expanded using
          Expand.
      expectation: A dictionary with the following key and value:
          'running' a boolean indicating whether the process should be running.
      variable_expander: A VariableExpander object.
    """
    # Create a list of paths of all running processes.
    running_process_paths = [path for (_, path) in
                             chrome_helper.GetProcessIDAndPathPairs()]
    process_path = variable_expander.Expand(expectation_name)
    is_running = process_path in running_process_paths
    assert expectation['running'] == is_running, \
        ('Process %s is running' % process_path) if is_running else \
        ('Process %s is not running' % process_path)
