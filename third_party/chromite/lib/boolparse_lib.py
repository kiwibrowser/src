# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Parser used to parse the boolean expression."""

from __future__ import print_function

import ast
import pyparsing
import re


class BoolParseError(Exception):
  """Base exception for this module."""


class _BoolOperand(object):
  """Read pyparsing.Keyword as operand and evaluate its boolean value."""

  def __init__(self, t):
    """Initialize the object.

    Read boolean operands from pyparsing.Keyword and evaluate into the
    corresponding boolean values.

    Args:
      t: t[0] is pyparsing.Keyword corresponding to False or True.
    """
    self.label = t[0]
    self.value = ast.literal_eval(t[0])

  def __bool__(self):
    return self.value

  def __str__(self):
    return self.label

  __nonzero__ = __bool__


class _BoolBinOp(object):
  """General class for binary operation."""

  def __init__(self, t):
    """Initialize object.

    Extract the operand from the input. The operand is the pyparsing.Keyword.

    Args:
      t: A list containing a list of operand and operator, such as
      [[True, 'and', False]]. t[0] is [True, 'and', False]. t[0][0::2] are the
      two operands.
    """
    self.args = t[0][0::2]

  def __bool__(self):
    """Evaluate the boolean value of the binary boolean expression.

    evalop is the method used to evaluate, which is overwritten in the children
    class of _BoolBinOp.

    Returns:
      boolean result.
    """
    return self.evalop(bool(a) for a in self.args)

  __nonzero__ = __bool__


class _BoolAnd(_BoolBinOp):
  """And boolean binary operation."""

  evalop = all


class _BoolOr(_BoolBinOp):
  """Or boolean binary operation."""

  evalop = any


class _BoolNot(object):
  """Not operation."""

  def __init__(self, t):
    self.arg = t[0][1]

  def __bool__(self):
    v = bool(self.arg)
    return not v

  __nonzero__ = __bool__


def _ExprOverwrite(expr, true_variables):
  """Overwrite variables in |expr| based on |true_variables|.

  Overwrite variables in |expr| with 'True' if they occur in |true_variables|,
  'False' otherwise.

  Args:
    expr: The orginal boolean expression, like 'A and B'.
    true_variables: Collection of variable names to be considered True, e.g.
    {'A'}.

  Returns:
    A boolean expression string with pyparsing.Keyword, like 'True and False'.
  """
  # When true_variables is None, replace it with empty collection ()
  target_set = set(true_variables or ())
  items = {
      x.strip() for x in re.split(r'(?i) and | or |not |\(|\)', expr)
      if x.strip()}
  boolstr = expr
  for item in items:
    boolstr = boolstr.replace(
        item, 'True' if item in target_set else 'False')

  return boolstr

def BoolstrResult(expr, true_variables):
  """Determine if a boolean expression is satisfied.

  BoolstrResult('A and B and not C', {'A', 'C'}) -> False

  Args:
    expr: The orginal boolean expression, like 'A and B'.
    true_variables: Collection to be checked whether satisfy the boolean expr.

  Returns:
    True if the given |true_variables| cause the boolean expression |expr| to
    be satisfied, False otherwise.
  """
  boolstr = _ExprOverwrite(expr, true_variables)

  # Define the boolean logic
  TRUE = pyparsing.Keyword('True')
  FALSE = pyparsing.Keyword('False')
  boolOperand = TRUE | FALSE
  boolOperand.setParseAction(_BoolOperand)

  # Define expression, based on expression operand and list of operations in
  # precedence order.
  boolExpr = pyparsing.infixNotation(
      boolOperand, [('not', 1, pyparsing.opAssoc.RIGHT, _BoolNot),
                    ('and', 2, pyparsing.opAssoc.LEFT, _BoolAnd),
                    ('or', 2, pyparsing.opAssoc.LEFT, _BoolOr),])

  try:
    res = boolExpr.parseString(boolstr)[0]
    return bool(res)
  except (AttributeError, pyparsing.ParseException):
    raise BoolParseError('Cannot parse the boolean expression string "%s".'
                         % expr)
