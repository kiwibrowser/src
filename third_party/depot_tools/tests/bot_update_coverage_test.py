#!/usr/bin/env python
# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import codecs
import copy
import json
import os
import sys
import unittest

#import test_env  # pylint: disable=relative-import,unused-import

sys.path.insert(0, os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    'recipes', 'recipe_modules', 'bot_update', 'resources'))
import bot_update


class MockedPopen(object):
  """A fake instance of a called subprocess.

  This is meant to be used in conjunction with MockedCall.
  """
  def __init__(self, args=None, kwargs=None):
    self.args = args or []
    self.kwargs = kwargs or {}
    self.return_value = None
    self.fails = False

  def returns(self, rv):
    """Set the return value when this popen is called.

    rv can be a string, or a callable (eg function).
    """
    self.return_value = rv
    return self

  def check(self, args, kwargs):
    """Check to see if the given args/kwargs call match this instance.

    This does a partial match, so that a call to "git clone foo" will match
    this instance if this instance was recorded as "git clone"
    """
    if any(input_arg != expected_arg
           for (input_arg, expected_arg) in zip(args, self.args)):
      return False
    return self.return_value

  def __call__(self, args, kwargs):
    """Actually call this popen instance."""
    if hasattr(self.return_value, '__call__'):
      return self.return_value(*args, **kwargs)
    return self.return_value


class MockedCall(object):
  """A fake instance of bot_update.call().

  This object is pre-seeded with "answers" in self.expectations.  The type
  is a MockedPopen object, or any object with a __call__() and check() method.
  The check() method is used to check to see if the correct popen object is
  chosen (can be a partial match, eg a "git clone" popen module would match
  a "git clone foo" call).
  By default, if no answers have been pre-seeded, the call() returns successful
  with an empty string.
  """
  def __init__(self, fake_filesystem):
    self.expectations = []
    self.records = []

  def expect(self, args=None, kwargs=None):
    args = args or []
    kwargs = kwargs or {}
    popen = MockedPopen(args, kwargs)
    self.expectations.append(popen)
    return popen

  def __call__(self, *args, **kwargs):
    self.records.append((args, kwargs))
    for popen in self.expectations:
      if popen.check(args, kwargs):
        self.expectations.remove(popen)
        return popen(args, kwargs)
    return ''


class MockedGclientSync():
  """A class producing a callable instance of gclient sync.

  Because for bot_update, gclient sync also emits an output json file, we need
  a callable object that can understand where the output json file is going, and
  emit a (albite) fake file for bot_update to consume.
  """
  def __init__(self, fake_filesystem):
    self.output = {}
    self.fake_filesystem = fake_filesystem
    self.records = []

  def __call__(self, *args, **_):
    output_json_index = args.index('--output-json') + 1
    with self.fake_filesystem.open(args[output_json_index], 'w') as f:
      json.dump(self.output, f)
    self.records.append(args)


class FakeFile():
  def __init__(self):
    self.contents = ''

  def write(self, buf):
    self.contents += buf

  def read(self):
    return self.contents

  def __enter__(self):
    return self

  def __exit__(self, _, __, ___):
    pass


class FakeFilesystem():
  def __init__(self):
    self.files = {}

  def open(self, target, mode='r', encoding=None):
    if 'w' in mode:
      self.files[target] = FakeFile()
      return self.files[target]
    return self.files[target]


def fake_git(*args, **kwargs):
  return bot_update.call('git', *args, **kwargs)


class BotUpdateUnittests(unittest.TestCase):
  DEFAULT_PARAMS = {
      'solutions': [{
          'name': 'somename',
          'url': 'https://fake.com'
      }],
      'revisions': {},
      'first_sln': 'somename',
      'target_os': None,
      'target_os_only': None,
      'target_cpu': None,
      'patch_root': None,
      'gerrit_repo': None,
      'gerrit_ref': None,
      'gerrit_rebase_patch_ref': None,
      'shallow': False,
      'refs': [],
      'git_cache_dir': '',
      'cleanup_dir': None,
      'gerrit_reset': None,
      'disable_syntax_validation': False,
      'apply_patch_on_gclient': False,
  }

  def setUp(self):
    sys.platform = 'linux2'  # For consistency, ya know?
    self.filesystem = FakeFilesystem()
    self.call = MockedCall(self.filesystem)
    self.gclient = MockedGclientSync(self.filesystem)
    self.call.expect(
        (sys.executable, '-u', bot_update.GCLIENT_PATH, 'sync')
    ).returns(self.gclient)
    self.old_call = getattr(bot_update, 'call')
    self.params = copy.deepcopy(self.DEFAULT_PARAMS)
    setattr(bot_update, 'call', self.call)
    setattr(bot_update, 'git', fake_git)

    self.old_os_cwd = os.getcwd
    setattr(os, 'getcwd', lambda: '/b/build/slave/foo/build')

    setattr(bot_update, 'open', self.filesystem.open)
    self.old_codecs_open = codecs.open
    setattr(codecs, 'open', self.filesystem.open)

  def tearDown(self):
    setattr(bot_update, 'call', self.old_call)
    setattr(os, 'getcwd', self.old_os_cwd)
    delattr(bot_update, 'open')
    setattr(codecs, 'open', self.old_codecs_open)

  def overrideSetupForWindows(self):
    sys.platform = 'win'
    self.call.expect(
        (sys.executable, '-u', bot_update.GCLIENT_PATH, 'sync')
    ).returns(self.gclient)

  def testBasic(self):
    bot_update.ensure_checkout(**self.params)
    return self.call.records

  def testBasicShallow(self):
    self.params['shallow'] = True
    bot_update.ensure_checkout(**self.params)
    return self.call.records

  def testBasicRevision(self):
    self.params['revisions'] = {
        'src': 'HEAD', 'src/v8': 'deadbeef', 'somename': 'DNE'}
    bot_update.ensure_checkout(**self.params)
    args = self.gclient.records[0]
    idx_first_revision = args.index('--revision')
    idx_second_revision = args.index(
        '--revision', idx_first_revision+1)
    idx_third_revision = args.index('--revision', idx_second_revision+1)
    self.assertEquals(args[idx_first_revision+1], 'somename@unmanaged')
    self.assertEquals(args[idx_second_revision+1], 'src@origin/master')
    self.assertEquals(args[idx_third_revision+1], 'src/v8@deadbeef')
    return self.call.records

  def testEnableGclientExperiment(self):
    self.params['gerrit_ref'] = 'refs/changes/12/345/6'
    self.params['gerrit_repo'] = 'https://chromium.googlesource.com/v8/v8'
    self.params['apply_patch_on_gclient'] = True
    bot_update.ensure_checkout(**self.params)
    args = self.gclient.records[0]
    idx = args.index('--patch-ref')
    self.assertEqual(
        args[idx+1],
        self.params['gerrit_repo'] + '@' + self.params['gerrit_ref'])
    self.assertNotIn('--patch-ref', args[idx+1:])
    # Assert we're not patching in bot_update.py
    for record in self.call.records:
      self.assertNotIn('git fetch ' + self.params['gerrit_repo'],
                       ' '.join(record[0]))

  def testBreakLocks(self):
    self.overrideSetupForWindows()
    bot_update.ensure_checkout(**self.params)
    gclient_sync_cmd = None
    for record in self.call.records:
      args = record[0]
      if args[:4] == (sys.executable, '-u', bot_update.GCLIENT_PATH, 'sync'):
        gclient_sync_cmd = args
    self.assertTrue('--break_repo_locks' in gclient_sync_cmd)

  def testGitCheckoutBreaksLocks(self):
    self.overrideSetupForWindows()
    path = '/b/build/slave/foo/build/.git'
    lockfile = 'index.lock'
    removed = []
    old_os_walk = os.walk
    old_os_remove = os.remove
    setattr(os, 'walk', lambda _: [(path, None, [lockfile])])
    setattr(os, 'remove', removed.append)
    bot_update.ensure_checkout(**self.params)
    setattr(os, 'walk', old_os_walk)
    setattr(os, 'remove', old_os_remove)
    self.assertTrue(os.path.join(path, lockfile) in removed)

  def testGenerateManifestsBasic(self):
    gclient_output = {
        'solutions': {
            'breakpad/': {
                'revision': None,
                'scm': None,
                'url': ('https://chromium.googlesource.com/breakpad.git' +
                        '@5f638d532312685548d5033618c8a36f73302d0a')
            },
            "src/": {
                'revision': 'f671d3baeb64d9dba628ad582e867cf1aebc0207',
                'scm': None,
                'url': 'https://chromium.googlesource.com/a/chromium/src.git'
            },
            'src/overriden': {
                'revision': None,
                'scm': 'git',
                'url': None,
            },
        }
    }
    out = bot_update.create_manifest(gclient_output, None, None)
    self.assertEquals(len(out['directories']), 2)
    self.assertEquals(
        out['directories']['src']['git_checkout']['revision'],
        'f671d3baeb64d9dba628ad582e867cf1aebc0207')
    self.assertEquals(
        out['directories']['src']['git_checkout']['repo_url'],
        'https://chromium.googlesource.com/chromium/src')
    self.assertEquals(
        out['directories']['breakpad']['git_checkout']['revision'],
        '5f638d532312685548d5033618c8a36f73302d0a')
    self.assertEquals(
        out['directories']['breakpad']['git_checkout']['repo_url'],
        'https://chromium.googlesource.com/breakpad')
    self.assertNotIn('src/overridden', out['directories'])


if __name__ == '__main__':
  unittest.main()
