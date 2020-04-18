# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import importlib
import logging

from distutils import version  # pylint: disable=no-name-in-module

# LooseVersion allows versions like "1.8.0rc1" (default numpy on macOS Sierra)
# and "2.4.13.2" (a version of OpenCV 2.x).
MODULES = {
    'cv2': (version.LooseVersion('2.4.8'), version.LooseVersion('3.0.0')),
    'numpy': (version.LooseVersion('1.8.0'), version.LooseVersion('1.12.0')),
    'psutil': (version.LooseVersion('0.5.0'), None),
}

def ImportRequiredModule(module):
  """Tries to import the desired module.

  Returns:
    The module on success, raises error on failure.
  Raises:
    ImportError: The import failed."""
  versions = MODULES.get(module)
  if versions is None:
    raise NotImplementedError('Please teach telemetry about module %s.' %
                              module)
  min_version, max_version = versions

  module = importlib.import_module(module)
  if ((min_version is not None and
       version.LooseVersion(module.__version__) < min_version) or
      (max_version is not None and
       version.LooseVersion(module.__version__) >= max_version)):
    raise ImportError(('Incorrect {0} version found, expected {1} <= version '
                       '< {2}, found version {3}').format(
                           module.__name__, min_version, max_version,
                           module.__version__))
  return module

def ImportOptionalModule(module):
  """Tries to import the desired module.

  Returns:
    The module if successful, None if not."""
  try:
    return ImportRequiredModule(module)
  except ImportError as e:
    # This can happen due to a circular dependency. It is usually not a
    # failure to import module_name, but a failed import somewhere in
    # the implementation. It's important to re-raise the error here
    # instead of failing silently.
    if 'cannot import name' in str(e):
      print 'Possible circular dependency!'
      raise
    logging.warning('Unable to import %s due to: %s', module, e)
    return None
