#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Defer tool for SCons."""


import os
import sys
import types
import SCons.Errors


# Current group name being executed by ExecuteDefer().  Set to None outside
# of ExecuteDefer().
_execute_defer_context = None


class DeferGroup:
  """Named list of functions to be deferred."""
  # If we derive DeferGroup from object, instances of it return type
  # <class 'defer.DeferGroup'>, which prevents SCons.Util.semi_deepcopy()
  # from calling its __semi_deepcopy__ function.
  # TODO: Make semi_deepcopy() capable of handling classes derived from
  # object.

  def __init__(self):
    """Initialize deferred function object."""
    self.func_env_cwd = []
    self.after = set()

  def __semi_deepcopy__(self):
    """Makes a semi-deep-copy of this object.

    Returns:
      A semi-deep-copy of this object.

    This means it copies the sets and lists contained by this object, but
    doesn't make copies of the function pointers and environments pointed to by
    those lists.

    Needed so env.Clone() makes a copy of the defer list, so that functions
    and after-relationships subsequently added to the clone are not added to
    the parent.
    """
    c = DeferGroup()
    c.func_env_cwd = self.func_env_cwd[:]
    c.after = self.after.copy()
    return c


def SetDeferRoot(self):
  """Sets the current environment as the root environment for defer.

  Args:
    self: Current environment context.

  Functions deferred by environments cloned from the root environment (that is,
  function deferred by children of the root environment) will be executed when
  ExecuteDefer() is called from the root environment.

  Functions deferred by environments from which the root environment was cloned
  (that is, functions deferred by parents of the root environment) will be
  passed the root environment instead of the original parent environment.
  (Otherwise, they would have no way to determine the root environment.)
  """
  # Set the current environment as the root for holding defer groups
  self['_DEFER_ROOT_ENV'] = self

  # Deferred functions this environment got from its parents will be run in the
  # new root context.
  for group in GetDeferGroups(self).values():
    new_list = [(func, self, cwd) for (func, env, cwd) in group.func_env_cwd]
    group.func_env_cwd = new_list


def GetDeferRoot(self):
  """Returns the root environment for defer.

  Args:
    self: Current environment context.

  Returns:
    The root environment for defer.  If one of this environment's parents
    called SetDeferRoot(), returns that environment.  Otherwise returns the
    current environment.
  """
  return self.get('_DEFER_ROOT_ENV', self)


def GetDeferGroups(env):
  """Returns the dict of defer groups from the root defer environment.

  Args:
    env: Environment context.

  Returns:
    The dict of defer groups from the root defer environment.
  """
  return env.GetDeferRoot()['_DEFER_GROUPS']


def ExecuteDefer(self):
  """Executes deferred functions.

  Args:
    self: Current environment context.
  """
  # Check for re-entrancy
  global _execute_defer_context
  if _execute_defer_context:
    raise SCons.Errors.UserError('Re-entrant call to ExecuteDefer().')

  # Save directory, so SConscript functions can occur in the right subdirs
  oldcwd = os.getcwd()

  # If defer root is set and isn't this environment, we're being called from a
  # sub-environment.  That's not where we should be called.
  if self.GetDeferRoot() != self:
    print ('Warning: Ignoring call to ExecuteDefer() from child of the '
           'environment passed to SetDeferRoot().')
    return

  # Get list of defer groups from ourselves.
  defer_groups = GetDeferGroups(self)

  # Loop through deferred functions
  try:
    while defer_groups:
      did_work = False
      for name, group in defer_groups.items():
        if group.after.intersection(defer_groups.keys()):
          continue        # Still have dependencies

        # Set defer context
        _execute_defer_context = name

        # Remove this group from the list of defer groups now, in case one of
        # the functions it calls adds back a function into that defer group.
        del defer_groups[name]

        if group.func_env_cwd:
          # Run all the functions in our named group
          for func, env, cwd in group.func_env_cwd:
            os.chdir(cwd)
            func(env)

        # The defer groups have been altered, so restart the search for
        # functions that can be executed.
        did_work = True
        break

      if not did_work:
        errmsg = 'Error in ExecuteDefer: dependency cycle detected.\n'
        for name, group in defer_groups.items():
          errmsg += '   %s after: %s\n' % (name, group.after)
        raise SCons.Errors.UserError(errmsg)
  finally:
    # No longer in a defer context
    _execute_defer_context = None

  # Restore directory
  os.chdir(oldcwd)


def PrintDefer(self, print_functions=True):
  """Prints the current defer dependency graph.

  Args:
    self: Environment in which PrintDefer() was called.
    print_functions: Print individual functions in defer groups.
  """
  # Get the defer dict
  # Get list of defer groups from ourselves.
  defer_groups = GetDeferGroups(self)
  dgkeys = defer_groups.keys()
  dgkeys.sort()
  for k in dgkeys:
    print ' +- %s' % k
    group = defer_groups[k]
    after = list(group.after)
    if after:
      print ' |  after'
      after.sort()
      for a in after:
        print ' |   +- %s' % a
    if print_functions and group.func_env_cwd:
      print '    functions'
      for func, env, cwd in group.func_env_cwd:
        print ' |   +- %s %s' % (func.__name__, cwd)


def Defer(self, *args, **kwargs):
  """Adds a deferred function or modifies defer dependencies.

  Args:
    self: Environment in which Defer() was called
    args: Positional arguments
    kwargs: Named arguments

  The deferred function will be passed the environment used to call Defer(),
  and will be executed in the same working directory as the calling SConscript.
  (Exception: if this environment is cloned and the clone calls SetDeferRoot()
  and then ExecuteDefer(), the function will be passed the root environment,
  instead of the environment used to call Defer().)

  All deferred functions run after all SConscripts.  Additional dependencies
  may be specified with the after= keyword.

  Usage:

    env.Defer(func)
      # Defer func() until after all SConscripts

    env.Defer(func, after=otherfunc)
      # Defer func() until otherfunc() runs

    env.Defer(func, 'bob')
      # Defer func() until after SConscripts, put in group 'bob'

    env.Defer(func2, after='bob')
      # Defer func2() until after all funcs in 'bob' group have run

    env.Defer(func3, 'sam')
      # Defer func3() until after SConscripts, put in group 'sam'

    env.Defer('bob', after='sam')
      # Defer all functions in group 'bob' until after all functions in group
      # 'sam' have run.

    env.Defer(func4, after=['bob', 'sam'])
      # Defer func4() until after all functions in groups 'bob' and 'sam' have
      # run.
  """
  # Get name of group to defer and/or the a function
  name = None
  func = None
  for a in args:
    if isinstance(a, str):
      name = a
    elif isinstance(a, types.FunctionType):
      func = a
  if func and not name:
    name = func.__name__

  # TODO: Why not allow multiple functions?  Should be ok

  # Get list of names and/or functions this function should defer until after
  after = []
  for a in self.Flatten(kwargs.get('after')):
    if isinstance(a, str):
      # TODO: Should check if '$' in a, and if so, subst() it and recurse into
      # it.
      after.append(a)
    elif isinstance(a, types.FunctionType):
      after.append(a.__name__)
    elif a is not None:
      # Deferring
      raise ValueError('Defer after=%r is not a function or name' % a)

  # Find the deferred function
  defer_groups = GetDeferGroups(self)
  if name not in defer_groups:
    defer_groups[name] = DeferGroup()
  group = defer_groups[name]

  # If we were given a function, also save environment and current directory
  if func:
    group.func_env_cwd.append((func, self, os.getcwd()))

  # Add dependencies for the function
  group.after.update(after)

  # If we are already inside a call to ExecuteDefer(), any functions which are
  # deferring until after the current function must also be deferred until
  # after this new function.  In short, this means that if b() defers until
  # after a() and a() calls Defer() to defer c(), then b() must also defer
  # until after c().
  if _execute_defer_context and name != _execute_defer_context:
    for other_name, other_group in GetDeferGroups(self).items():
      if other_name == name:
        continue        # Don't defer after ourselves
      if _execute_defer_context in other_group.after:
        other_group.after.add(name)


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""
  env.Append(_DEFER_GROUPS={})

  env.AddMethod(Defer)
  env.AddMethod(ExecuteDefer)
  env.AddMethod(GetDeferRoot)
  env.AddMethod(PrintDefer)
  env.AddMethod(SetDeferRoot)
