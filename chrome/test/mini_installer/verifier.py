# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Verifier:
  """Verifies that the current machine states match the expectation."""

  def VerifyInput(self, verifier_input, variable_expander):
    """Verifies that the current machine states match |verifier_input|.

    Args:
      verifier_input: An input to the verifier. It is a dictionary where each
          key is an expectation name and the associated value is an expectation
          dictionary. The expectation dictionary may contain an optional
          'condition' property, a string that determines whether the expectation
          should be verified. Each subclass can specify a different expectation
          name and expectation dictionary.
      variable_expander: A VariableExpander object.
    """
    for expectation_name, expectation in verifier_input.iteritems():
      if 'condition' in expectation:
        condition = variable_expander.Expand(expectation['condition'])
        if not self._EvaluateCondition(condition):
          continue
      self._VerifyExpectation(expectation_name, expectation, variable_expander)

  def _VerifyExpectation(self, expectation_name, expectation,
                         variable_expander):
    """Verifies that the current machine states match |verifier_input|.

    This is an abstract method for subclasses to override.

    Args:
      expectation_name: An expectation name. Each subclass can specify a
          different expectation name format.
      expectation: An expectation dictionary. Each subclass can specify a
          different expectation dictionary format.
      variable_expander: A VariableExpander object.
    """
    raise NotImplementedError()

  def _EvaluateCondition(self, condition):
    """Evaluates |condition| using eval().

    Args:
      condition: A condition string.

    Returns:
      The result of the evaluated condition.
    """
    return eval(condition, {'__builtins__': {'False': False, 'True': True}})
