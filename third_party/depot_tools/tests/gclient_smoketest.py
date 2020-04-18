#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Smoke tests for gclient.py.

Shell out 'gclient' and run basic conformance tests.

This test assumes GClientSmokeBase.URL_BASE is valid.
"""

import json
import logging
import os
import re
import subprocess
import sys
import unittest

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT_DIR)

import gclient_utils
import scm as gclient_scm
import subprocess2
from testing_support import fake_repos
from testing_support.fake_repos import join, write

GCLIENT_PATH = os.path.join(ROOT_DIR, 'gclient')
COVERAGE = False


class GClientSmokeBase(fake_repos.FakeReposTestBase):
  def setUp(self):
    super(GClientSmokeBase, self).setUp()
    # Make sure it doesn't try to auto update when testing!
    self.env = os.environ.copy()
    self.env['DEPOT_TOOLS_UPDATE'] = '0'

  def gclient(self, cmd, cwd=None):
    if not cwd:
      cwd = self.root_dir
    if COVERAGE:
      # Don't use the wrapper script.
      cmd_base = ['coverage', 'run', '-a', GCLIENT_PATH + '.py']
    else:
      cmd_base = [GCLIENT_PATH]
    cmd = cmd_base + cmd
    process = subprocess.Popen(cmd, cwd=cwd, env=self.env,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               shell=sys.platform.startswith('win'))
    (stdout, stderr) = process.communicate()
    logging.debug("XXX: %s\n%s\nXXX" % (' '.join(cmd), stdout))
    logging.debug("YYY: %s\n%s\nYYY" % (' '.join(cmd), stderr))
    return (stdout.replace('\r\n', '\n'), stderr.replace('\r\n', '\n'),
            process.returncode)

  def untangle(self, stdout):
    tasks = {}
    remaining = []
    for line in stdout.splitlines(False):
      m = re.match(r'^(\d)+>(.*)$', line)
      if not m:
        remaining.append(line)
      else:
        self.assertEquals([], remaining)
        tasks.setdefault(int(m.group(1)), []).append(m.group(2))
    out = []
    for key in sorted(tasks.iterkeys()):
      out.extend(tasks[key])
    out.extend(remaining)
    return '\n'.join(out)

  def parseGclient(self, cmd, items, expected_stderr='', untangle=False):
    """Parse gclient's output to make it easier to test.
    If untangle is True, tries to sort out the output from parallel checkout."""
    (stdout, stderr, returncode) = self.gclient(cmd)
    if untangle:
      stdout = self.untangle(stdout)
    self.checkString(expected_stderr, stderr)
    self.assertEquals(0, returncode)
    return self.checkBlock(stdout, items)

  def splitBlock(self, stdout):
    """Split gclient's output into logical execution blocks.
    ___ running 'foo' at '/bar'
    (...)
    ___ running 'baz' at '/bar'
    (...)

    will result in 2 items of len((...).splitlines()) each.
    """
    results = []
    for line in stdout.splitlines(False):
      # Intentionally skips empty lines.
      if not line:
        continue
      if not line.startswith('__'):
        if results:
          results[-1].append(line)
        else:
          # TODO(maruel): gclient's git stdout is inconsistent.
          # This should fail the test instead!!
          pass
        continue

      match = re.match(r'^________ ([a-z]+) \'(.*)\' in \'(.*)\'$', line)
      if match:
        results.append([[match.group(1), match.group(2), match.group(3)]])
        continue

      match = re.match(r'^_____ (.*) is missing, synching instead$', line)
      if match:
        # Blah, it's when a dependency is deleted, we should probably not
        # output this message.
        results.append([line])
        continue

      # These two regexps are a bit too broad, they are necessary only for git
      # checkouts.
      if (re.match(r'_____ [^ ]+ at [^ ]+', line) or
          re.match(r'_____ [^ ]+ : Attempting rebase onto [0-9a-f]+...', line)):
        continue

      # Fail for any unrecognized lines that start with '__'.
      self.fail(line)
    return results

  def checkBlock(self, stdout, items):
    results = self.splitBlock(stdout)
    for i in xrange(min(len(results), len(items))):
      if isinstance(items[i], (list, tuple)):
        verb = items[i][0]
        path = items[i][1]
      else:
        verb = items[i]
        path = self.root_dir
      self.checkString(results[i][0][0], verb, (i, results[i][0][0], verb))
      if sys.platform == 'win32':
        # Make path lower case since casing can change randomly.
        self.checkString(
            results[i][0][2].lower(),
            path.lower(),
            (i, results[i][0][2].lower(), path.lower()))
      else:
        self.checkString(results[i][0][2], path, (i, results[i][0][2], path))
    self.assertEquals(len(results), len(items), (stdout, items, len(results)))
    return results


class GClientSmoke(GClientSmokeBase):
  """Doesn't require git-daemon."""
  @property
  def git_base(self):
    return 'git://random.server/git/'

  def testNotConfigured(self):
    res = ('', 'Error: client not configured; see \'gclient config\'\n', 1)
    self.check(res, self.gclient(['diff']))
    self.check(res, self.gclient(['pack']))
    self.check(res, self.gclient(['revert']))
    self.check(res, self.gclient(['revinfo']))
    self.check(res, self.gclient(['runhooks']))
    self.check(res, self.gclient(['status']))
    self.check(res, self.gclient(['sync']))
    self.check(res, self.gclient(['update']))

  def testConfig(self):
    # Get any bootstrapping out of the way.
    results = self.gclient(['version'])
    self.assertEquals(results[2], 0)

    p = join(self.root_dir, '.gclient')
    def test(cmd, expected):
      if os.path.exists(p):
        os.remove(p)
      results = self.gclient(cmd)
      self.check(('', '', 0), results)
      self.checkString(expected, open(p, 'rU').read())

    test(['config', self.git_base + 'src/'],
         ('solutions = [\n'
          '  { "name"        : "src",\n'
          '    "url"         : "%ssrc",\n'
          '    "deps_file"   : "DEPS",\n'
          '    "managed"     : True,\n'
          '    "custom_deps" : {\n'
          '    },\n'
          '    "custom_vars": {},\n'
          '  },\n'
          ']\n'
          'cache_dir = None\n') % self.git_base)

    test(['config', self.git_base + 'repo_1', '--name', 'src'],
         ('solutions = [\n'
          '  { "name"        : "src",\n'
          '    "url"         : "%srepo_1",\n'
          '    "deps_file"   : "DEPS",\n'
          '    "managed"     : True,\n'
          '    "custom_deps" : {\n'
          '    },\n'
          '    "custom_vars": {},\n'
          '  },\n'
          ']\n'
          'cache_dir = None\n') % self.git_base)

    test(['config', 'https://example.com/foo', 'faa'],
         'solutions = [\n'
         '  { "name"        : "foo",\n'
         '    "url"         : "https://example.com/foo",\n'
         '    "deps_file"   : "DEPS",\n'
         '    "managed"     : True,\n'
         '    "custom_deps" : {\n'
         '    },\n'
         '    "custom_vars": {},\n'
         '  },\n'
         ']\n'
         'cache_dir = None\n')

    test(['config', 'https://example.com/foo', '--deps', 'blah'],
         'solutions = [\n'
         '  { "name"        : "foo",\n'
         '    "url"         : "https://example.com/foo",\n'
         '    "deps_file"   : "blah",\n'
         '    "managed"     : True,\n'
         '    "custom_deps" : {\n'
         '    },\n'
         '    "custom_vars": {},\n'
         '  },\n'
         ']\n'
         'cache_dir = None\n')

    test(['config', self.git_base + 'src/',
          '--custom-var', 'bool_var=True',
          '--custom-var', 'str_var="abc"'],
         ('solutions = [\n'
          '  { "name"        : "src",\n'
          '    "url"         : "%ssrc",\n'
          '    "deps_file"   : "DEPS",\n'
          '    "managed"     : True,\n'
          '    "custom_deps" : {\n'
          '    },\n'
          '    "custom_vars": {\'bool_var\': True, \'str_var\': \'abc\'},\n'
          '  },\n'
          ']\n'
          'cache_dir = None\n') % self.git_base)

    test(['config', '--spec', '["blah blah"]'], '["blah blah"]')

    os.remove(p)
    results = self.gclient(['config', 'foo', 'faa', 'fuu'])
    err = ('Usage: gclient.py config [options] [url]\n\n'
           'gclient.py: error: Inconsistent arguments. Use either --spec or one'
           ' or 2 args\n')
    self.check(('', err, 2), results)
    self.assertFalse(os.path.exists(join(self.root_dir, '.gclient')))

  def testSolutionNone(self):
    results = self.gclient(['config', '--spec',
                            'solutions=[{"name": "./", "url": None}]'])
    self.check(('', '', 0), results)
    results = self.gclient(['sync'])
    self.check(('', '', 0), results)
    self.assertTree({})
    results = self.gclient(['revinfo'])
    self.check(('./: None\n', '', 0), results)
    self.check(('', '', 0), self.gclient(['diff']))
    self.assertTree({})
    self.check(('', '', 0), self.gclient(['pack']))
    self.check(('', '', 0), self.gclient(['revert']))
    self.assertTree({})
    self.check(('', '', 0), self.gclient(['runhooks']))
    self.assertTree({})
    self.check(('', '', 0), self.gclient(['status']))

  def testDifferentTopLevelDirectory(self):
    # Check that even if the .gclient file does not mention the directory src
    # itself, but it is included via dependencies, the .gclient file is used.
    self.gclient(['config', self.git_base + 'src.DEPS'])
    deps = join(self.root_dir, 'src.DEPS')
    os.mkdir(deps)
    subprocess2.check_output(['git', 'init'], cwd=deps)
    write(join(deps, 'DEPS'),
        'deps = { "src": "%ssrc" }' % (self.git_base))
    subprocess2.check_output(['git', 'add', 'DEPS'], cwd=deps)
    subprocess2.check_output(
        ['git', 'commit', '-a', '-m', 'DEPS file'], cwd=deps)
    src = join(self.root_dir, 'src')
    os.mkdir(src)
    subprocess2.check_output(['git', 'init'], cwd=src)
    res = self.gclient(['status', '--jobs', '1', '-v'], src)
    self.checkBlock(res[0], [('running', deps), ('running', src)])


class GClientSmokeGIT(GClientSmokeBase):
  def setUp(self):
    super(GClientSmokeGIT, self).setUp()
    self.enabled = self.FAKE_REPOS.set_up_git()

  def testSync(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    # Test unversioned checkout.
    self.parseGclient(
        ['sync', '--deps', 'mac', '--jobs', '1'],
        ['running', 'running', 'running'])
    # TODO(maruel): http://crosbug.com/3582 hooks run even if not matching, must
    # add sync parsing to get the list of updated files.
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@1', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    self.assertTree(tree)

    # Manually remove git_hooked1 before synching to make sure it's not
    # recreated.
    os.remove(join(self.root_dir, 'src', 'git_hooked1'))

    # Test incremental versioned sync: sync backward.
    self.parseGclient(
        ['sync', '--jobs', '1', '--revision',
        'src@' + self.githash('repo_1', 1),
        '--deps', 'mac', '--delete_unversioned_trees'],
        ['deleting'])
    tree = self.mangle_git_tree(('repo_1@1', 'src'),
                                ('repo_2@2', 'src/repo2'),
                                ('repo_3@1', 'src/repo2/repo3'),
                                ('repo_4@2', 'src/repo4'))
    tree['src/git_hooked2'] = 'git_hooked2'
    tree['src/repo2/gclient.args'] = '\n'.join([
        '# Generated from \'DEPS\'',
        'false_var = false',
        'false_str_var = false',
        'true_var = true',
        'true_str_var = true',
        'str_var = "abc"',
        'cond_var = false',
    ])
    self.assertTree(tree)
    # Test incremental sync: delete-unversioned_trees isn't there.
    self.parseGclient(
        ['sync', '--deps', 'mac', '--jobs', '1'],
        ['running', 'running'])
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@1', 'src/repo2'),
                                ('repo_3@1', 'src/repo2/repo3'),
                                ('repo_3@2', 'src/repo2/repo_renamed'),
                                ('repo_4@2', 'src/repo4'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    tree['src/repo2/gclient.args'] = '\n'.join([
        '# Generated from \'DEPS\'',
        'false_var = false',
        'false_str_var = false',
        'true_var = true',
        'true_str_var = true',
        'str_var = "abc"',
        'cond_var = false',
    ])
    self.assertTree(tree)

  def testSyncJsonOutput(self):
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    output_json = os.path.join(self.root_dir, 'output.json')
    self.gclient(['sync', '--deps', 'mac', '--output-json', output_json])
    with open(output_json) as f:
      output_json = json.load(f)

    out = {
        'solutions': {
            'src/': {
                'scm': 'git',
                'url': self.git_base + 'repo_1',
                'revision': self.githash('repo_1', 2),
            },
            'src/repo2/': {
                'scm': 'git',
                'url':
                    self.git_base + 'repo_2@' + self.githash('repo_2', 1)[:7],
                'revision': self.githash('repo_2', 1),
            },
            'src/repo2/repo_renamed/': {
                'scm': 'git',
                'url': self.git_base + 'repo_3',
                'revision': self.githash('repo_3', 2),
            },
        },
    }
    self.assertEqual(out, output_json)

  def testSyncIgnoredSolutionName(self):
    """TODO(maruel): This will become an error soon."""
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.parseGclient(
        ['sync', '--deps', 'mac', '--jobs', '1',
         '--revision', 'invalid@' + self.githash('repo_1', 1)],
        ['running', 'running', 'running'],
        'Please fix your script, having invalid --revision flags '
        'will soon be considered an error.\n')
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@1', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    self.assertTree(tree)

  def testSyncNoSolutionName(self):
    if not self.enabled:
      return
    # When no solution name is provided, gclient uses the first solution listed.
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.parseGclient(
        ['sync', '--deps', 'mac', '--jobs', '1',
         '--revision', self.githash('repo_1', 1)],
        ['running'])
    tree = self.mangle_git_tree(('repo_1@1', 'src'),
                                ('repo_2@2', 'src/repo2'),
                                ('repo_3@1', 'src/repo2/repo3'),
                                ('repo_4@2', 'src/repo4'))
    tree['src/repo2/gclient.args'] = '\n'.join([
        '# Generated from \'DEPS\'',
        'false_var = false',
        'false_str_var = false',
        'true_var = true',
        'true_str_var = true',
        'str_var = "abc"',
        'cond_var = false',
    ])
    self.assertTree(tree)

  def testSyncJobs(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    # Test unversioned checkout.
    self.parseGclient(
        ['sync', '--deps', 'mac', '--jobs', '8'],
        ['running', 'running', 'running'],
        untangle=True)
    # TODO(maruel): http://crosbug.com/3582 hooks run even if not matching, must
    # add sync parsing to get the list of updated files.
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@1', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    self.assertTree(tree)

    # Manually remove git_hooked1 before synching to make sure it's not
    # recreated.
    os.remove(join(self.root_dir, 'src', 'git_hooked1'))

    # Test incremental versioned sync: sync backward.
    # Use --jobs 1 otherwise the order is not deterministic.
    self.parseGclient(
        ['sync', '--revision', 'src@' + self.githash('repo_1', 1),
          '--deps', 'mac', '--delete_unversioned_trees', '--jobs', '1'],
        ['deleting'],
        untangle=True)
    tree = self.mangle_git_tree(('repo_1@1', 'src'),
                                ('repo_2@2', 'src/repo2'),
                                ('repo_3@1', 'src/repo2/repo3'),
                                ('repo_4@2', 'src/repo4'))
    tree['src/git_hooked2'] = 'git_hooked2'
    tree['src/repo2/gclient.args'] = '\n'.join([
        '# Generated from \'DEPS\'',
        'false_var = false',
        'false_str_var = false',
        'true_var = true',
        'true_str_var = true',
        'str_var = "abc"',
        'cond_var = false',
    ])
    self.assertTree(tree)
    # Test incremental sync: delete-unversioned_trees isn't there.
    self.parseGclient(
        ['sync', '--deps', 'mac', '--jobs', '8'],
        ['running', 'running'],
        untangle=True)
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@1', 'src/repo2'),
                                ('repo_3@1', 'src/repo2/repo3'),
                                ('repo_3@2', 'src/repo2/repo_renamed'),
                                ('repo_4@2', 'src/repo4'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    tree['src/repo2/gclient.args'] = '\n'.join([
        '# Generated from \'DEPS\'',
        'false_var = false',
        'false_str_var = false',
        'true_var = true',
        'true_str_var = true',
        'str_var = "abc"',
        'cond_var = false',
    ])
    self.assertTree(tree)

  def testSyncFetch(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_13', '--name', 'src'])
    _out, _err, rc = self.gclient(['sync', '-v', '-v', '-v'])
    self.assertEquals(0, rc)

  def testSyncFetchUpdate(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_13', '--name', 'src'])

    # Sync to an earlier revision first, one that doesn't refer to
    # non-standard refs.
    _out, _err, rc = self.gclient(
        ['sync', '-v', '-v', '-v', '--revision', self.githash('repo_13', 1)])
    self.assertEquals(0, rc)

    # Make sure update that pulls a non-standard ref works.
    _out, _err, rc = self.gclient(['sync', '-v', '-v', '-v'])
    self.assertEquals(0, rc)

  def testSyncDirect(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_12', '--name', 'src'])
    _out, _err, rc = self.gclient(
        ['sync', '-v', '-v', '-v', '--revision', 'refs/changes/1212'])
    self.assertEquals(0, rc)

  def testSyncUrl(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient([
        'sync', '-v', '-v', '-v',
        '--revision', 'src/repo2@%s' % self.githash('repo_2', 1),
        '--revision', '%srepo_2@%s' % (self.git_base, self.githash('repo_2', 2))
    ])
    # repo_2 should've been synced to @2 instead of @1, since URLs override
    # paths.
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@2', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    self.assertTree(tree)

  def testSyncPatchRef(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient([
        'sync', '-v', '-v', '-v',
        '--revision', 'src/repo2@%s' % self.githash('repo_2', 1),
        '--patch-ref',
        '%srepo_2@%s' % (self.git_base, self.githash('repo_2', 2)),
    ])
    # Assert that repo_2 files coincide with revision @2 (the patch ref)
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@2', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    self.assertTree(tree)
    # Assert that HEAD revision of repo_2 is @1 (the base we synced to) since we
    # should have done a soft reset.
    self.assertEqual(
        self.githash('repo_2', 1),
        self.gitrevparse(os.path.join(self.root_dir, 'src/repo2')))

  def testSyncPatchRefNoHooks(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient([
        'sync', '-v', '-v', '-v',
        '--revision', 'src/repo2@%s' % self.githash('repo_2', 1),
        '--patch-ref',
        '%srepo_2@%s' % (self.git_base, self.githash('repo_2', 2)),
        '--nohooks',
    ])
    # Assert that repo_2 files coincide with revision @2 (the patch ref)
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@2', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    self.assertTree(tree)
    # Assert that HEAD revision of repo_2 is @1 (the base we synced to) since we
    # should have done a soft reset.
    self.assertEqual(
        self.githash('repo_2', 1),
        self.gitrevparse(os.path.join(self.root_dir, 'src/repo2')))

  def testRunHooks(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient(['sync', '--deps', 'mac'])
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@1', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    self.assertTree(tree)

    os.remove(join(self.root_dir, 'src', 'git_hooked1'))
    os.remove(join(self.root_dir, 'src', 'git_hooked2'))
    # runhooks runs all hooks even if not matching by design.
    out = self.parseGclient(['runhooks', '--deps', 'mac'],
                            ['running', 'running'])
    self.assertEquals(1, len(out[0]))
    self.assertEquals(1, len(out[1]))
    tree = self.mangle_git_tree(('repo_1@2', 'src'),
                                ('repo_2@1', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    self.assertTree(tree)

  def testRunHooksCondition(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_7', '--name', 'src'])
    self.gclient(['sync', '--deps', 'mac'])
    tree = self.mangle_git_tree(('repo_7@1', 'src'))
    tree['src/should_run'] = 'should_run'
    self.assertTree(tree)

  def testPreDepsHooks(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_5', '--name', 'src'])
    expectation = [
        ('running', self.root_dir),                 # git clone
        ('running', self.root_dir),                 # pre-deps hook
    ]
    out = self.parseGclient(['sync', '--deps', 'mac', '--jobs=1',
                             '--revision', 'src@' + self.githash('repo_5', 2)],
                            expectation)
    self.assertEquals('Cloning into ', out[0][1][:13])
    self.assertEquals(2, len(out[1]))
    self.assertEquals('pre-deps hook', out[1][1])
    tree = self.mangle_git_tree(('repo_5@2', 'src'),
                                ('repo_1@2', 'src/repo1'),
                                ('repo_2@1', 'src/repo2')
                                )
    tree['src/git_pre_deps_hooked'] = 'git_pre_deps_hooked'
    self.assertTree(tree)

    os.remove(join(self.root_dir, 'src', 'git_pre_deps_hooked'))

    # Pre-DEPS hooks don't run with runhooks.
    self.gclient(['runhooks', '--deps', 'mac'])
    tree = self.mangle_git_tree(('repo_5@2', 'src'),
                                ('repo_1@2', 'src/repo1'),
                                ('repo_2@1', 'src/repo2')
                                )
    self.assertTree(tree)

    # Pre-DEPS hooks run when syncing with --nohooks.
    self.gclient(['sync', '--deps', 'mac', '--nohooks',
                  '--revision', 'src@' + self.githash('repo_5', 2)])
    tree = self.mangle_git_tree(('repo_5@2', 'src'),
                                ('repo_1@2', 'src/repo1'),
                                ('repo_2@1', 'src/repo2')
                                )
    tree['src/git_pre_deps_hooked'] = 'git_pre_deps_hooked'
    self.assertTree(tree)

    os.remove(join(self.root_dir, 'src', 'git_pre_deps_hooked'))

    # Pre-DEPS hooks don't run with --noprehooks
    self.gclient(['sync', '--deps', 'mac', '--noprehooks',
                  '--revision', 'src@' + self.githash('repo_5', 2)])
    tree = self.mangle_git_tree(('repo_5@2', 'src'),
                                ('repo_1@2', 'src/repo1'),
                                ('repo_2@1', 'src/repo2')
                                )
    self.assertTree(tree)

  def testPreDepsHooksError(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_5', '--name', 'src'])
    expectated_stdout = [
        ('running', self.root_dir),                 # git clone
        ('running', self.root_dir),                 # pre-deps hook
        ('running', self.root_dir),                 # pre-deps hook (fails)
    ]
    expected_stderr = ("Error: Command '%s -c import sys; "
                       "sys.exit(1)' returned non-zero exit status 1 in %s\n"
                       % (sys.executable, self.root_dir))
    stdout, stderr, retcode = self.gclient(['sync', '--deps', 'mac', '--jobs=1',
                                            '--revision',
                                            'src@' + self.githash('repo_5', 3)])
    self.assertEquals(stderr, expected_stderr)
    self.assertEquals(2, retcode)
    self.checkBlock(stdout, expectated_stdout)

  def testRevInfo(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient(['sync', '--deps', 'mac'])
    results = self.gclient(['revinfo', '--deps', 'mac'])
    out = ('src: %(base)srepo_1\n'
           'src/repo2: %(base)srepo_2@%(hash2)s\n'
           'src/repo2/repo_renamed: %(base)srepo_3\n' %
          {
            'base': self.git_base,
            'hash2': self.githash('repo_2', 1)[:7],
          })
    self.check((out, '', 0), results)

  def testRevInfoActual(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient(['sync', '--deps', 'mac'])
    results = self.gclient(['revinfo', '--deps', 'mac', '--actual'])
    out = ('src: %(base)srepo_1@%(hash1)s\n'
           'src/repo2: %(base)srepo_2@%(hash2)s\n'
           'src/repo2/repo_renamed: %(base)srepo_3@%(hash3)s\n' %
          {
            'base': self.git_base,
            'hash1': self.githash('repo_1', 2),
            'hash2': self.githash('repo_2', 1),
            'hash3': self.githash('repo_3', 2),
          })
    self.check((out, '', 0), results)

  def testRevInfoFilterPath(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient(['sync', '--deps', 'mac'])
    results = self.gclient(['revinfo', '--deps', 'mac', '--filter', 'src'])
    out = ('src: %(base)srepo_1\n' %
          {
            'base': self.git_base,
          })
    self.check((out, '', 0), results)

  def testRevInfoFilterURL(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient(['sync', '--deps', 'mac'])
    results = self.gclient(['revinfo', '--deps', 'mac',
                            '--filter', '%srepo_2' % self.git_base])
    out = ('src/repo2: %(base)srepo_2@%(hash2)s\n' %
          {
            'base': self.git_base,
            'hash2': self.githash('repo_2', 1)[:7],
          })
    self.check((out, '', 0), results)

  def testRevInfoFilterURLOrPath(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient(['sync', '--deps', 'mac'])
    results = self.gclient(['revinfo', '--deps', 'mac', '--filter', 'src',
                            '--filter', '%srepo_2' % self.git_base])
    out = ('src: %(base)srepo_1\n'
           'src/repo2: %(base)srepo_2@%(hash2)s\n' %
          {
            'base': self.git_base,
            'hash2': self.githash('repo_2', 1)[:7],
          })
    self.check((out, '', 0), results)

  def testRevInfoJsonOutput(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient(['sync', '--deps', 'mac'])
    output_json = os.path.join(self.root_dir, 'output.json')
    self.gclient(['revinfo', '--deps', 'mac', '--output-json', output_json])
    with open(output_json) as f:
      output_json = json.load(f)

    out = {
        'src': {
            'url': self.git_base + 'repo_1',
            'rev': None,
        },
        'src/repo2': {
            'url': self.git_base + 'repo_2',
            'rev': self.githash('repo_2', 1)[:7],
        },
       'src/repo2/repo_renamed': {
           'url': self.git_base + 'repo_3',
           'rev': None,
        },
    }
    self.assertEqual(out, output_json)

  def testRevInfoJsonOutputSnapshot(self):
    if not self.enabled:
      return
    self.gclient(['config', self.git_base + 'repo_1', '--name', 'src'])
    self.gclient(['sync', '--deps', 'mac'])
    output_json = os.path.join(self.root_dir, 'output.json')
    self.gclient(['revinfo', '--deps', 'mac', '--snapshot',
                  '--output-json', output_json])
    with open(output_json) as f:
      output_json = json.load(f)

    out = [{
        'solution_url': self.git_base + 'repo_1',
        'managed': True,
        'name': 'src',
        'deps_file': 'DEPS',
        'custom_deps': {
            'src/repo2': '%srepo_2@%s' % (
                self.git_base, self.githash('repo_2', 1)),
            'src/repo2/repo_renamed': '%srepo_3@%s' % (
                self.git_base, self.githash('repo_3', 2)),
        },
    }]
    self.assertEqual(out, output_json)

  def testSetDep(self):
    fake_deps = os.path.join(self.root_dir, 'DEPS.fake')
    with open(fake_deps, 'w') as f:
      f.write('\n'.join([
          'vars = { ',
          '  "foo_var": "foo_val",',
          '  "foo_rev": "foo_rev",',
          '}',
          'deps = {',
          '  "foo": {',
          '    "url": "url@{foo_rev}",',
          '  },',
          '  "bar": "url@bar_rev",',
          '}',
      ]))

    self.gclient([
        'setdep', '-r', 'foo@new_foo', '-r', 'bar@new_bar',
        '--var', 'foo_var=new_val', '--deps-file', fake_deps])

    with open(fake_deps) as f:
      contents = f.read().splitlines()

    self.assertEqual([
          'vars = { ',
          '  "foo_var": "new_val",',
          '  "foo_rev": "new_foo",',
          '}',
          'deps = {',
          '  "foo": {',
          '    "url": "url@{foo_rev}",',
          '  },',
          '  "bar": "url@new_bar",',
          '}',
    ], contents)

  def testGetDep(self):
    fake_deps = os.path.join(self.root_dir, 'DEPS.fake')
    with open(fake_deps, 'w') as f:
      f.write('\n'.join([
          'vars = { ',
          '  "foo_var": "foo_val",',
          '  "foo_rev": "foo_rev",',
          '}',
          'deps = {',
          '  "foo": {',
          '    "url": "url@{foo_rev}",',
          '  },',
          '  "bar": "url@bar_rev",',
          '}',
      ]))

    results = self.gclient([
        'getdep', '-r', 'foo', '-r', 'bar','--var', 'foo_var',
        '--deps-file', fake_deps])

    self.assertEqual([
        'foo_val',
        'foo_rev',
        'bar_rev',
    ], results[0].splitlines())

  def testFlatten(self):
    if not self.enabled:
      return

    output_deps = os.path.join(self.root_dir, 'DEPS.flattened')
    self.assertFalse(os.path.exists(output_deps))

    self.gclient(['config', self.git_base + 'repo_6', '--name', 'src',
                  # This should be ignored because 'custom_true_var' isn't
                  # defined in the DEPS.
                  '--custom-var', 'custom_true_var=True',
                  # This should override 'true_var=True' from the DEPS.
                  '--custom-var', 'true_var="False"'])
    self.gclient(['sync'])
    self.gclient(['flatten', '-v', '-v', '-v', '--output-deps', output_deps])

    # Assert we can sync to the flattened DEPS we just wrote.
    solutions = [{
        "url": self.git_base + 'repo_6',
        'name': 'src',
        'deps_file': output_deps
    }]
    results = self.gclient([
        'sync',
        '--spec=solutions=%s' % solutions
    ])
    self.assertEqual(results[2], 0)

    with open(output_deps) as f:
      deps_contents = f.read()

    self.maxDiff = None  # pylint: disable=attribute-defined-outside-init
    self.assertEqual([
        'gclient_gn_args_file = "src/repo2/gclient.args"',
        'gclient_gn_args = [\'false_var\', \'false_str_var\', \'true_var\', '
            '\'true_str_var\', \'str_var\', \'cond_var\']',
        'allowed_hosts = [',
        '  "' + self.git_base + '",',
        ']',
        '',
        'deps = {',
        '  # src -> src/repo2 -> foo/bar',
        '  "foo/bar": {',
        '    "url": "/repo_3",',
        '    "condition": \'(repo2_false_var) and (true_str_var)\',',
        '  },',
        '',
        '  # src',
        '  "src": {',
        '    "url": "' + self.git_base + 'repo_6",',
        '  },',
        '',
        '  # src -> src/mac_repo',
        '  "src/mac_repo": {',
        '    "url": "{repo5_var}",',
        '    "condition": \'checkout_mac\',',
        '  },',
        '',
        '  # src -> src/repo8 -> src/recursed_os_repo',
        '  "src/recursed_os_repo": {',
        '    "url": "/repo_5",',
        '    "condition": \'(checkout_linux) or (checkout_mac)\',',
        '  },',
        '',
        '  # src -> src/repo2',
        '  "src/repo2": {',
        '    "url": "{git_base}repo_2@%s",' % (
                 self.githash('repo_2', 1)[:7]),
        '    "condition": \'true_str_var\',',
        '  },',
        '',
        '  # src -> src/repo4',
        '  "src/repo4": {',
        '    "url": "/repo_4",',
        '    "condition": \'False\',',
        '  },',
        '',
        '  # src -> src/repo8',
        '  "src/repo8": {',
        '    "url": "/repo_8",',
        '  },',
        '',
        '  # src -> src/unix_repo',
        '  "src/unix_repo": {',
        '    "url": "{repo5_var}",',
        '    "condition": \'checkout_linux\',',
        '  },',
        '',
        '  # src -> src/win_repo',
        '  "src/win_repo": {',
        '    "url": "{repo5_var}",',
        '    "condition": \'checkout_win\',',
        '  },',
        '',
        '}',
        '',
        'hooks = [',
        '  # src',
        '  {',
        '    "pattern": ".",',
        '    "condition": \'True\',',
        '    "cwd": ".",',
        '    "action": [',
        '        "python",',
        '        "-c",',
        '        "open(\'src/git_hooked1\', \'w\')'
            '.write(\'{hook1_contents}\')",',
        '    ]',
        '  },',
        '',
        '  # src',
        '  {',
        '    "pattern": "nonexistent",',
        '    "cwd": ".",',
        '    "action": [',
        '        "python",',
        '        "-c",',
        '        "open(\'src/git_hooked2\', \'w\').write(\'git_hooked2\')",',
        '    ]',
        '  },',
        '',
        '  # src',
        '  {',
        '    "pattern": ".",',
        '    "condition": \'checkout_mac\',',
        '    "cwd": ".",',
        '    "action": [',
        '        "python",',
        '        "-c",',
        '        "open(\'src/git_hooked_mac\', \'w\').write('
                           '\'git_hooked_mac\')",',
        '    ]',
        '  },',
        '',
        ']',
        '',
        'vars = {',
        '  # src',
        '  "DummyVariable": \'repo\',',
        '',
        '  # src',
        '  "cond_var": \'false_str_var and true_var\',',
        '',
        '  # src',
        '  "false_str_var": \'False\',',
        '',
        '  # src',
        '  "false_var": False,',
        '',
        '  # src',
        '  "git_base": \'' + self.git_base + '\',',
        '',
        '  # src',
        '  "hook1_contents": \'git_hooked1\',',
        '',
        '  # src -> src/repo2',
        '  "repo2_false_var": \'False\',',
        '',
        '  # src',
        '  "repo5_var": \'/repo_5\',',
        '',
        '  # src',
        '  "str_var": \'abc\',',
        '',
        '  # src',
        '  "true_str_var": \'True\',',
        '',
        '  # src [custom_var override]',
        '  "true_var": \'False\',',
        '',
        '}',
        '',
        '# ' + self.git_base + 'repo_2@%s, DEPS' % (
                 self.githash('repo_2', 1)[:7]),
        '# ' + self.git_base + 'repo_8, DEPS'
    ], deps_contents.splitlines())

  def testFlattenPinAllDeps(self):
    if not self.enabled:
      return

    output_deps = os.path.join(self.root_dir, 'DEPS.flattened')
    self.assertFalse(os.path.exists(output_deps))

    self.gclient(['config', self.git_base + 'repo_6', '--name', 'src'])
    self.gclient(['sync', '--process-all-deps'])
    self.gclient(['flatten', '-v', '-v', '-v', '--output-deps', output_deps,
                  '--pin-all-deps'])

    with open(output_deps) as f:
      deps_contents = f.read()

    self.maxDiff = None  # pylint: disable=attribute-defined-outside-init
    self.assertEqual([
        'gclient_gn_args_file = "src/repo2/gclient.args"',
        'gclient_gn_args = [\'false_var\', \'false_str_var\', \'true_var\', '
            '\'true_str_var\', \'str_var\', \'cond_var\']',
        'allowed_hosts = [',
        '  "' + self.git_base + '",',
        ']',
        '',
        'deps = {',
        '  # src -> src/repo2 -> foo/bar',
        '  "foo/bar": {',
        '    "url": "/repo_3@%s",' % (self.githash('repo_3', 2)),
        '    "condition": \'(repo2_false_var) and (true_str_var)\',',
        '  },',
        '',
        '  # src',
        '  "src": {',
        '    "url": "' + self.git_base + 'repo_6@%s",' % (
                 self.githash('repo_6', 1)),
        '  },',
        '',
        '  # src -> src/mac_repo',
        '  "src/mac_repo": {',
        '    "url": "{repo5_var}@%s",' % (self.githash('repo_5', 3)),
        '    "condition": \'checkout_mac\',',
        '  },',
        '',
        '  # src -> src/repo8 -> src/recursed_os_repo',
        '  "src/recursed_os_repo": {',
        '    "url": "/repo_5@%s",' % (self.githash('repo_5', 3)),
        '    "condition": \'(checkout_linux) or (checkout_mac)\',',
        '  },',
        '',
        '  # src -> src/repo2',
        '  "src/repo2": {',
        '    "url": "{git_base}repo_2@%s",' % (
                 self.githash('repo_2', 1)),
        '    "condition": \'true_str_var\',',
        '  },',
        '',
        '  # src -> src/repo4',
        '  "src/repo4": {',
        '    "url": "/repo_4@%s",' % (self.githash('repo_4', 2)),
        '    "condition": \'False\',',
        '  },',
        '',
        '  # src -> src/repo8',
        '  "src/repo8": {',
        '    "url": "/repo_8@%s",' % (self.githash('repo_8', 1)),
        '  },',
        '',
        '  # src -> src/unix_repo',
        '  "src/unix_repo": {',
        '    "url": "{repo5_var}@%s",' % (self.githash('repo_5', 3)),
        '    "condition": \'checkout_linux\',',
        '  },',
        '',
        '  # src -> src/win_repo',
        '  "src/win_repo": {',
        '    "url": "{repo5_var}@%s",' % (self.githash('repo_5', 3)),
        '    "condition": \'checkout_win\',',
        '  },',
        '',
        '}',
        '',
        'hooks = [',
        '  # src',
        '  {',
        '    "pattern": ".",',
        '    "condition": \'True\',',
        '    "cwd": ".",',
        '    "action": [',
        '        "python",',
        '        "-c",',
        '        "open(\'src/git_hooked1\', \'w\')'
            '.write(\'{hook1_contents}\')",',
        '    ]',
        '  },',
        '',
        '  # src',
        '  {',
        '    "pattern": "nonexistent",',
        '    "cwd": ".",',
        '    "action": [',
        '        "python",',
        '        "-c",',
        '        "open(\'src/git_hooked2\', \'w\').write(\'git_hooked2\')",',
        '    ]',
        '  },',
        '',
        '  # src',
        '  {',
        '    "pattern": ".",',
        '    "condition": \'checkout_mac\',',
        '    "cwd": ".",',
        '    "action": [',
        '        "python",',
        '        "-c",',
        '        "open(\'src/git_hooked_mac\', \'w\').write('
                           '\'git_hooked_mac\')",',
        '    ]',
        '  },',
        '',
        ']',
        '',
        'vars = {',
        '  # src',
        '  "DummyVariable": \'repo\',',
        '',
        '  # src',
        '  "cond_var": \'false_str_var and true_var\',',
        '',
        '  # src',
        '  "false_str_var": \'False\',',
        '',
        '  # src',
        '  "false_var": False,',
        '',
        '  # src',
        '  "git_base": \'' + self.git_base + '\',',
        '',
        '  # src',
        '  "hook1_contents": \'git_hooked1\',',
        '',
        '  # src -> src/repo2',
        '  "repo2_false_var": \'False\',',
        '',
        '  # src',
        '  "repo5_var": \'/repo_5\',',
        '',
        '  # src',
        '  "str_var": \'abc\',',
        '',
        '  # src',
        '  "true_str_var": \'True\',',
        '',
        '  # src',
        '  "true_var": True,',
        '',
        '}',
        '',
        '# ' + self.git_base + 'repo_2@%s, DEPS' % (
            self.githash('repo_2', 1)),
        '# ' + self.git_base + 'repo_8@%s, DEPS' % (
            self.githash('repo_8', 1)),
    ], deps_contents.splitlines())

  def testFlattenRecursedeps(self):
    if not self.enabled:
      return

    output_deps = os.path.join(self.root_dir, 'DEPS.flattened')
    self.assertFalse(os.path.exists(output_deps))

    output_deps_files = os.path.join(self.root_dir, 'DEPS.files')
    self.assertFalse(os.path.exists(output_deps_files))

    self.gclient(['config', self.git_base + 'repo_10', '--name', 'src'])
    self.gclient(['sync', '--process-all-deps'])
    self.gclient(['flatten', '-v', '-v', '-v',
                  '--output-deps', output_deps,
                  '--output-deps-files', output_deps_files])

    with open(output_deps) as f:
      deps_contents = f.read()

    self.maxDiff = None
    self.assertEqual([
        'gclient_gn_args_file = "src/repo2/gclient.args"',
        "gclient_gn_args = ['str_var']",
        'deps = {',
        '  # src',
        '  "src": {',
        '    "url": "' + self.git_base + 'repo_10",',
        '  },',
        '',
        '  # src -> src/repo9 -> src/repo8 -> src/recursed_os_repo',
        '  "src/recursed_os_repo": {',
        '    "url": "/repo_5",',
        '    "condition": \'(checkout_linux) or (checkout_mac)\',',
        '  },',
        '',
        '  # src -> src/repo11',
        '  "src/repo11": {',
        '    "url": "/repo_11",',
        '    "condition": \'(checkout_ios) or (checkout_mac)\',',
        '  },',
        '',
        '  # src -> src/repo11 -> src/repo12',
        '  "src/repo12": {',
        '    "url": "/repo_12",',
        '    "condition": \'(checkout_ios) or (checkout_mac)\',',
        '  },',
        '',
        '  # src -> src/repo9 -> src/repo4',
        '  "src/repo4": {',
        '    "url": "/repo_4",',
        '    "condition": \'checkout_android\',',
        '  },',
        '',
        '  # src -> src/repo6',
        '  "src/repo6": {',
        '    "url": "/repo_6",',
        '  },',
        '',
        '  # src -> src/repo9 -> src/repo7',
        '  "src/repo7": {',
        '    "url": "/repo_7",',
        '  },',
        '',
        '  # src -> src/repo9 -> src/repo8',
        '  "src/repo8": {',
        '    "url": "/repo_8",',
        '  },',
        '',
        '  # src -> src/repo9',
        '  "src/repo9": {',
        '    "url": "/repo_9",',
        '  },',
        '',
        '}',
        '',
        'vars = {',
        '  # src -> src/repo9',
        '  "str_var": \'xyz\',',
        '',
        '}',
        '',
        '# ' + self.git_base + 'repo_11, DEPS',
        '# ' + self.git_base + 'repo_8, DEPS',
        '# ' + self.git_base + 'repo_9, DEPS',
    ], deps_contents.splitlines())

    with open(output_deps_files) as f:
      deps_files_contents = json.load(f)

    self.assertEqual([
      {'url': self.git_base + 'repo_11', 'deps_file': 'DEPS',
       'hierarchy': [['src', self.git_base + 'repo_10'],
                     ['src/repo11', self.git_base + 'repo_11']]},
      {'url': self.git_base + 'repo_8', 'deps_file': 'DEPS',
       'hierarchy': [['src', self.git_base + 'repo_10'],
                     ['src/repo9', self.git_base + 'repo_9'],
                     ['src/repo8', self.git_base + 'repo_8']]},
      {'url': self.git_base + 'repo_9', 'deps_file': 'DEPS',
       'hierarchy': [['src', self.git_base + 'repo_10'],
                     ['src/repo9', self.git_base + 'repo_9']]},
    ], deps_files_contents)

  def testFlattenCipd(self):
    if not self.enabled:
      return

    output_deps = os.path.join(self.root_dir, 'DEPS.flattened')
    self.assertFalse(os.path.exists(output_deps))

    self.gclient(['config', self.git_base + 'repo_14', '--name', 'src'])
    self.gclient(['sync'])
    self.gclient(['flatten', '-v', '-v', '-v', '--output-deps', output_deps])

    with open(output_deps) as f:
      deps_contents = f.read()

    self.maxDiff = None  # pylint: disable=attribute-defined-outside-init
    self.assertEqual([
        'deps = {',
        '  # src',
        '  "src": {',
        '    "url": "' + self.git_base + 'repo_14",',
        '  },',
        '',
        '  # src -> src/cipd_dep:package0',
        '  "src/cipd_dep": {',
        '    "packages": [',
        '      {',
        '        "package": "package0",',
        '        "version": "0.1",',
        '      },',
        '    ],',
        '    "dep_type": "cipd",',
        '  },',
        '',
        '}',
        ''
    ], deps_contents.splitlines())


class GClientSmokeGITMutates(GClientSmokeBase):
  """testRevertAndStatus mutates the git repo so move it to its own suite."""
  def setUp(self):
    super(GClientSmokeGITMutates, self).setUp()
    self.enabled = self.FAKE_REPOS.set_up_git()

  def testRevertAndStatus(self):
    if not self.enabled:
      return

    # Commit new change to repo to make repo_2's hash use a custom_var.
    cur_deps = self.FAKE_REPOS.git_hashes['repo_1'][-1][1]['DEPS']
    repo_2_hash = self.FAKE_REPOS.git_hashes['repo_2'][1][0][:7]
    new_deps = cur_deps.replace('repo_2@%s\'' % repo_2_hash,
                                'repo_2@\' + Var(\'r2hash\')')
    new_deps = 'vars = {\'r2hash\': \'%s\'}\n%s' % (repo_2_hash, new_deps)
    self.FAKE_REPOS._commit_git('repo_1', {  # pylint: disable=protected-access
      'DEPS': new_deps,
      'origin': 'git/repo_1@3\n',
    })

    config_template = (
"""solutions = [{
  "name"        : "src",
  "url"         : "%(git_base)srepo_1",
  "deps_file"   : "DEPS",
  "managed"     : True,
  "custom_vars" : %(custom_vars)s,
}]""")

    self.gclient(['config', '--spec', config_template % {
      'git_base': self.git_base,
      'custom_vars': {}
    }])

    # Tested in testSync.
    self.gclient(['sync', '--deps', 'mac'])
    write(join(self.root_dir, 'src', 'repo2', 'hi'), 'Hey!')

    out = self.parseGclient(['status', '--deps', 'mac', '--jobs', '1'], [])
    # TODO(maruel): http://crosbug.com/3584 It should output the unversioned
    # files.
    self.assertEquals(0, len(out))

    # Revert implies --force implies running hooks without looking at pattern
    # matching. For each expected path, 'git reset' and 'git clean' are run, so
    # there should be two results for each. The last two results should reflect
    # writing git_hooked1 and git_hooked2. There's only one result for the third
    # because it is clean and has no output for 'git clean'.
    out = self.parseGclient(['revert', '--deps', 'mac', '--jobs', '1'],
                            ['running', 'running'])
    self.assertEquals(2, len(out))
    tree = self.mangle_git_tree(('repo_1@3', 'src'),
                                ('repo_2@1', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    self.assertTree(tree)

    # Make a new commit object in the origin repo, to force reset to fetch.
    self.FAKE_REPOS._commit_git('repo_2', {  # pylint: disable=protected-access
      'origin': 'git/repo_2@3\n',
    })

    self.gclient(['config', '--spec', config_template % {
      'git_base': self.git_base,
      'custom_vars': {'r2hash': self.FAKE_REPOS.git_hashes['repo_2'][-1][0] }
    }])
    out = self.parseGclient(['revert', '--deps', 'mac', '--jobs', '1'],
                            ['running', 'running'])
    self.assertEquals(2, len(out))
    tree = self.mangle_git_tree(('repo_1@3', 'src'),
                                ('repo_2@3', 'src/repo2'),
                                ('repo_3@2', 'src/repo2/repo_renamed'))
    tree['src/git_hooked1'] = 'git_hooked1'
    tree['src/git_hooked2'] = 'git_hooked2'
    self.assertTree(tree)

    results = self.gclient(['status', '--deps', 'mac', '--jobs', '1'])
    out = results[0].splitlines(False)
    # TODO(maruel): http://crosbug.com/3584 It should output the unversioned
    # files.
    self.assertEquals(0, len(out))

  def testSyncNoHistory(self):
    if not self.enabled:
      return
    # Create an extra commit in repo_2 and point DEPS to its hash.
    cur_deps = self.FAKE_REPOS.git_hashes['repo_1'][-1][1]['DEPS']
    repo_2_hash_old = self.FAKE_REPOS.git_hashes['repo_2'][1][0][:7]
    self.FAKE_REPOS._commit_git('repo_2', {  # pylint: disable=protected-access
      'last_file': 'file created in last commit',
    })
    repo_2_hash_new = self.FAKE_REPOS.git_hashes['repo_2'][-1][0]
    new_deps = cur_deps.replace(repo_2_hash_old, repo_2_hash_new)
    self.assertNotEqual(new_deps, cur_deps)
    self.FAKE_REPOS._commit_git('repo_1', {  # pylint: disable=protected-access
      'DEPS': new_deps,
      'origin': 'git/repo_1@4\n',
    })

    config_template = (
"""solutions = [{
"name"        : "src",
"url"         : "%(git_base)srepo_1",
"deps_file"   : "DEPS",
"managed"     : True,
}]""")

    self.gclient(['config', '--spec', config_template % {
      'git_base': self.git_base
    }])

    self.gclient(['sync', '--no-history', '--deps', 'mac'])
    repo2_root = join(self.root_dir, 'src', 'repo2')

    # Check that repo_2 is actually shallow and its log has only one entry.
    rev_lists = subprocess2.check_output(['git', 'rev-list', 'HEAD'],
                                         cwd=repo2_root)
    self.assertEquals(repo_2_hash_new, rev_lists.strip('\r\n'))

    # Check that we have actually checked out the right commit.
    self.assertTrue(os.path.exists(join(repo2_root, 'last_file')))


class SkiaDEPSTransitionSmokeTest(GClientSmokeBase):
  """Simulate the behavior of bisect bots as they transition across the Skia
  DEPS change."""

  FAKE_REPOS_CLASS = fake_repos.FakeRepoSkiaDEPS

  def setUp(self):
    super(SkiaDEPSTransitionSmokeTest, self).setUp()
    self.enabled = self.FAKE_REPOS.set_up_git()

  def testSkiaDEPSChangeGit(self):
    if not self.enabled:
      return

    # Create an initial checkout:
    # - Single checkout at the root.
    # - Multiple checkouts in a shared subdirectory.
    self.gclient(['config', '--spec',
        'solutions=['
        '{"name": "src",'
        ' "url": "' + self.git_base + 'repo_2",'
        '}]'])

    checkout_path = os.path.join(self.root_dir, 'src')
    skia = os.path.join(checkout_path, 'third_party', 'skia')
    skia_gyp = os.path.join(skia, 'gyp')
    skia_include = os.path.join(skia, 'include')
    skia_src = os.path.join(skia, 'src')

    gyp_git_url = self.git_base + 'repo_3'
    include_git_url = self.git_base + 'repo_4'
    src_git_url = self.git_base + 'repo_5'
    skia_git_url = self.FAKE_REPOS.git_base + 'repo_1'

    pre_hash = self.githash('repo_2', 1)
    post_hash = self.githash('repo_2', 2)

    # Initial sync. Verify that we get the expected checkout.
    res = self.gclient(['sync', '--deps', 'mac', '--revision',
                        'src@%s' % pre_hash])
    self.assertEqual(res[2], 0, 'Initial sync failed.')
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia_gyp), gyp_git_url)
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia_include), include_git_url)
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia_src), src_git_url)

    # Verify that the sync succeeds. Verify that we have the  expected merged
    # checkout.
    res = self.gclient(['sync', '--deps', 'mac', '--revision',
                        'src@%s' % post_hash])
    self.assertEqual(res[2], 0, 'DEPS change sync failed.')
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia), skia_git_url)

    # Sync again. Verify that we still have the expected merged checkout.
    res = self.gclient(['sync', '--deps', 'mac', '--revision',
                        'src@%s' % post_hash])
    self.assertEqual(res[2], 0, 'Subsequent sync failed.')
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia), skia_git_url)

    # Sync back to the original DEPS. Verify that we get the original structure.
    res = self.gclient(['sync', '--deps', 'mac', '--revision',
                        'src@%s' % pre_hash])
    self.assertEqual(res[2], 0, 'Reverse sync failed.')
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia_gyp), gyp_git_url)
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia_include), include_git_url)
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia_src), src_git_url)

    # Sync again. Verify that we still have the original structure.
    res = self.gclient(['sync', '--deps', 'mac', '--revision',
                        'src@%s' % pre_hash])
    self.assertEqual(res[2], 0, 'Subsequent sync #2 failed.')
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia_gyp), gyp_git_url)
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia_include), include_git_url)
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             skia_src), src_git_url)


class BlinkDEPSTransitionSmokeTest(GClientSmokeBase):
  """Simulate the behavior of bisect bots as they transition across the Blink
  DEPS change."""

  FAKE_REPOS_CLASS = fake_repos.FakeRepoBlinkDEPS

  def setUp(self):
    super(BlinkDEPSTransitionSmokeTest, self).setUp()
    self.enabled = self.FAKE_REPOS.set_up_git()
    self.checkout_path = os.path.join(self.root_dir, 'src')
    self.blink = os.path.join(self.checkout_path, 'third_party', 'WebKit')
    self.blink_git_url = self.FAKE_REPOS.git_base + 'repo_2'
    self.pre_merge_sha = self.githash('repo_1', 1)
    self.post_merge_sha = self.githash('repo_1', 2)

  def CheckStatusPreMergePoint(self):
    self.assertEqual(gclient_scm.GIT.Capture(['config', 'remote.origin.url'],
                                             self.blink), self.blink_git_url)
    self.assertTrue(os.path.exists(join(self.blink, '.git')))
    self.assertTrue(os.path.exists(join(self.blink, 'OWNERS')))
    with open(join(self.blink, 'OWNERS')) as f:
      owners_content = f.read()
      self.assertEqual('OWNERS-pre', owners_content, 'OWNERS not updated')
    self.assertTrue(os.path.exists(join(self.blink, 'Source', 'exists_always')))
    self.assertTrue(os.path.exists(
        join(self.blink, 'Source', 'exists_before_but_not_after')))
    self.assertFalse(os.path.exists(
        join(self.blink, 'Source', 'exists_after_but_not_before')))

  def CheckStatusPostMergePoint(self):
    # Check that the contents still exists
    self.assertTrue(os.path.exists(join(self.blink, 'OWNERS')))
    with open(join(self.blink, 'OWNERS')) as f:
      owners_content = f.read()
      self.assertEqual('OWNERS-post', owners_content, 'OWNERS not updated')
    self.assertTrue(os.path.exists(join(self.blink, 'Source', 'exists_always')))
    # Check that file removed between the branch point are actually deleted.
    self.assertTrue(os.path.exists(
        join(self.blink, 'Source', 'exists_after_but_not_before')))
    self.assertFalse(os.path.exists(
        join(self.blink, 'Source', 'exists_before_but_not_after')))
    # But not the .git folder
    self.assertFalse(os.path.exists(join(self.blink, '.git')))

  @unittest.skip('flaky')
  def testBlinkDEPSChangeUsingGclient(self):
    """Checks that {src,blink} repos are consistent when syncing going back and
    forth using gclient sync src@revision."""
    if not self.enabled:
      return

    self.gclient(['config', '--spec',
        'solutions=['
        '{"name": "src",'
        ' "url": "' + self.git_base + 'repo_1",'
        '}]'])

    # Go back and forth two times.
    for _ in xrange(2):
      res = self.gclient(['sync', '--jobs', '1',
                          '--revision', 'src@%s' % self.pre_merge_sha])
      self.assertEqual(res[2], 0, 'DEPS change sync failed.')
      self.CheckStatusPreMergePoint()

      res = self.gclient(['sync', '--jobs', '1',
                          '--revision', 'src@%s' % self.post_merge_sha])
      self.assertEqual(res[2], 0, 'DEPS change sync failed.')
      self.CheckStatusPostMergePoint()


  @unittest.skip('flaky')
  def testBlinkDEPSChangeUsingGit(self):
    """Like testBlinkDEPSChangeUsingGclient, but move the main project using
    directly git and not gclient sync."""
    if not self.enabled:
      return

    self.gclient(['config', '--spec',
        'solutions=['
        '{"name": "src",'
        ' "url": "' + self.git_base + 'repo_1",'
        ' "managed": False,'
        '}]'])

    # Perform an initial sync to bootstrap the repo.
    res = self.gclient(['sync', '--jobs', '1'])
    self.assertEqual(res[2], 0, 'Initial gclient sync failed.')

    # Go back and forth two times.
    for _ in xrange(2):
      subprocess2.check_call(['git', 'checkout', '-q', self.pre_merge_sha],
                             cwd=self.checkout_path)
      res = self.gclient(['sync', '--jobs', '1'])
      self.assertEqual(res[2], 0, 'gclient sync failed.')
      self.CheckStatusPreMergePoint()

      subprocess2.check_call(['git', 'checkout', '-q', self.post_merge_sha],
                             cwd=self.checkout_path)
      res = self.gclient(['sync', '--jobs', '1'])
      self.assertEqual(res[2], 0, 'DEPS change sync failed.')
      self.CheckStatusPostMergePoint()


  @unittest.skip('flaky')
  def testBlinkLocalBranchesArePreserved(self):
    """Checks that the state of local git branches are effectively preserved
    when going back and forth."""
    if not self.enabled:
      return

    self.gclient(['config', '--spec',
        'solutions=['
        '{"name": "src",'
        ' "url": "' + self.git_base + 'repo_1",'
        '}]'])

    # Initialize to pre-merge point.
    self.gclient(['sync', '--revision', 'src@%s' % self.pre_merge_sha])
    self.CheckStatusPreMergePoint()

    # Create a branch named "foo".
    subprocess2.check_call(['git', 'checkout', '-qB', 'foo'],
                           cwd=self.blink)

    # Cross the pre-merge point.
    self.gclient(['sync', '--revision', 'src@%s' % self.post_merge_sha])
    self.CheckStatusPostMergePoint()

    # Go backwards and check that we still have the foo branch.
    self.gclient(['sync', '--revision', 'src@%s' % self.pre_merge_sha])
    self.CheckStatusPreMergePoint()
    subprocess2.check_call(
        ['git', 'show-ref', '-q', '--verify', 'refs/heads/foo'], cwd=self.blink)


if __name__ == '__main__':
  if '-v' in sys.argv:
    logging.basicConfig(level=logging.DEBUG)

  if '-c' in sys.argv:
    COVERAGE = True
    sys.argv.remove('-c')
    if os.path.exists('.coverage'):
      os.remove('.coverage')
    os.environ['COVERAGE_FILE'] = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        '.coverage')
  unittest.main()
