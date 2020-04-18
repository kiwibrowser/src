#!/usr/bin/env python
# Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.


import errno
import json
import os.path
import shutil
import stat
import sys
import tempfile
import unittest
import distutils.dir_util

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PARENT_DIR = os.path.join(SCRIPT_DIR, os.pardir)
sys.path.append(PARENT_DIR)
from checkin_chromium_dep import Config, CheckinDependency, RunCommand, \
  DependencyNotFound

CHECKOUT_SRC_DIR = os.path.realpath(os.path.join(SCRIPT_DIR, os.pardir,
                                                 os.pardir))
FAKE_REMOTE_TEMPLATE_ROOT = os.path.join(SCRIPT_DIR, 'testdata',
                                         'checkin_chromium_dep', 'remote_root')
FAKE_SOURCE_TEMPLATE_ROOT = os.path.join(SCRIPT_DIR, 'testdata',
                                         'checkin_chromium_dep', 'src_root')


def _HandleRemoveReadonly(func, path, exc):
  excvalue = exc[1]
  if func in (os.rmdir, os.remove) and excvalue.errno == errno.EACCES:
    os.chmod(path, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)  # 0777
    func(path)
  else:
    raise excvalue


class TestCheckInChromiumDep(unittest.TestCase):
  def setUp(self):
    self._temp_dir = tempfile.mkdtemp(prefix='webrtc_test_')
    self._fake_chromium_repo = tempfile.mkdtemp(prefix='webrtc_test_')
    self._fake_source_repo = tempfile.mkdtemp(prefix='webrtc_test_')

    print("Temp dir: %s\n"
          "Chromium third_party fake repo: %s\n"
          "WebRTC source fake repo: %s" % (
            self._temp_dir, self._fake_chromium_repo,
            self._fake_source_repo))

    self._fake_chromium_revision = TestCheckInChromiumDep._InitFakeChromiumRepo(
        self._fake_chromium_repo)
    TestCheckInChromiumDep._InitFakeSourceRepo(self._fake_source_repo,
                                               self._fake_chromium_revision)

  @staticmethod
  def _InitFakeChromiumRepo(repo_dir):
    RunCommand(['git', 'init'], working_dir=repo_dir)
    distutils.dir_util.copy_tree(FAKE_REMOTE_TEMPLATE_ROOT, repo_dir)
    RunCommand(['git', 'add', '-A', '.'], working_dir=repo_dir)
    RunCommand(['git', 'commit', '-m', 'Init'],
               working_dir=repo_dir)
    stdout, _ = RunCommand(['git', 'rev-parse', 'HEAD'], working_dir=repo_dir)
    return stdout.strip()

  @staticmethod
  def _InitFakeSourceRepo(repo_dir, chromium_third_party_revision):
    RunCommand(['git', 'init'], working_dir=repo_dir)
    # Copy repo template
    distutils.dir_util.copy_tree(FAKE_SOURCE_TEMPLATE_ROOT, repo_dir)
    # Set right chromium third_party revision in DEPS file
    with open(os.path.join(repo_dir, 'DEPS'), 'rb') as f:
      deps_content = f.read()
    deps_content = deps_content % chromium_third_party_revision
    with open(os.path.join(repo_dir, 'DEPS'), 'wb') as f:
      f.write(deps_content)
    # Commit all repo content
    RunCommand(['git', 'add', '-A', '.'], working_dir=repo_dir)
    RunCommand(['git', 'commit', '-m', 'Init'],
               working_dir=repo_dir)

  def tearDown(self):
    shutil.rmtree(self._temp_dir, ignore_errors=False,
                  onerror=_HandleRemoveReadonly)
    shutil.rmtree(self._fake_chromium_repo, ignore_errors=False,
                  onerror=_HandleRemoveReadonly)
    shutil.rmtree(self._fake_source_repo, ignore_errors=False,
                  onerror=_HandleRemoveReadonly)

  def testCheckIn(self):
    third_party_dir = os.path.join(self._fake_source_repo, 'third_party')

    CheckinDependency('dep_bar',
                      Config(
                          self._fake_source_repo,
                          'file://%s' % self._fake_chromium_repo,
                          self._temp_dir))
    third_party_deps_list_file = os.path.join(self._fake_source_repo,
                                              'THIRD_PARTY_CHROMIUM_DEPS.json')
    with open(third_party_deps_list_file, 'rb') as f:
      deps_list = json.load(f).get('dependencies', [])

    # New dependency appended to deps list file
    self.assertIn('dep_foo', deps_list)
    self.assertIn('dep_bar', deps_list)
    # Only new dependency was appended
    self.assertNotIn('dep_buzz', deps_list)
    # New dependency was copied into source tree
    self.assertIn('dep_bar', os.listdir(third_party_dir))
    self.assertIn(
        'source_file.js', os.listdir(os.path.join(third_party_dir, 'dep_bar')))
    # Only new dependency was copied into source tree
    self.assertNotIn('dep_buzz', os.listdir(third_party_dir))

  def testCheckInNotExistingDep(self):
    self.assertRaises(DependencyNotFound,
                      CheckinDependency,
                      'dep_missing',
                      Config(self._fake_source_repo,
                             'file://%s' % self._fake_chromium_repo,
                             self._temp_dir))


if __name__ == '__main__':
  unittest.main()
