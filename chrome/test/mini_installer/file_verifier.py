# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import verifier


class FileVerifier(verifier.Verifier):
  """Verifies that the current files match the expectation dictionaries."""

  def _VerifyExpectation(self, expectation_name, expectation,
                         variable_expander):
    """Overridden from verifier.Verifier.

    This method will throw an AssertionError if file state doesn't match the
    |expectation|.

    Args:
      expectation_name: Path to the file being verified. It is expanded using
          Expand.
      expectation: A dictionary with the following key and value:
          'exists' a boolean indicating whether the file should exist.
      variable_expander: A VariableExpander object.
    """
    file_path = variable_expander.Expand(expectation_name)
    file_exists = os.path.exists(file_path)
    assert expectation['exists'] == file_exists, \
        ('File %s exists' % file_path) if file_exists else \
        ('File %s is missing' % file_path)
