# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import os


def _AddToPathIfNeeded(path):
  if path not in sys.path:
    sys.path.insert(0, path)


def UpdateSysPathIfNeeded():
  p = DashboardProject()

  _AddToPathIfNeeded(p.catapult_third_party_path)
  _AddToPathIfNeeded(p.catapult_path)
  _AddToPathIfNeeded(p.tracing_root_path)
  import tracing_project
  tracing_project.UpdateSysPathIfNeeded()


def _FindAllFilesRecursive(source_paths):
  assert isinstance(source_paths, list)
  all_filenames = set()
  for source_path in source_paths:
    for dirpath, _, filenames in os.walk(source_path):
      for f in filenames:
        if f.startswith('.'):
          continue
        x = os.path.abspath(os.path.join(dirpath, f))
        all_filenames.add(x)
  return all_filenames


def _IsFilenameATest(x):
  if x.endswith('-test.html'):
    return True
  if x.endswith('_test.html'):
    return True
  return False


class DashboardProject(object):
  catapult_path = os.path.abspath(
      os.path.join(os.path.dirname(__file__), '..'))

  catapult_third_party_path = os.path.join(catapult_path, 'third_party')

  dashboard_root_path = os.path.join(catapult_path, 'dashboard')
  dashboard_src_path = os.path.join(dashboard_root_path, 'dashboard')
  dashboard_test_data_path = os.path.join(dashboard_root_path, 'test_data')
  dashboard_polymer_path = os.path.join(catapult_third_party_path, 'polymer')

  tracing_root_path = os.path.join(catapult_path, 'tracing')

  def __init__(self):
    self._source_paths = None

  @property
  def source_paths(self):
    # We lazily init source_paths to resolve this cyclic dependency
    # (See perf_insights_project.py).
    if self._source_paths is None:
      self._source_paths = []
      self._source_paths.append(self.dashboard_root_path)
      self._source_paths.append(self.dashboard_polymer_path)
      self._source_paths.append(self.catapult_third_party_path)

      import tracing_project as tracing_project_module
      tracing_project = tracing_project_module.TracingProject()
      self._source_paths.extend(tracing_project.source_paths)

    return self._source_paths

  def FindAllTestModuleRelPaths(self, pred=None):
    if pred is None:
      pred = lambda x: True
    all_filenames = _FindAllFilesRecursive([self.dashboard_src_path])
    test_module_filenames = [x for x in all_filenames if
                             _IsFilenameATest(x) and pred(x)]
    test_module_filenames.sort()

    return [os.path.relpath(x, self.dashboard_root_path)
            for x in test_module_filenames]
