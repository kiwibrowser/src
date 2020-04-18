# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Library for dry_run utilities."""

from __future__ import print_function

from chromite.lib import cros_logging as logging


class DryRunMgr(object):
  """Manage the calling of functions that make real changes.

  We'll automatically disable things when in dry_run mode.
  """

  __slots__ = (
      'dry_run',   # Boolean.  See __init__ docstring.
      'quiet',     # Boolean.  See __init__ docstring.
  )

  def __init__(self, dry_run, quiet=False):
    """Create a DryRunMgr object.

    Args:
      dry_run: If True then this DryRunMgr will not execute the functions
        given to the Run method.
      quiet: If False, then when Run method skips functions (because of
        dry_run), then give a log message about skipping.
    """
    self.dry_run = dry_run
    self.quiet = quiet

  def __nonzero__(self):
    """This allows a DryRunMgr to serve as a Boolean proxy for self.dry_run."""
    return self.dry_run

  def __call__(self, func, *args, **kwargs):
    """See Run method, which this forwards to.

    This makes a DryRunMgr object callable.  Example:
    drm(os.remove, '/some/file')
    """
    return self.Run(func, *args, **kwargs)

  def Run(self, func, *args, **kwargs):
    """Run func(*args, **kwargs) if self.dry_run is not True.

    Examples:
    drm.Run(os.remove, '/some/file')

    Args:
      func: Must be a function object.
      args: Index-based arguments to pass to func.
      kwargs: Keyword-based arguments to pass to func.

    Returns:
      Whatever func returns if it is called, otherwise None.
    """
    func_name = None
    try:
      func_name = '%s.%s' % (func.__module__, func.__name__)
    except AttributeError:
      # This happens in unittests where func is a mocked function.
      # pylint: disable=W0212
      func_name = func._name
    except Exception as e:
      if 'UnknownMethodCallError' in type(e).__name__:
        # This is a mox exception that can happen in unittests when func is
        # mocked out with mox. In this case it's safe to just call the func.
        # Note: We are using the string match against the exception name
        # because mox is not always available for import here (and importing
        # mox into non-test code is dirty).
        return self._Call(func, *args, **kwargs)
      else:
        raise

    if self.dry_run:
      return self._Skip(func_name, *args, **kwargs)
    else:
      return self._Call(func, *args, **kwargs)

  def _Call(self, func, *args, **kwargs):
    """Call func(*args, **kwargs)."""
    return func(*args, **kwargs)

  def _Skip(self, func_name, *args, **kwargs):
    """If not quiet, give message about skipping func_name(*args, **kwargs)."""
    if not self.quiet:
      argstr_list = ([repr(a) for a in args] +
                     ['%s=%r' % (k, v) for k, v in kwargs.iteritems()])
      argstr = ', '.join(argstr_list)

      logging.info('dry-run skipping %s(%s)', func_name, argstr)
