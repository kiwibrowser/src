# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the lint module."""

from __future__ import print_function

import collections
import StringIO

from chromite.cli.cros import lint
from chromite.lib import cros_test_lib


# pylint: disable=protected-access


class TestNode(object):
  """Object good enough to stand in for lint funcs"""

  Args = collections.namedtuple('Args', ('args', 'vararg', 'kwarg'))
  Arg = collections.namedtuple('Arg', ('name',))

  def __init__(self, doc='', fromlineno=0, path='foo.py', args=(), vararg='',
               kwarg='', names=None, lineno=0, name='module',
               display_type='Module', col_offset=None):
    if names is None:
      names = [('name', None)]
    self.doc = doc
    self.lines = doc.split('\n')
    self.fromlineno = fromlineno
    self.lineno = lineno
    self.file = path
    self.args = self.Args(args=[self.Arg(name=x) for x in args],
                          vararg=vararg, kwarg=kwarg)
    self.names = names
    self.name = name
    self._display_type = display_type
    self.col_offset = col_offset

  def argnames(self):
    return [arg.name for arg in self.args.args]

  def display_type(self):
    return self._display_type


class StatStub(object):
  """Dummy object to stand in for stat checks."""

  def __init__(self, size=0, mode=0o644):
    self.st_size = size
    self.st_mode = mode


class CheckerTestCase(cros_test_lib.TestCase):
  """Helpers for Checker modules"""

  def add_message(self, msg_id, node=None, line=None, args=None):
    """Capture lint checks"""
    # We include node.doc here explicitly so the pretty assert message
    # inclues it in the output automatically.
    doc = node.doc if node else ''
    self.results.append((msg_id, doc, line, args))

  def setUp(self):
    assert hasattr(self, 'CHECKER'), 'TestCase must set CHECKER'

    self.results = []
    self.checker = self.CHECKER()
    self.checker.add_message = self.add_message


class DocStringCheckerTest(CheckerTestCase):
  """Tests for DocStringChecker module"""

  GOOD_FUNC_DOCSTRINGS = (
      'Some string',
      """Short summary

      Body of text.
      """,
      """line o text

      Body and comments on
      more than one line.

      Args:
        moo: cow

      Returns:
        some value

      Raises:
        something else
      """,
      """Short summary.

      Args:
        fat: cat

      Yields:
        a spoon
      """,
      """Don't flag args variables as sections.

      Args:
        return: Foo!
      """,
  )

  BAD_FUNC_DOCSTRINGS = (
      """
      bad first line
      """,
      """The first line is good
      but the second one isn't
      """,
      """ whitespace is wrong""",
      """whitespace is wrong	""",
      """ whitespace is wrong

      Multiline tickles differently.
      """,
      """Should be no trailing blank lines

      Returns:
        a value

      """,
      """ok line

      cuddled end""",
      """we want Args, not Arguments

      Arguments:
        some: arg
      """,
      """section order is wrong here

      Raises:
        It raised.

      Returns:
        It returned
      """,
      """sections are duplicated

      Returns:
        True

      Returns:
        or was it false
      """,
      """sections lack whitespace between them

      Args:
        foo: bar
      Returns:
        yeah
      """,
      """yields is misspelled

      Yield:
        a car
      """,
      """Section name has bad spacing

      Args:\x20\x20\x20
        key: here
      """,
      """too many blank lines


      Returns:
        None
      """,
      """wrongly uses javadoc

      @returns None
      """,
      """the indentation is incorrect

        Args:
          some: day
      """,
      """the final indentation is incorrect

      Blah.
       """,
  )

  # The current linter isn't good enough yet to detect these.
  TODO_BAD_FUNC_DOCSTRINGS = (
      """The returns section isn't a proper section

      Args:
        bloop: de

      returns something
      """,
  )

  CHECKER = lint.DocStringChecker

  def testGood_visit_function(self):
    """Allow known good docstrings"""
    for dc in self.GOOD_FUNC_DOCSTRINGS:
      self.results = []
      node = TestNode(doc=dc, display_type=None, col_offset=4)
      self.checker.visit_function(node)
      self.assertEqual(self.results, [],
                       msg='docstring was not accepted:\n"""%s"""' % dc)

  def testBad_visit_function(self):
    """Reject known bad docstrings"""
    for dc in self.BAD_FUNC_DOCSTRINGS:
      self.results = []
      node = TestNode(doc=dc, display_type=None, col_offset=4)
      self.checker.visit_function(node)
      self.assertNotEqual(self.results, [],
                          msg='docstring was not rejected:\n"""%s"""' % dc)

  def testSmoke_visit_module(self):
    """Smoke test for modules"""
    self.checker.visit_module(TestNode(doc='foo'))
    self.assertEqual(self.results, [])
    self.checker.visit_module(TestNode(doc='', path='/foo/__init__.py'))
    self.assertEqual(self.results, [])

  def testSmoke_visit_class(self):
    """Smoke test for classes"""
    self.checker.visit_class(TestNode(doc='bar'))

  def testGood_check_first_line(self):
    """Verify _check_first_line accepts good inputs"""
    docstrings = (
        'Some string',
    )
    for dc in docstrings:
      self.results = []
      node = TestNode(doc=dc)
      self.checker._check_first_line(node, node.lines)
      self.assertEqual(self.results, [],
                       msg='docstring was not accepted:\n"""%s"""' % dc)

  def testBad_check_first_line(self):
    """Verify _check_first_line rejects bad inputs"""
    docstrings = (
        '\nSome string\n',
    )
    for dc in docstrings:
      self.results = []
      node = TestNode(doc=dc)
      self.checker._check_first_line(node, node.lines)
      self.assertEqual(len(self.results), 1)

  def testGood_check_second_line_blank(self):
    """Verify _check_second_line_blank accepts good inputs"""
    docstrings = (
        'Some string\n\nThis is the third line',
        'Some string',
    )
    for dc in docstrings:
      self.results = []
      node = TestNode(doc=dc)
      self.checker._check_second_line_blank(node, node.lines)
      self.assertEqual(self.results, [],
                       msg='docstring was not accepted:\n"""%s"""' % dc)

  def testBad_check_second_line_blank(self):
    """Verify _check_second_line_blank rejects bad inputs"""
    docstrings = (
        'Some string\nnonempty secondline',
    )
    for dc in docstrings:
      self.results = []
      node = TestNode(doc=dc)
      self.checker._check_second_line_blank(node, node.lines)
      self.assertEqual(len(self.results), 1)

  def testGoodFuncVarKwArg(self):
    """Check valid inputs for *args and **kwargs"""
    for vararg in (None, 'args', '_args'):
      for kwarg in (None, 'kwargs', '_kwargs'):
        self.results = []
        node = TestNode(vararg=vararg, kwarg=kwarg)
        self.checker._check_func_signature(node)
        self.assertEqual(len(self.results), 0)

  def testMisnamedFuncVarKwArg(self):
    """Reject anything but *args and **kwargs"""
    for vararg in ('arg', 'params', 'kwargs', '_moo'):
      self.results = []
      node = TestNode(vararg=vararg)
      self.checker._check_func_signature(node)
      self.assertEqual(len(self.results), 1)

    for kwarg in ('kwds', '_kwds', 'args', '_moo'):
      self.results = []
      node = TestNode(kwarg=kwarg)
      self.checker._check_func_signature(node)
      self.assertEqual(len(self.results), 1)

  def testGoodFuncArgs(self):
    """Verify normal args in Args are allowed"""
    datasets = (
        ("""args are correct, and cls is ignored

         Args:
           moo: cow
         """,
         ('cls', 'moo',), None, None,
        ),
        ("""args are correct, and self is ignored

         Args:
           moo: cow
           *args: here
         """,
         ('self', 'moo',), 'args', 'kwargs',
        ),
        ("""args are allowed to wrap

         Args:
           moo:
             a big fat cow
             that takes many lines
             to describe its fatness
         """,
         ('moo',), None, 'kwargs',
        ),
    )
    for dc, args, vararg, kwarg in datasets:
      self.results = []
      node = TestNode(doc=dc, args=args, vararg=vararg, kwarg=kwarg)
      self.checker._check_all_args_in_doc(node, node.lines)
      self.assertEqual(len(self.results), 0)

  def testBadFuncArgs(self):
    """Verify bad/missing args in Args are caught"""
    datasets = (
        ("""missing 'bar'

         Args:
           moo: cow
         """,
         ('moo', 'bar',),
        ),
        ("""missing 'cow' but has 'bloop'

         Args:
           moo: cow
         """,
         ('bloop',),
        ),
        ("""too much space after colon

         Args:
           moo:  cow
         """,
         ('moo',),
        ),
        ("""not enough space after colon

         Args:
           moo:cow
         """,
         ('moo',),
        ),
    )
    for dc, args in datasets:
      self.results = []
      node = TestNode(doc=dc, args=args)
      self.checker._check_all_args_in_doc(node, node.lines)
      self.assertEqual(len(self.results), 1)


class ChromiteLoggingCheckerTest(CheckerTestCase):
  """Tests for ChromiteLoggingChecker module"""

  CHECKER = lint.ChromiteLoggingChecker

  def testLoggingImported(self):
    """Test that import logging is flagged."""
    node = TestNode(names=[('logging', None)], lineno=15)
    self.checker.visit_import(node)
    self.assertEqual(self.results, [('R9301', '', 15, None)])

  def testLoggingNotImported(self):
    """Test that importing something else (not logging) is not flagged."""
    node = TestNode(names=[('myModule', None)], lineno=15)
    self.checker.visit_import(node)
    self.assertEqual(self.results, [])


class SourceCheckerTest(CheckerTestCase):
  """Tests for SourceChecker module"""

  CHECKER = lint.SourceChecker

  def _testShebang(self, shebangs, exp, mode):
    """Helper for shebang tests"""
    for shebang in shebangs:
      self.results = []
      node = TestNode()
      stream = StringIO.StringIO(shebang)
      st = StatStub(size=len(shebang), mode=mode)
      self.checker._check_shebang(node, stream, st)
      self.assertEqual(len(self.results), exp,
                       msg='processing shebang failed: %r' % shebang)

  def testBadShebang(self):
    """Verify _check_shebang rejects bad shebangs"""
    shebangs = (
        '#!/usr/bin/python\n',
        '#! /usr/bin/python2 \n',
        '#!/usr/bin/env python\n',
        '#! /usr/bin/env python2 \n',
        '#!/usr/bin/python2\n',
    )
    self._testShebang(shebangs, 1, 0o755)

  def testGoodShebangNoExec(self):
    """Verify _check_shebang rejects shebangs on non-exec files"""
    shebangs = (
        '#!/usr/bin/env python2\n',
        '#!/usr/bin/env python3\n',
    )
    self._testShebang(shebangs, 1, 0o644)

  def testGoodShebang(self):
    """Verify _check_shebang accepts good shebangs"""
    shebangs = (
        '#!/usr/bin/env python2\n',
        '#!/usr/bin/env python3\n',
        '#!/usr/bin/env python2\t\n',
    )
    self._testShebang(shebangs, 0, 0o755)

  def testGoodUnittestName(self):
    """Verify _check_module_name accepts good unittest names"""
    module_names = (
        'lint_unittest',
    )
    for name in module_names:
      node = TestNode(name=name)
      self.results = []
      self.checker._check_module_name(node)
      self.assertEqual(len(self.results), 0)

  def testBadUnittestName(self):
    """Verify _check_module_name accepts good unittest names"""
    module_names = (
        'lint_unittests',
    )
    for name in module_names:
      node = TestNode(name=name)
      self.results = []
      self.checker._check_module_name(node)
      self.assertEqual(len(self.results), 1)
