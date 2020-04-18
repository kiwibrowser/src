# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The `depot_tools` module provides safe functions to access paths within
the depot_tools repo."""

import contextlib

from recipe_engine import recipe_api

class DepotToolsApi(recipe_api.RecipeApi):
  @property
  def download_from_google_storage_path(self):
    return self.package_repo_resource('download_from_google_storage.py')

  @property
  def upload_to_google_storage_path(self):
    return self.package_repo_resource('upload_to_google_storage.py')

  @property
  def root(self):
    """Returns (Path): The "depot_tools" root directory."""
    return self.package_repo_resource()

  @property
  def cros_path(self):
    return self.package_repo_resource('cros')

  @property
  def gn_py_path(self):
    return self.package_repo_resource('gn.py')

  # TODO(dnj): Remove this once everything uses the "gsutil" recipe module
  # version.
  @property
  def gsutil_py_path(self):
    return self.package_repo_resource('gsutil.py')

  @property
  def ninja_path(self):
    ninja_exe = 'ninja.exe' if self.m.platform.is_win else 'ninja'
    return self.package_repo_resource(ninja_exe)

  @property
  def presubmit_support_py_path(self):
    return self.package_repo_resource('presubmit_support.py')

  @contextlib.contextmanager
  def on_path(self):
    """Use this context manager to put depot_tools on $PATH.

    Example:

      with api.depot_tools.on_path():
        # run some steps
    """
    # On buildbot we have to put this on the FRONT of path, to combat the
    # 'automatic' depot_tools. However, on LUCI, there is no automatic
    # depot_tools, so it's safer to put it at the END of path, where it won't
    # accidentally override e.g. python, vpython, etc.
    key = 'env_prefixes'
    if self.m.runtime.is_luci:
      key = 'env_suffixes'

    with self.m.context(**{key: {'PATH': [self.root]}}):
      yield
