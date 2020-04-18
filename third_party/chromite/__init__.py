# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function

import os
import pkg_resources
import sys

# Add the third_party/ dir to our search path so that we can find the
# modules in there automatically.  This isn't normal, so don't replicate
# this pattern elsewhere.
_chromite_dir = os.path.normpath(os.path.dirname(os.path.realpath(__file__)))
_containing_dir = os.path.dirname(_chromite_dir)
_third_party_dirs = [os.path.join(_chromite_dir, 'third_party')]
# If chromite is living inside the Chrome checkout under
# <chrome_root>/src/third_party/chromite, its dependencies will be checked out
# to <chrome_root>/src/third_party instead of the normal chromite/third_party
# location due to git-submodule limitations (a submodule cannot be contained
# inside another submodule's workspace), so we want to add that to the
# search path.
if os.path.basename(_containing_dir) == 'third_party':
  _third_party_dirs.append(_containing_dir)

# List of third_party directories that might need subpaths added to search.
_extra_import_paths = [
    'dpkt',
    'google',
    'pyelftools',
]

for _path in _extra_import_paths:
  for _third_party_dir in _third_party_dirs[:]:
    _component = os.path.join(_third_party_dir, _path)
    if os.path.isdir(_component):
      _third_party_dirs.append(_component)

_insert_at = 0
if hasattr(sys, 'real_prefix'):
  # sys.real_prefix should only exist in virtualenv (https://stackoverflow.com/
  # questions/1871549/python-determine-if-running-inside-virtualenv#1883251)
  # When it's in virtualenv, make sure the packages installed by the virtualenv
  # have precedence over the third_party libraries.
  virtual_env_path = sys.prefix
  for _i, _path in reversed(list(enumerate(sys.path))):
    if virtual_env_path in _path:
      _insert_at = _i + 1
      break

sys.path[_insert_at:_insert_at] = _third_party_dirs

# Fix the .__path__ attributes of these submodules to correspond with sys.path.
# This prevents globally installed packages from shadowing the third_party
# packages. See crbug.com/674760 and
# https://github.com/google/protobuf/issues/1484 for more context.
# TODO(phobbs) this won't be necessary when we use venv everywhere.
_pkg_resources = __import__('pkg_resources')
for _package in ['google', 'google.protobuf']:
  _pkg_resources.declare_namespace(_package)
for _path in _third_party_dirs:
  _pkg_resources.fixup_namespace_packages(_path)

# Make sure we're only using the local google.protobuf.
import google.protobuf
google.protobuf.__path__ = [_p for _p in google.protobuf.__path__
                            if any(_dir in _p for _dir in _third_party_dirs)]
