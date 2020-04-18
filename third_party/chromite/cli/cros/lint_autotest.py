# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This module is not automatically loaded by the `cros` helper.  The filename
# would need a "cros_" prefix to make that happen.  It lives here so that it
# is alongside the cros_lint.py file.
#
# For msg namespaces, the 9xxx should generally be reserved for our own use.

"""A lint module loaded by pylint for Autotest linting.

This module patches pylint library functions to suit autotest.

This is loaded by pylint directly via the autotest pylintrc file:
  load-plugins=chromite.cli.cros.lint_autotest
"""

from __future__ import print_function

import re

from pylint.checkers import base
from pylint.checkers import BaseChecker
from pylint.checkers import imports
from pylint.checkers import variables
from pylint.interfaces import IAstroidChecker
import logilab.common.modutils


# patch up the logilab module lookup tools to understand autotest_lib.* trash
_ffm = logilab.common.modutils.file_from_modpath

def file_from_modpath(modpath, paths=None, context_file=None):
  """Wrapper to eliminate autotest_lib from modpath.

  Args:
    modpath: name of module splitted on '.'
    paths: optional list of paths where module should be searched for.
    context_file: path to file doing the importing.

  Returns:
    The path to the module as returned by the parent method invocation.

  Raises:
    ImportError if these is no such module.
  """
  if modpath[0] == "autotest_lib":
    return _ffm(modpath[1:], paths, context_file)
  else:
    return _ffm(modpath, paths, context_file)


# patch up pylint import checker to handle our importing magic
ROOT_MODULE = 'autotest_lib.'

# A list of modules for pylint to ignore, specifically, these modules
# are imported for their side-effects and are not meant to be used.
_IGNORE_MODULES = (
    'common',
    'frontend_test_utils',
    'setup_django_environment',
    'setup_django_lite_environment',
    'setup_django_readonly_environment',
    'setup_test_environment',
)


def patch_modname(modname):
  """Patches modname so we can make sense of autotest_lib modules.

  Args:
    modname: name of a module, contains '.'

  Returns:
    The modname string without the 'autotest_lib.' prefix. For example,
    patch_modname('autotest_lib.foo.bar') == 'foo.bar'
    patch_modname('foo.bar') == 'foo.bar'
  """
  if modname.startswith(ROOT_MODULE) or modname.startswith(ROOT_MODULE[:-1]):
    modname = modname[len(ROOT_MODULE):]
  return modname


def patch_consumed_list(to_consume=None, consumed=None):
  """Patches the consumed modules list to ignore modules with side effects.

  Autotest relies on importing certain modules solely for their side
  effects. Pylint doesn't understand this and flags them as unused, since
  they're not referenced anywhere in the code. To overcome this we need
  to transplant said modules into the dictionary of modules pylint has
  already seen, before pylint checks it.

  Args:
    to_consume: a dictionary of names pylint needs to see referenced.
    consumed: a dictionary of names that pylint has seen referenced.
  """
  if to_consume is None or consumed is None:
    return

  for module in _IGNORE_MODULES:
    if module in to_consume:
      consumed[module] = to_consume[module]
      del to_consume[module]


# This decorator will be used for monkey patching the built-in pylint classes.
def patch_cls(cls):
  """Sets a method of `cls`."""
  def patcher(method):
    setattr(cls, method.__name__, method)
  return patcher


def CustomizeImportsChecker():
  """Modifies stock imports checker to suit autotest."""
  cls = imports.ImportsChecker

  old_visit_from = cls.visit_from
  @patch_cls(cls)
  def visit_from(self, node):  # pylint: disable=unused-variable
    node.modname = patch_modname(node.modname)
    return old_visit_from(self, node)


def CustomizeVariablesChecker():
  """Modifies stock variables checker to suit autotest."""
  cls = variables.VariablesChecker

  old_visit_module = cls.visit_module
  @patch_cls(cls)
  def visit_module(self, node):  # pylint: disable=unused-variable
    """Unflag 'import common'.

    _to_consume eg: [({to reference}, {referenced}, 'scope type')]
    Enteries are appended to this list as we drill deeper in scope.
    If we ever come across a module to ignore,  we immediately move it
    to the consumed list.

    Args:
      node: node of the ast we're currently checking.
    """
    old_visit_module(self, node)
    # pylint: disable=protected-access
    scoped_names = self._to_consume.pop()
    patch_consumed_list(scoped_names[0], scoped_names[1])
    self._to_consume.append(scoped_names)

  old_visit_from = cls.visit_from
  @patch_cls(cls)
  def visit_from(self, node):  # pylint: disable=unused-variable
    """Patches modnames so pylints understands autotest_lib."""
    node.modname = patch_modname(node.modname)
    return old_visit_from(self, node)


def _ShouldSkipArg(arg):
  """Checks if arg name can be excluded from @param list.

  Returns:
    True if the argument given by arg is whitelisted, and does
    not require a "@param" docstring.
  """
  return arg in ('self', 'cls', 'args', 'kwargs', 'dargs')


def ShouldSkipDocstring(node):
  """Returns whether docstring checks should run on this function node.

  Args:
    node: The node under examination.
  """
  # Even plain functions will have a parent, which is the
  # module they're in, and a frame, which is the context
  # of said module; They need not however, always have
  # ancestors.
  return (node.name in ('run_once', 'initialize', 'cleanup') and
          hasattr(node.parent.frame(), 'ancestors') and
          any(ancestor.name == 'base_test' for ancestor in
              node.parent.frame().ancestors()))

def CustomizeDocStringChecker():
  """Modifies stock docstring checker to suit Autotest doxygen style."""

  cls = base.DocStringChecker

  @patch_cls(cls)
  def visit_module(_self, _node):  # pylint: disable=unused-variable
    """Don't visit imported modules when checking for docstrings.

    Args:
      node: the node we're visiting.
    """

  old_visit_function = cls.visit_function
  @patch_cls(cls)
  def visit_function(self, node):   # pylint: disable=unused-variable
    """Don't request docstrings for commonly overridden autotest functions.

    Args:
      node: node of the ast we're currently checking.
    """
    if ShouldSkipDocstring(node):
      return

    old_visit_function(self, node)


class ParamChecker(BaseChecker):
  """Checks that each argument has a @param entry in the docstring."""
  __implements__ = IAstroidChecker

  # The numbering for this message matches that of the doc string checker class
  # in chromite.cli.cros.lint
  class _MessageCP010(object):
    """Message for missing @param statements."""
    pass

  name = 'doc_string_param_checker'
  priority = -1
  MSG_ARGS = 'offset:%(offset)i: {%(line)s}'
  msgs = {
      'C9010': ('Docstring for %(func)s needs "@param %(arg)s:"',
                ('docstring-missing-args'), _MessageCP010),
  }

  def visit_function(self, node):
    """Verify the function's docstrings."""
    if node.doc and not ShouldSkipDocstring(node):
      self._check_all_args_in_doc(node)

  ARG_DOCSTRING_RGX = re.compile(r'@param ([^:]+)')

  def _check_all_args_in_doc(self, node):
    """Teaches pylint to look for @param with each argument.

    Args:
      node_type: type of the node we're currently checking.
      node: node of the ast we're currently checking.
    """
    present_args = set(arg for arg in node.argnames()
                       if not _ShouldSkipArg(arg))
    documented_args = set(re.findall(self.ARG_DOCSTRING_RGX, node.doc))

    for undocumented in present_args - documented_args:
      self.add_message('C9010', node=node,
                       line=node.fromlineno,
                       args={'arg': undocumented,
                             'func': node.name},)


def register(linter):
  """Pylint will call this func when we use the 'load-plugins' invocation.

  Args:
    linter: The pylint linter instance for this run.
  """
  # Walk all the classes in this module and register ours.
  linter.register_checker(ParamChecker(linter))
  CustomizeDocStringChecker()
  CustomizeImportsChecker()
  CustomizeVariablesChecker()
  logilab.common.modutils.file_from_modpath = file_from_modpath
