# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Contains functionality used to implement a partial mock."""

from __future__ import print_function

import collections
import mock
import os
import re

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils


class Comparator(object):
  """Base class for all comparators."""

  def Match(self, arg):
    """Match the comparator against an argument."""
    raise NotImplementedError('method must be implemented by a subclass.')

  def Equals(self, rhs):
    """Returns whether rhs compares the same thing."""
    return type(self) == type(rhs) and self.__dict__ == rhs.__dict__

  def __eq__(self, rhs):
    return self.Equals(rhs)

  def __ne__(self, rhs):
    return not self.Equals(rhs)


class In(Comparator):
  """Checks whether an item (or key) is in a list (or dict) parameter."""

  def __init__(self, key):
    """Initialize.

    Args:
      key: Any thing that could be in a list or a key in a dict
    """
    Comparator.__init__(self)
    self._key = key

  def Match(self, arg):
    try:
      return self._key in arg
    except TypeError:
      return False

  def __repr__(self):
    return '<sequence or map containing %r>' % str(self._key)


class InOrder(Comparator):
  """Checks whether every items of a list exists in a list/dict parameter."""

  def __init__(self, items):
    """Constructor.

    Args:
      items: A list of things that could be in a list or a key in a dict
    """
    super(InOrder, self).__init__()
    self.items = items

  def Match(self, args):
    """Checks if args' item matches all expected items in sequence.

    Args:
      args: parameter list.

    Returns:
      True if all expected items are matched.
    """
    items = list(self.items)
    to_match = items.pop(0)
    for arg in args:
      if to_match == arg:
        if len(items) < 1:
          return True
        to_match = items.pop(0)
    return False

  def __repr__(self):
    return '<sequence or map containing %r>' % str(self.items)


class Regex(Comparator):
  """Checks if a string matches a regular expression."""

  def __init__(self, pattern, flags=0):
    """Initialize.

    Args:
      pattern: is the regular expression to search for
      flags: passed to re.compile function as the second argument
    """
    Comparator.__init__(self)
    self.pattern = pattern
    self.flags = flags
    self.regex = re.compile(pattern, flags=flags)

  def Match(self, arg):
    try:
      return self.regex.search(arg) is not None
    except TypeError:
      return False

  def __repr__(self):
    s = '<regular expression %r' % self.regex.pattern
    if self.regex.flags:
      s += ', flags=%d' % self.regex.flags
    s += '>'
    return s


class ListRegex(Regex):
  """Checks if any string from an iterable matches a regular expression.

  This can be used to match on a list. For example, a regex of
  'dump_fmap -p .*bios.bin' will match ['dump_fmap', '-p', 'mainbios.bin'].
  """

  @staticmethod
  def _ProcessArg(arg):
    if not isinstance(arg, basestring):
      return ' '.join(arg)
    return arg

  def Match(self, arg):
    try:
      return self.regex.search(self._ProcessArg(arg)) is not None
    except TypeError:
      return False


class Ignore(Comparator):
  """Used when we don't care about an argument of a method call."""

  def Match(self, _arg):
    return True

  def __repr__(self):
    return '<IgnoreArg>'


class HasString(str):
  """A substring matcher for mock assertion.

  It overrides str's '==' operator so that
  HasString('substring') == 'A sentence with substring'

  It is used for mock.assert_called_with(). Note that it is not a Comparator.

  Usage:
    some_mock.assert_called_with(
        partial_mock.HasString('need_this_keyword'))
  """
  def __eq__(self, target):
    return self in target


def _RecursiveCompare(lhs, rhs):
  """Compare parameter specs recursively.

  Args:
    lhs: Left Hand Side parameter spec to compare.
    rhs: Right Hand Side parameter spec to compare.
    equality: In the case of comparing Comparator objects, True means we call
      the Equals() function.  We call Match() if set to False (default).
  """
  if isinstance(lhs, Comparator):
    return lhs.Match(rhs)
  elif isinstance(lhs, (tuple, list)):
    return (type(lhs) == type(rhs) and
            len(lhs) == len(rhs) and
            all(_RecursiveCompare(i, j) for i, j in zip(lhs, rhs)))
  elif isinstance(lhs, dict):
    return _RecursiveCompare(sorted(lhs.iteritems()), sorted(rhs.iteritems()))
  else:
    return lhs == rhs


def ListContains(small, big, strict=False):
  """Looks for a sublist within a bigger list.

  Args:
    small: The sublist to search for.
    big: The list to search in.
    strict: If True, all items in list must be adjacent.
  """
  if strict:
    for i in xrange(len(big) - len(small) + 1):
      if _RecursiveCompare(small, big[i:i + len(small)]):
        return True
    return False
  else:
    j = 0
    for i in xrange(len(small)):
      for j in xrange(j, len(big)):
        if _RecursiveCompare(small[i], big[j]):
          j += 1
          break
      else:
        return False
    return True


def DictContains(small, big):
  """Looks for a subset within a dictionary.

  Args:
    small: The sub-dict to search for.
    big: The dict to search in.
  """
  for k, v in small.iteritems():
    if k not in big or not _RecursiveCompare(v, big[k]):
      return False
  return True


class MockedCallResults(object):
  """Implements internal result specification for partial mocks.

  Used with the PartialMock class.

  Internal results are different from external results (return values,
  side effects, exceptions, etc.) for functions.  Internal results are
  *used* by the partial mock to generate external results.  Often internal
  results represent the external results of the dependencies of the function
  being partially mocked.  Of course, the partial mock can just pass through
  the internal results to become external results.
  """

  Params = collections.namedtuple('Params', ['args', 'kwargs'])
  MockedCall = collections.namedtuple(
      'MockedCall', ['params', 'strict', 'result', 'side_effect'])

  def __init__(self, name):
    """Initialize.

    Args:
      name: The name given to the mock.  Will be used in debug output.
    """
    self.name = name
    self.mocked_calls = []
    self.default_result, self.default_side_effect = None, None

  @staticmethod
  def AssertArgs(args, kwargs):
    """Verify arguments are of expected type."""
    assert isinstance(args, (tuple))
    if kwargs:
      assert isinstance(kwargs, dict)

  def AddResultForParams(self, args, result, kwargs=None, side_effect=None,
                         strict=True):
    """Record the internal results of a given partial mock call.

    Args:
      args: A list containing the positional args an invocation must have for
        it to match the internal result.  The list can contain instances of
        meta-args (such as IgnoreArg, Regex, In, etc.).  Positional argument
        matching is always *strict*, meaning extra positional arguments in
        the invocation are not allowed.
      result: The internal result that will be matched for the command
        invocation specified.
      kwargs: A dictionary containing the keyword args an invocation must have
        for it to match the internal result.  The dictionary can contain
        instances of meta-args (such as IgnoreArg, Regex, In, etc.).  Keyword
        argument matching is by default *strict*, but can be modified by the
        |strict| argument.
      side_effect: A functor that gets called every time a partially mocked
        function is invoked.  The arguments the partial mock is invoked with are
        passed to the functor.  This is similar to how side effects work for
        mocks.
      strict: Specifies whether keyword are matched strictly.  With strict
        matching turned on, any keyword args a partial mock is invoked with that
        are not specified in |kwargs| will cause the match to fail.
    """
    self.AssertArgs(args, kwargs)
    if kwargs is None:
      kwargs = {}

    params = self.Params(args=args, kwargs=kwargs)
    dup, filtered = cros_build_lib.PredicateSplit(
        lambda mc: mc.params == params, self.mocked_calls)

    new = self.MockedCall(params=params, strict=strict, result=result,
                          side_effect=side_effect)
    filtered.append(new)
    self.mocked_calls = filtered

    if dup:
      logging.debug('%s: replacing mock for arguments %r:\n%r -> %r',
                    self.name, params, dup, new)

  def SetDefaultResult(self, result, side_effect=None):
    """Set the default result for an unmatched partial mock call.

    Args:
      result: See AddResultsForParams.
      side_effect: See AddResultsForParams.
    """
    self.default_result, self.default_side_effect = result, side_effect

  def LookupResult(self, args, kwargs=None, hook_args=None, hook_kwargs=None):
    """For a given mocked function call lookup the recorded internal results.

    Args:
      args: A list containing positional args the function was called with.
      kwargs: A dict containing keyword args the function was called with.
      hook_args: A list of positional args to call the hook with.
      hook_kwargs: A dict of key/value args to call the hook with.

    Returns:
      The recorded result for the invocation.

    Raises:
      AssertionError when the call is not mocked, or when there is more
      than one mock that matches.
    """
    def filter_fn(mc):
      if mc.strict:
        return _RecursiveCompare(mc.params, params)

      return (DictContains(mc.params.kwargs, kwargs) and
              _RecursiveCompare(mc.params.args, args))

    def is_exception(obj):
      """Returns True if obj is an exception instance or class."""
      return (
          isinstance(obj, BaseException) or
          isinstance(obj, type) and issubclass(obj, BaseException))

    self.AssertArgs(args, kwargs)
    if kwargs is None:
      kwargs = {}

    params = self.Params(args, kwargs)
    matched, _ = cros_build_lib.PredicateSplit(filter_fn, self.mocked_calls)
    if len(matched) > 1:
      raise AssertionError(
          '%s: args %r matches more than one mock:\n%s'
          % (self.name, params, '\n'.join([repr(c) for c in matched])))
    elif matched:
      side_effect, result = matched[0].side_effect, matched[0].result
    elif (self.default_result, self.default_side_effect) != (None, None):
      side_effect, result = self.default_side_effect, self.default_result
    else:
      raise AssertionError('%s: %r not mocked!' % (self.name, params))

    if is_exception(side_effect):
      raise side_effect
    if side_effect:
      assert hook_args is not None
      assert hook_kwargs is not None
      hook_result = side_effect(*hook_args, **hook_kwargs)
      if hook_result is not None:
        return hook_result
    return result


class PartialMock(object):
  """Provides functionality for partially mocking out a function or method.

  Partial mocking is useful in cases where the side effects of a function or
  method are complex, and so re-using the logic of the function with
  *dependencies* mocked out is preferred over mocking out the entire function
  and re-implementing the side effect (return value, state modification) logic
  in the test.  It is also useful for creating re-usable mocks.
  """

  TARGET = None
  ATTRS = None

  def __init__(self, create_tempdir=False):
    """Initialize.

    Args:
      create_tempdir: If set to True, the partial mock will create its own
        temporary directory when start() is called, and will set self.tempdir to
        the path of the directory.  The directory is deleted when stop() is
        called.
    """
    self.backup = {}
    self.patchers = {}
    self.patched = {}
    self.external_patchers = []
    self.create_tempdir = create_tempdir

    # Set when start() is called.
    self._tempdir_obj = None
    self.tempdir = None
    self.__saved_env__ = None
    self.started = False

    self._results = {}

    if not all([self.TARGET, self.ATTRS]) and any([self.TARGET, self.ATTRS]):
      raise AssertionError('TARGET=%r but ATTRS=%r!'
                           % (self.TARGET, self.ATTRS))

    if self.ATTRS is not None:
      for attr in self.ATTRS:
        self._results[attr] = MockedCallResults(attr)

  def __enter__(self):
    return self.start()

  def __exit__(self, exc_type, exc_value, traceback):
    self.stop()

  def PreStart(self):
    """Called at the beginning of start(). Child classes can override this.

    If __init__ was called with |create_tempdir| set, then self.tempdir will
    point to an existing temporary directory when this function is called.
    """

  def PreStop(self):
    """Called at the beginning of stop().  Child classes can override this.

    If __init__ was called with |create_tempdir| set, then self.tempdir will
    not be deleted until after this function returns.
    """

  def StartPatcher(self, patcher):
    """PartialMock will stop the patcher when stop() is called."""
    self.external_patchers.append(patcher)
    return patcher.start()

  def PatchObject(self, *args, **kwargs):
    """Create and start a mock.patch.object().

    stop() will be called automatically during tearDown.
    """
    return self.StartPatcher(mock.patch.object(*args, **kwargs))

  def _start(self):
    if not all([self.TARGET, self.ATTRS]):
      return

    chunks = self.TARGET.rsplit('.', 1)
    module = cros_build_lib.load_module(chunks[0])

    cls = getattr(module, chunks[1])
    for attr in self.ATTRS:
      self.backup[attr] = getattr(cls, attr)
      src_attr = '_target%s' % attr if attr.startswith('__') else attr
      if hasattr(self.backup[attr], 'reset_mock'):
        raise AssertionError(
            'You are trying to nest mock contexts - this is currently '
            'unsupported by PartialMock.')
      if callable(self.backup[attr]):
        patcher = mock.patch.object(cls, attr, autospec=True,
                                    side_effect=getattr(self, src_attr))
      else:
        patcher = mock.patch.object(cls, attr, getattr(self, src_attr))
      self.patched[attr] = patcher.start()
      self.patchers[attr] = patcher

    return self

  def start(self):
    """Activates the mock context."""
    try:
      self.__saved_env__ = os.environ.copy()
      self.tempdir = None
      if self.create_tempdir:
        self._tempdir_obj = osutils.TempDir(set_global=True)
        self.tempdir = self._tempdir_obj.tempdir

      self.started = True
      self.PreStart()
      return self._start()
    except:
      self.stop()
      raise

  def stop(self):
    """Restores namespace to the unmocked state."""
    try:
      if self.__saved_env__ is not None:
        osutils.SetEnvironment(self.__saved_env__)

      tasks = ([self.PreStop] + [p.stop for p in self.patchers.itervalues()] +
               [p.stop for p in self.external_patchers])
      if self._tempdir_obj is not None:
        tasks += [self._tempdir_obj.Cleanup]
      cros_build_lib.SafeRun(tasks)
    finally:
      self.started = False
      self.tempdir, self._tempdir_obj = None, None

  def UnMockAttr(self, attr):
    """Unsetting the mock of an attribute/function."""
    self.patchers.pop(attr).stop()


def CheckAttr(f):
  """Automatically set mock_attr based on class default.

  This function decorator automatically sets the mock_attr keyword argument
  based on the class default. The mock_attr specifies which mocked attribute
  a given function is referring to.

  Raises an AssertionError if mock_attr is left unspecified.
  """

  def new_f(self, *args, **kwargs):
    mock_attr = kwargs.pop('mock_attr', None)
    if mock_attr is None:
      mock_attr = self.DEFAULT_ATTR
      if self.DEFAULT_ATTR is None:
        raise AssertionError(
            'mock_attr not specified, and no default configured.')
    return f(self, *args, mock_attr=mock_attr, **kwargs)
  return new_f


class PartialCmdMock(PartialMock):
  """Base class for mocking functions that wrap command line functionality.

  Implements mocking for functions that shell out.  The internal results are
  'returncode', 'output', 'error'.
  """

  CmdResult = collections.namedtuple(
      'MockResult', ['returncode', 'output', 'error'])

  DEFAULT_ATTR = None

  @CheckAttr
  def SetDefaultCmdResult(self, returncode=0, output='', error='',
                          side_effect=None, mock_attr=None):
    """Specify the default command result if no command is matched.

    Args:
      returncode: See AddCmdResult.
      output: See AddCmdResult.
      error: See AddCmdResult.
      side_effect: See MockedCallResults.AddResultForParams
      mock_attr: Which attributes's mock is being referenced.
    """
    result = self.CmdResult(returncode, output, error)
    self._results[mock_attr].SetDefaultResult(result, side_effect)

  @CheckAttr
  def AddCmdResult(self, cmd, returncode=0, output='', error='',
                   kwargs=None, strict=False, side_effect=None, mock_attr=None):
    """Specify the result to simulate for a given command.

    Args:
      cmd: The command string or list to record a result for.
      returncode: The returncode of the command (on the command line).
      output: The stdout output of the command.
      error: The stderr output of the command.
      kwargs: Keyword arguments that the function needs to be invoked with.
      strict: Defaults to False.  See MockedCallResults.AddResultForParams.
      side_effect: See MockedCallResults.AddResultForParams
      mock_attr: Which attributes's mock is being referenced.
    """
    result = self.CmdResult(returncode, output, error)
    self._results[mock_attr].AddResultForParams(
        (cmd,), result, kwargs=kwargs, side_effect=side_effect, strict=strict)

  @CheckAttr
  def CommandContains(self, args, cmd_arg_index=-1, mock_attr=None, **kwargs):
    """Verify that at least one command contains the specified args.

    Args:
      args: Set of expected command-line arguments.
      cmd_arg_index: The index of the command list in the positional call_args.
        Defaults to the last positional argument.
      kwargs: Set of expected keyword arguments.
      mock_attr: Which attributes's mock is being referenced.
    """
    for call_args, call_kwargs in self.patched[mock_attr].call_args_list:
      if (ListContains(args, call_args[cmd_arg_index]) and
          DictContains(kwargs, call_kwargs)):
        return True
    return False

  @CheckAttr
  def assertCommandContains(self, args=(), expected=True, mock_attr=None,
                            **kwargs):
    """Assert that RunCommand was called with the specified args.

    This verifies that at least one of the RunCommand calls contains the
    specified arguments on the command line.

    Args:
      args: Set of expected command-line arguments.
      expected: If False, instead verify that none of the RunCommand calls
          contained the specified arguments.
      **kwargs: Set of expected keyword arguments.
      mock_attr: Which attributes's mock is being referenced.
    """
    if bool(expected) != self.CommandContains(args, **kwargs):
      if expected:
        msg = 'Expected to find %r in any of:\n%s'
      else:
        msg = 'Expected to not find %r in any of:\n%s'
      patched = self.patched[mock_attr]
      cmds = '\n'.join(repr(x) for x in patched.call_args_list)
      raise AssertionError(msg % (mock.call(args, **kwargs), cmds))

  @CheckAttr
  def assertCommandCalled(self, args=(), mock_attr=None, **kwargs):
    """Assert that RunCommand was called with the specified args.

    This verifies that at least one of the RunCommand calls exactly
    matches the specified command line and misc-arguments.

    Args:
      args: Set of expected command-line arguments.
      mock_attr: Which attributes's mock is being referenced.
      **kwargs: Set of expected keyword arguments.
    """
    call = mock.call(args, **kwargs)
    patched = self.patched[mock_attr]

    for icall in patched.call_args_list:
      if call == icall:
        return

    cmds = '\n'.join(repr(x) for x in patched.call_args_list)
    raise AssertionError('Expected to find %r in any of:\n%s' % (call, cmds))

  @property
  @CheckAttr
  def call_count(self, mock_attr=None):
    """Return the number of times we've been called."""
    return self.patched[mock_attr].call_count

  @property
  @CheckAttr
  def call_args_list(self, mock_attr=None):
    """Return the list of args we've been called with."""
    return self.patched[mock_attr].call_args_list
