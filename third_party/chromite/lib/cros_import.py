# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper functions for importing python modules."""

from __future__ import print_function


def ImportModule(target):
  """Import |target| and return a reference to it.

  This uses the same import mechanism as the "import" statement.  That means
  references get added to the global scope and the results are cached.  This
  is different from the imp module which always loads and does not update the
  existing variable/module/etc... scope.

  Examples:
    # This statement:
    module = ImportModule('chromite.lib.cros_build_lib')
    # Is equivalent to:
    module = ImportModule(['chromite', 'lib', 'cros_build_lib'])
    # Is equivalent to:
    import chromite.lib.cros_build_lib
    module = chromite.lib.cros_build_lib

  Args:
    target: A name like you'd use with the "import" statement (ignoring the
      "from" part). May also be an iterable of the path components.

  Returns:
    A reference to the module.
  """
  # Normalize |target| into a dotted string and |parts| into a list.
  if isinstance(target, basestring):
    parts = target.split('.')
  else:
    parts = list(target)
    target = '.'.join(parts)

  # This caches things like normal, so no need to worry about overhead of
  # reloading modules multiple times.
  module = __import__(target)

  # __import__ gets us the root of the namespace import; walk our way up.
  for attr in parts[1:]:
    module = getattr(module, attr)

  return module
