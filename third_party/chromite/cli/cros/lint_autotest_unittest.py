# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the lint_autotest module."""

from __future__ import print_function

import os
import mock

from pylint.checkers import base
from pylint.checkers import imports
from pylint.checkers import variables
from pylint import testutils

from chromite.cli.cros import lint_autotest
from chromite.cli.cros import lint_unittest


class ParamCheckerTest(lint_unittest.CheckerTestCase):
  """Tests for ParamChecker."""

  CHECKER = lint_autotest.ParamChecker

  GOOD_ARGUMENT_DOCSTRINGS = (
      (['arg'],
       """Does something.
       @param arg: An argument
       """),

      ([],
       """Does something."""),

      (['x', 'y'],
       """Does something.
       @param x: An argument
       @param y: An argument
       """),

      (['cls', 'arg'],
       """Does something.
       @param arg: An argument
       """),

      (['self', 'cls', 'args', 'kwargs'],
       """These args don't need documentation."""),
  )

  BAD_ARGUMENT_DOCSTRINGS = (
      (['x'],
       """Does something."""),

      (['self', 'x'],
       """Does something."""),

      (['x', 'y'],
       """Foo bar baz.
       @param y: blah
       """),
  )

  def testGoodArgumentDocstrings(self):
    """Allow docstrings whose @param and argument lists match."""
    for args, doc in self.GOOD_ARGUMENT_DOCSTRINGS:
      self.results = []
      node = lint_unittest.TestNode(doc=doc, display_type=None, col_offset=4,
                                    args=args)
      self.checker.visit_function(node)
      self.assertEqual(self.results, [],
                       msg='docstring was not accepted:\n"""%s""". ' % doc)

  def testBadArgumentDocstrings(self):
    """Don't allow docstrings whose @param and argument lists don't match."""
    for args, doc in self.BAD_ARGUMENT_DOCSTRINGS:
      self.results = []
      node = lint_unittest.TestNode(doc=doc, display_type=None, col_offset=4,
                                    args=args)
      self.checker.visit_function(node)
      self.assertNotEqual(self.results, [],
                          msg='docstring was not rejected:\n"""%s"""' % doc)


# Generate test classes for the functional tests.
INPUT_DIR = os.path.join(os.path.dirname(__file__),
                         'tests', 'lint_input')
MESSAGE_DIR = os.path.join(os.path.dirname(__file__),
                           'tests', 'lint_messages')
def MakeFunctionalTests():
  class PluginFunctionalTest(testutils.LintTestUsingFile):
    """Functional test which registers the lint_autotest plugin."""

    def setUp(self):
      """Save the state of the pylint plugins, then monkey patch them."""
      # While developer desktop environments will probably fail the "import
      # common" import, the test machines won't. By mocking out the
      # _IGNORE_MODULES list to use a different module name, the test will pass
      # regardless of whether the environment has a "common" module to import.
      self.context = mock.patch.object(
          lint_autotest, '_IGNORE_MODULES',
          ('lint_autotest_integration_test_example_module',))

      self.context.__enter__()

      classes = (
          base.DocStringChecker,
          imports.ImportsChecker,
          variables.VariablesChecker
      )
      # note: copy.copy doesn't work on DictProxy objects. Converting the
      # DictProxy to a dictionary works.
      self.old_state = {cls: dict(cls.__dict__) for cls in classes}
      lint_autotest.register(testutils.linter)

    def tearDown(self):
      """Reset the damage done by lint_autotest.register(...)"""
      self.context.__exit__(None, None, None)
      for cls, old_dict in self.old_state.iteritems():
        # .__dict__ is not writeable. Copy over k,v pairs.
        for k, v in old_dict.iteritems():
          # Some attributes are not writeable. Only fix the ones that changed.
          if cls.__dict__[k] is not v:
            setattr(cls, k, v)

      super(PluginFunctionalTest, self).tearDown()

  callbacks = [testutils.cb_test_gen(PluginFunctionalTest)]
  tests = testutils.make_tests(INPUT_DIR, MESSAGE_DIR, None, callbacks)
  for i, test in enumerate(tests):
    globals()['FunctionalTest%d' % i] = test

MakeFunctionalTests()
