# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(dtu): Merge this file with its counterparts in catapult_build/ when
# dashboard and PI migrate to Google Cloud SDK.

import contextlib
import os
import shutil
import tempfile
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from base import constants


def Modules(root_dir):
  """Yields module names in root_dir."""
  for root, _, files in os.walk(root_dir):
    for file_name in files:
      if os.path.splitext(file_name)[1] == '.yaml':
        yield os.path.basename(root)
        break


def Yamls(root_dir):
  """Yields yaml files in root_dir."""
  for root, _, files in os.walk(root_dir):
    for file_name in files:
      if os.path.splitext(file_name)[1] == '.yaml':
        yield os.path.join(root, file_name)


@contextlib.contextmanager
def TempAppDir(root_dir, symlinks):
  """Sets up and tears down a directory for deploying or running an app.

  Args:
    root_dir: The root directory of the app.
    symlinks: If true, use symbolic links instead of copying files. This allows
        the dev server to detect file changes in the repo, and is faster.

  Yields:
    The path to the temporary directory.
  """
  if symlinks:
    link = os.symlink
  else:
    def Link(src, dest):
      if os.path.isdir(src):
        return shutil.copytree(src, dest)
      else:
        return shutil.copy2(src, dest)
    link = Link

  project_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
  third_party_source_dir = os.path.join(os.path.dirname(project_dir),
                                        'third_party')
  temp_app_dir = tempfile.mkdtemp(prefix='app-')
  try:
    for module in Modules(root_dir):
      module_source_dir = os.path.join(root_dir, module)
      module_dest_dir = os.path.join(temp_app_dir, module)
      os.mkdir(module_dest_dir)

      # Copy/symlink module into app directory.
      for node in os.listdir(module_source_dir):
        link(os.path.join(module_source_dir, node),
             os.path.join(module_dest_dir, node))

      # Copy/symlink base/ into module directory.
      link(os.path.join(root_dir, 'base'),
           os.path.join(module_dest_dir, 'base'))

      # Copy/symlink Gcloud library dependencies into module directory.
      third_party_dest_dir = os.path.join(module_dest_dir, 'third_party')
      os.mkdir(third_party_dest_dir)
      for library in constants.THIRD_PARTY_LIBRARIES:
        link(os.path.join(third_party_source_dir, library),
             os.path.join(third_party_dest_dir, library))

    yield temp_app_dir
  finally:
    shutil.rmtree(temp_app_dir)
