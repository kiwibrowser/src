#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for gclient.py.

See gclient_smoketest.py for integration tests.
"""

import Queue
import copy
import logging
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import gclient
import gclient_utils
import gclient_scm
from testing_support import trial_dir


def write(filename, content):
  """Writes the content of a file and create the directories as needed."""
  filename = os.path.abspath(filename)
  dirname = os.path.dirname(filename)
  if not os.path.isdir(dirname):
    os.makedirs(dirname)
  with open(filename, 'w') as f:
    f.write(content)


class SCMMock(object):
  unit_test = None
  def __init__(self, parsed_url, root_dir, name, out_fh=None, out_cb=None,
               print_outbuf=False):
    self.unit_test.assertTrue(
        parsed_url.startswith('svn://example.com/'), parsed_url)
    self.unit_test.assertTrue(
        root_dir.startswith(self.unit_test.root_dir), root_dir)
    self.name = name
    self.url = parsed_url

  def RunCommand(self, command, options, args, file_list):
    self.unit_test.assertEquals('None', command)
    self.unit_test.processed.put((self.name, self.url))

  # pylint: disable=no-self-use
  def DoesRemoteURLMatch(self, _):
    return True

  def GetActualRemoteURL(self, _):
    return self.url


class GclientTest(trial_dir.TestCase):
  def setUp(self):
    super(GclientTest, self).setUp()
    self.processed = Queue.Queue()
    self.previous_dir = os.getcwd()
    os.chdir(self.root_dir)
    # Manual mocks.
    self._old_createscm = gclient.gclient_scm.GitWrapper
    gclient.gclient_scm.GitWrapper = SCMMock
    SCMMock.unit_test = self
    self._old_sys_stdout = sys.stdout
    sys.stdout = gclient.gclient_utils.MakeFileAutoFlush(sys.stdout)
    sys.stdout = gclient.gclient_utils.MakeFileAnnotated(sys.stdout)

  def tearDown(self):
    self.assertEquals([], self._get_processed())
    gclient.gclient_scm.GitWrapper = self._old_createscm
    sys.stdout = self._old_sys_stdout
    os.chdir(self.previous_dir)
    super(GclientTest, self).tearDown()

  def testDependencies(self):
    self._dependencies('1')

  def testDependenciesJobs(self):
    self._dependencies('1000')

  def _dependencies(self, jobs):
    """Verifies that dependencies are processed in the right order.

    e.g. if there is a dependency 'src' and another 'src/third_party/bar', that
    bar isn't fetched until 'src' is done.

    Args:
      |jobs| is the number of parallel jobs simulated.
    """
    parser = gclient.OptionParser()
    options, args = parser.parse_args(['--jobs', jobs])
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo" },\n'
        '  { "name": "bar", "url": "svn://example.com/bar" },\n'
        '  { "name": "bar/empty", "url": "svn://example.com/bar_empty" },\n'
        ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "foo/dir1": "/dir1",\n'
        # This one will depend on dir1/dir2 in bar.
        '  "foo/dir1/dir2/dir3": "/dir1/dir2/dir3",\n'
        '  "foo/dir1/dir2/dir3/dir4": "/dir1/dir2/dir3/dir4",\n'
        '}')
    write(
        os.path.join('bar', 'DEPS'),
        'deps = {\n'
        # There is two foo/dir1/dir2. This one is fetched as bar/dir1/dir2.
        '  "foo/dir1/dir2": "/dir1/dir2",\n'
        '}')
    write(
        os.path.join('bar/empty', 'DEPS'),
        'deps = {\n'
        '}')

    obj = gclient.GClient.LoadCurrentConfig(options)
    self._check_requirements(obj.dependencies[0], {})
    self._check_requirements(obj.dependencies[1], {})
    obj.RunOnDeps('None', args)
    actual = self._get_processed()
    first_3 = [
        ('bar', 'svn://example.com/bar'),
        ('bar/empty', 'svn://example.com/bar_empty'),
        ('foo', 'svn://example.com/foo'),
    ]
    if jobs != 1:
      # We don't care of the ordering of these items except that bar must be
      # before bar/empty.
      self.assertTrue(
          actual.index(('bar', 'svn://example.com/bar')) <
          actual.index(('bar/empty', 'svn://example.com/bar_empty')))
      self.assertEquals(first_3, sorted(actual[0:3]))
    else:
      self.assertEquals(first_3, actual[0:3])
    self.assertEquals(
        [
          ('foo/dir1', 'svn://example.com/dir1'),
          ('foo/dir1/dir2', 'svn://example.com/dir1/dir2'),
          ('foo/dir1/dir2/dir3', 'svn://example.com/dir1/dir2/dir3'),
          ('foo/dir1/dir2/dir3/dir4',
           'svn://example.com/dir1/dir2/dir3/dir4'),
        ],
        actual[3:])

    self.assertEquals(3, len(obj.dependencies))
    self.assertEquals('foo', obj.dependencies[0].name)
    self.assertEquals('bar', obj.dependencies[1].name)
    self.assertEquals('bar/empty', obj.dependencies[2].name)
    self._check_requirements(
        obj.dependencies[0],
        {
          'foo/dir1': ['bar', 'bar/empty', 'foo'],
          'foo/dir1/dir2/dir3':
              ['bar', 'bar/empty', 'foo', 'foo/dir1', 'foo/dir1/dir2'],
          'foo/dir1/dir2/dir3/dir4':
              [ 'bar', 'bar/empty', 'foo', 'foo/dir1', 'foo/dir1/dir2',
                'foo/dir1/dir2/dir3'],
        })
    self._check_requirements(
        obj.dependencies[1],
        {
          'foo/dir1/dir2': ['bar', 'bar/empty', 'foo', 'foo/dir1'],
        })
    self._check_requirements(
        obj.dependencies[2],
        {})
    self._check_requirements(
        obj,
        {
          'foo': [],
          'bar': [],
          'bar/empty': ['bar'],
        })

  def _check_requirements(self, solution, expected):
    for dependency in solution.dependencies:
      e = expected.pop(dependency.name)
      a = sorted(dependency.requirements)
      self.assertEquals(e, a, (dependency.name, e, a))
    self.assertEquals({}, expected)

  def _get_processed(self):
    """Retrieves the item in the order they were processed."""
    items = []
    try:
      while True:
        items.append(self.processed.get_nowait())
    except Queue.Empty:
      pass
    return items

  def testAutofix(self):
    # Invalid urls causes pain when specifying requirements. Make sure it's
    # auto-fixed.
    url = 'proto://host/path/@revision'
    d = gclient.Dependency(
        None, 'name', url, url, None, None, None,
        None, '', True, False, None, True)
    self.assertEquals('proto://host/path@revision', d.url)

  def testStr(self):
    parser = gclient.OptionParser()
    options, _ = parser.parse_args([])
    obj = gclient.GClient('foo', options)
    obj.add_dependencies_and_close(
      [
        gclient.Dependency(
            obj, 'foo', 'svn://example.com/foo', 'svn://example.com/foo', None,
            None, None, None, 'DEPS', True, False, None, True),
        gclient.Dependency(
            obj, 'bar', 'svn://example.com/bar', 'svn://example.com/bar', None,
            None, None, None, 'DEPS', True, False, None, True),
      ],
      [])
    obj.dependencies[0].add_dependencies_and_close(
      [
        gclient.Dependency(
            obj.dependencies[0], 'foo/dir1', 'svn://example.com/foo/dir1',
            'svn://example.com/foo/dir1', None, None, None, None, 'DEPS', True,
            False, None, True),
      ],
      [])
    # TODO(ehmaldonado): Improve this test.
    # Make sure __str__() works fine.
    # pylint: disable=protected-access
    obj.dependencies[0]._file_list.append('foo')
    str_obj = str(obj)
    self.assertEquals(322, len(str_obj), '%d\n%s' % (len(str_obj), str_obj))

  def testHooks(self):
    topdir = self.root_dir
    gclient_fn = os.path.join(topdir, '.gclient')
    fh = open(gclient_fn, 'w')
    print >> fh, 'solutions = [{"name":"top","url":"svn://example.com/top"}]'
    fh.close()
    subdir_fn = os.path.join(topdir, 'top')
    os.mkdir(subdir_fn)
    deps_fn = os.path.join(subdir_fn, 'DEPS')
    fh = open(deps_fn, 'w')
    hooks = [{'pattern':'.', 'action':['cmd1', 'arg1', 'arg2']}]
    print >> fh, 'hooks = %s' % repr(hooks)
    fh.close()

    fh = open(os.path.join(subdir_fn, 'fake.txt'), 'w')
    print >> fh, 'bogus content'
    fh.close()

    os.chdir(topdir)

    parser = gclient.OptionParser()
    options, _ = parser.parse_args([])
    options.force = True
    client = gclient.GClient.LoadCurrentConfig(options)
    work_queue = gclient_utils.ExecutionQueue(options.jobs, None, False)
    for s in client.dependencies:
      work_queue.enqueue(s)
    work_queue.flush({}, None, [], options=options, patch_refs={})
    self.assertEqual(
        [h.action for h in client.GetHooks(options)],
        [tuple(x['action']) for x in hooks])

  def testCustomHooks(self):
    topdir = self.root_dir
    gclient_fn = os.path.join(topdir, '.gclient')
    fh = open(gclient_fn, 'w')
    extra_hooks = [{'name': 'append', 'pattern':'.', 'action':['supercmd']}]
    print >> fh, ('solutions = [{"name":"top","url":"svn://example.com/top",'
        '"custom_hooks": %s},' ) % repr(extra_hooks + [{'name': 'skip'}])
    print >> fh, '{"name":"bottom","url":"svn://example.com/bottom"}]'
    fh.close()
    subdir_fn = os.path.join(topdir, 'top')
    os.mkdir(subdir_fn)
    deps_fn = os.path.join(subdir_fn, 'DEPS')
    fh = open(deps_fn, 'w')
    hooks = [{'pattern':'.', 'action':['cmd1', 'arg1', 'arg2']}]
    hooks.append({'pattern':'.', 'action':['cmd2', 'arg1', 'arg2']})
    skip_hooks = [
        {'name': 'skip', 'pattern':'.', 'action':['cmd3', 'arg1', 'arg2']}]
    skip_hooks.append(
        {'name': 'skip', 'pattern':'.', 'action':['cmd4', 'arg1', 'arg2']})
    print >> fh, 'hooks = %s' % repr(hooks + skip_hooks)
    fh.close()

    # Make sure the custom hooks for that project don't affect the next one.
    subdir_fn = os.path.join(topdir, 'bottom')
    os.mkdir(subdir_fn)
    deps_fn = os.path.join(subdir_fn, 'DEPS')
    fh = open(deps_fn, 'w')
    sub_hooks = [{'pattern':'.', 'action':['response1', 'yes1', 'yes2']}]
    sub_hooks.append(
        {'name': 'skip', 'pattern':'.', 'action':['response2', 'yes', 'sir']})
    print >> fh, 'hooks = %s' % repr(sub_hooks)
    fh.close()

    fh = open(os.path.join(subdir_fn, 'fake.txt'), 'w')
    print >> fh, 'bogus content'
    fh.close()

    os.chdir(topdir)

    parser = gclient.OptionParser()
    options, _ = parser.parse_args([])
    options.force = True
    client = gclient.GClient.LoadCurrentConfig(options)
    work_queue = gclient_utils.ExecutionQueue(options.jobs, None, False)
    for s in client.dependencies:
      work_queue.enqueue(s)
    work_queue.flush({}, None, [], options=options, patch_refs={})
    self.assertEqual(
        [h.action for h in client.GetHooks(options)],
        [tuple(x['action']) for x in hooks + extra_hooks + sub_hooks])

  def testTargetOS(self):
    """Verifies that specifying a target_os pulls in all relevant dependencies.

    The target_os variable allows specifying the name of an additional OS which
    should be considered when selecting dependencies from a DEPS' deps_os. The
    value will be appended to the _enforced_os tuple.
    """

    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo",\n'
        '    "url": "svn://example.com/foo",\n'
        '  }]\n'
        'target_os = ["baz"]')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "foo/dir1": "/dir1",'
        '}\n'
        'deps_os = {\n'
        '  "unix": { "foo/dir2": "/dir2", },\n'
        '  "baz": { "foo/dir3": "/dir3", },\n'
        '}')

    parser = gclient.OptionParser()
    options, _ = parser.parse_args(['--jobs', '1'])
    options.deps_os = "unix"

    obj = gclient.GClient.LoadCurrentConfig(options)
    self.assertEqual(['baz', 'unix'], sorted(obj.enforced_os))

  def testTargetOsWithTargetOsOnly(self):
    """Verifies that specifying a target_os and target_os_only pulls in only
    the relevant dependencies.

    The target_os variable allows specifying the name of an additional OS which
    should be considered when selecting dependencies from a DEPS' deps_os. With
    target_os_only also set, the _enforced_os tuple will be set to only the
    target_os value.
    """

    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo",\n'
        '    "url": "svn://example.com/foo",\n'
        '  }]\n'
        'target_os = ["baz"]\n'
        'target_os_only = True')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "foo/dir1": "/dir1",'
        '}\n'
        'deps_os = {\n'
        '  "unix": { "foo/dir2": "/dir2", },\n'
        '  "baz": { "foo/dir3": "/dir3", },\n'
        '}')

    parser = gclient.OptionParser()
    options, _ = parser.parse_args(['--jobs', '1'])
    options.deps_os = "unix"

    obj = gclient.GClient.LoadCurrentConfig(options)
    self.assertEqual(['baz'], sorted(obj.enforced_os))

  def testTargetOsOnlyWithoutTargetOs(self):
    """Verifies that specifying a target_os_only without target_os_only raises
    an exception.
    """

    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo",\n'
        '    "url": "svn://example.com/foo",\n'
        '  }]\n'
        'target_os_only = True')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "foo/dir1": "/dir1",'
        '}\n'
        'deps_os = {\n'
        '  "unix": { "foo/dir2": "/dir2", },\n'
        '}')

    parser = gclient.OptionParser()
    options, _ = parser.parse_args(['--jobs', '1'])
    options.deps_os = "unix"

    exception_raised = False
    try:
      gclient.GClient.LoadCurrentConfig(options)
    except gclient_utils.Error:
      exception_raised = True
    self.assertTrue(exception_raised)

  def testTargetOsInDepsFile(self):
    """Verifies that specifying a target_os value in a DEPS file pulls in all
    relevant dependencies.

    The target_os variable in a DEPS file allows specifying the name of an
    additional OS which should be considered when selecting dependencies from a
    DEPS' deps_os. The value will be appended to the _enforced_os tuple.
    """

    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo",\n'
        '    "url": "svn://example.com/foo",\n'
        '  },\n'
        '  { "name": "bar",\n'
        '    "url": "svn://example.com/bar",\n'
        '  }]\n')
    write(
        os.path.join('foo', 'DEPS'),
        'target_os = ["baz"]\n')
    write(
        os.path.join('bar', 'DEPS'),
        '')

    parser = gclient.OptionParser()
    options, _ = parser.parse_args(['--jobs', '1'])
    options.deps_os = 'unix'

    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    self.assertEqual(['unix'], sorted(obj.enforced_os))
    self.assertEqual([('unix', 'baz'), ('unix',)],
                     [dep.target_os for dep in obj.dependencies])
    self.assertEqual([('foo', 'svn://example.com/foo'),
                      ('bar', 'svn://example.com/bar')],
                     self._get_processed())

  def testTargetOsForHooksInDepsFile(self):
    """Verifies that specifying a target_os value in a DEPS file runs the right
    entries in hooks_os.
    """

    write(
        'DEPS',
        'hooks = [\n'
        '  {\n'
        '    "name": "a",\n'
        '    "pattern": ".",\n'
        '    "action": [ "python", "do_a" ],\n'
        '  },\n'
        ']\n'
        '\n'
        'hooks_os = {\n'
        '  "blorp": ['
        '    {\n'
        '      "name": "b",\n'
        '      "pattern": ".",\n'
        '      "action": [ "python", "do_b" ],\n'
        '    },\n'
        '  ],\n'
        '}\n')

    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": ".",\n'
        '    "url": "svn://example.com/",\n'
        '  }]\n')
    # Test for an OS not in hooks_os.
    parser = gclient.OptionParser()
    options, args = parser.parse_args(['--jobs', '1'])
    options.deps_os = 'zippy'

    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', args)
    self.assertEqual(['zippy'], sorted(obj.enforced_os))
    all_hooks = obj.GetHooks(options)
    self.assertEquals(
        [('.', 'svn://example.com/'),],
        sorted(self._get_processed()))
    self.assertEquals([h.action for h in all_hooks],
                      [('python', 'do_a'),
                       ('python', 'do_b')])
    self.assertEquals([h.condition for h in all_hooks],
                      [None, 'checkout_blorp'])

  def testOverride(self):
    """Verifies expected behavior of URL overrides."""
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo",\n'
        '    "url": "svn://example.com/foo",\n'
        '    "custom_deps": {\n'
        '      "foo/bar": "svn://example.com/override",\n'
        '      "foo/skip2": None,\n'
        '      "foo/new": "svn://example.com/new",\n'
        '    },\n'
        '  },]\n')
    write(
        os.path.join('foo', 'DEPS'),
        'vars = {\n'
        '  "origin": "svn://example.com",\n'
        '}\n'
        'deps = {\n'
        '  "foo/skip": None,\n'
        '  "foo/bar": "{origin}/bar",\n'
        '  "foo/baz": "{origin}/baz",\n'
        '  "foo/skip2": "{origin}/skip2",\n'
        '  "foo/rel": "/rel",\n'
        '}')
    parser = gclient.OptionParser()
    options, _ = parser.parse_args(['--jobs', '1'])

    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])

    sol = obj.dependencies[0]
    self.assertEqual([
        ('foo', 'svn://example.com/foo'),
        ('foo/bar', 'svn://example.com/override'),
        ('foo/baz', 'svn://example.com/baz'),
        ('foo/new', 'svn://example.com/new'),
        ('foo/rel', 'svn://example.com/rel'),
    ], self._get_processed())

    self.assertEqual(6, len(sol.dependencies))
    self.assertEqual([
        ('foo/bar', 'svn://example.com/override'),
        ('foo/baz', 'svn://example.com/baz'),
        ('foo/new', 'svn://example.com/new'),
        ('foo/rel', 'svn://example.com/rel'),
        ('foo/skip', None),
        ('foo/skip2', None),
    ], [(dep.name, dep.url) for dep in sol.dependencies])

  def testDepsOsOverrideDepsInDepsFile(self):
    """Verifies that a 'deps_os' path cannot override a 'deps' path. Also
    see testUpdateWithOsDeps above.
    """

    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo",\n'
        '    "url": "svn://example.com/foo",\n'
        '  },]\n')
    write(
        os.path.join('foo', 'DEPS'),
        'target_os = ["baz"]\n'
        'deps = {\n'
        '  "foo/src": "/src",\n' # This path is to be overridden by similar path
                                 # in deps_os['unix'].
        '}\n'
        'deps_os = {\n'
        '  "unix": { "foo/unix": "/unix",'
        '            "foo/src": "/src_unix"},\n'
        '  "baz": { "foo/baz": "/baz",\n'
        '           "foo/src": None},\n'
        '  "jaz": { "foo/jaz": "/jaz", },\n'
        '}')

    parser = gclient.OptionParser()
    options, _ = parser.parse_args(['--jobs', '1'])
    options.deps_os = 'unix'

    obj = gclient.GClient.LoadCurrentConfig(options)
    with self.assertRaises(gclient_utils.Error):
      obj.RunOnDeps('None', [])
    self.assertEqual(['unix'], sorted(obj.enforced_os))
    self.assertEquals(
        [
          ('foo', 'svn://example.com/foo'),
          ],
        sorted(self._get_processed()))

  def testRecursionOverride(self):
    """Verifies gclient respects the |recursion| var syntax.

    We check several things here:
    - |recursion| = 3 sets recursion on the foo dep to exactly 3
      (we pull /fizz, but not /fuzz)
    - pulling foo/bar at recursion level 1 (in .gclient) is overriden by
      a later pull of foo/bar at recursion level 2 (in the dep tree)
    """
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo" },\n'
        '  { "name": "foo/bar", "url": "svn://example.com/bar" },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}\n'
        'recursion = 3')
    write(
        os.path.join('bar', 'DEPS'),
        'deps = {\n'
        '  "baz": "/baz",\n'
        '}')
    write(
        os.path.join('baz', 'DEPS'),
        'deps = {\n'
        '  "fizz": "/fizz",\n'
        '}')
    write(
        os.path.join('fizz', 'DEPS'),
        'deps = {\n'
        '  "fuzz": "/fuzz",\n'
        '}')

    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    self.assertEquals(
        [
          ('foo', 'svn://example.com/foo'),
          ('foo/bar', 'svn://example.com/bar'),
          ('bar', 'svn://example.com/bar'),
          ('baz', 'svn://example.com/baz'),
          ('fizz', 'svn://example.com/fizz'),
        ],
        self._get_processed())

  def testRecursedepsOverride(self):
    """Verifies gclient respects the |recursedeps| var syntax.

    This is what we mean to check here:
    - |recursedeps| = [...] on 2 levels means we pull exactly 3 deps
      (up to /fizz, but not /fuzz)
    - pulling foo/bar with no recursion (in .gclient) is overriden by
      a later pull of foo/bar with recursion (in the dep tree)
    - pulling foo/tar with no recursion (in .gclient) is no recursively
      pulled (taz is left out)
    """
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo" },\n'
        '  { "name": "foo/bar", "url": "svn://example.com/bar" },\n'
        '  { "name": "foo/tar", "url": "svn://example.com/tar" },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}\n'
        'recursedeps = ["bar"]')
    write(
        os.path.join('bar', 'DEPS'),
        'deps = {\n'
        '  "baz": "/baz",\n'
        '}\n'
        'recursedeps = ["baz"]')
    write(
        os.path.join('baz', 'DEPS'),
        'deps = {\n'
        '  "fizz": "/fizz",\n'
        '}')
    write(
        os.path.join('fizz', 'DEPS'),
        'deps = {\n'
        '  "fuzz": "/fuzz",\n'
        '}')
    write(
        os.path.join('tar', 'DEPS'),
        'deps = {\n'
        '  "taz": "/taz",\n'
        '}')

    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    self.assertEquals(
        [
          ('bar', 'svn://example.com/bar'),
          ('baz', 'svn://example.com/baz'),
          ('fizz', 'svn://example.com/fizz'),
          ('foo', 'svn://example.com/foo'),
          ('foo/bar', 'svn://example.com/bar'),
          ('foo/tar', 'svn://example.com/tar'),
        ],
        sorted(self._get_processed()))

  def testRecursedepsOverrideWithRelativePaths(self):
    """Verifies gclient respects |recursedeps| with relative paths."""

    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo" },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'use_relative_paths = True\n'
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}\n'
        'recursedeps = ["bar"]')
    write(
        os.path.join('foo/bar', 'DEPS'),
        'deps = {\n'
        '  "baz": "/baz",\n'
        '}')
    write(
        os.path.join('baz', 'DEPS'),
        'deps = {\n'
        '  "fizz": "/fizz",\n'
        '}')

    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    self.assertEquals(
        [
          ('foo', 'svn://example.com/foo'),
          ('foo/bar', 'svn://example.com/bar'),
          ('foo/baz', 'svn://example.com/baz'),
        ],
        self._get_processed())

  def testRelativeRecursion(self):
    """Verifies that nested use_relative_paths is always respected."""
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo" },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'use_relative_paths = True\n'
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}\n'
        'recursedeps = ["bar"]')
    write(
        os.path.join('foo/bar', 'DEPS'),
        'use_relative_paths = True\n'
        'deps = {\n'
        '  "baz": "/baz",\n'
        '}')
    write(
        os.path.join('baz', 'DEPS'),
        'deps = {\n'
        '  "fizz": "/fizz",\n'
        '}')

    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    self.assertEquals(
        [
          ('foo', 'svn://example.com/foo'),
          ('foo/bar', 'svn://example.com/bar'),
          ('foo/bar/baz', 'svn://example.com/baz'),
        ],
        self._get_processed())

  def testRecursionOverridesRecursedeps(self):
    """Verifies gclient respects |recursion| over |recursedeps|.

    |recursion| is set in a top-level DEPS file.  That value is meant
    to affect how many subdeps are parsed via recursion.

    |recursedeps| is set in each DEPS file to control whether or not
    to recurse into the immediate next subdep.

    This test verifies that if both syntaxes are mixed in a DEPS file,
    we disable |recursedeps| support and only obey |recursion|.

    Since this setting is evaluated per DEPS file, recursed DEPS
    files will each be re-evaluated according to the per DEPS rules.
    So a DEPS that only contains |recursedeps| could then override any
    previous |recursion| setting.  There is extra processing to ensure
    this does not happen.

    For this test to work correctly, we need to use a DEPS chain that
    only contains recursion controls in the top DEPS file.

    In foo, |recursion| and |recursedeps| are specified.  When we see
    |recursion|, we stop trying to use |recursedeps|.

    There are 2 constructions of DEPS here that are key to this test:

    (1) In foo, if we used |recursedeps| instead of |recursion|, we
        would also pull in bar.  Since bar's DEPS doesn't contain any
        recursion statements, we would stop processing at bar.

    (2) In fizz, if we used |recursedeps| at all, we should pull in
        fuzz.

    We expect to keep going past bar (satisfying 1) and we don't
    expect to pull in fuzz (satisfying 2).
    """
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo" },\n'
        '  { "name": "foo/bar", "url": "svn://example.com/bar" },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}\n'
        'recursion = 3\n'
        'recursedeps = ["bar"]')
    write(
        os.path.join('bar', 'DEPS'),
        'deps = {\n'
        '  "baz": "/baz",\n'
        '}')
    write(
        os.path.join('baz', 'DEPS'),
        'deps = {\n'
        '  "fizz": "/fizz",\n'
        '}')
    write(
        os.path.join('fizz', 'DEPS'),
        'deps = {\n'
        '  "fuzz": "/fuzz",\n'
        '}\n'
        'recursedeps = ["fuzz"]')
    write(
        os.path.join('fuzz', 'DEPS'),
        'deps = {\n'
        '  "tar": "/tar",\n'
        '}')

    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    self.assertEquals(
        [
          ('foo', 'svn://example.com/foo'),
          ('foo/bar', 'svn://example.com/bar'),
          ('bar', 'svn://example.com/bar'),
          # Deps after this would have been skipped if we were obeying
          # |recursedeps|.
          ('baz', 'svn://example.com/baz'),
          ('fizz', 'svn://example.com/fizz'),
          # And this dep would have been picked up if we were obeying
          # |recursedeps|.
          # 'svn://example.com/foo/bar/baz/fuzz',
        ],
        self._get_processed())

  def testRecursedepsAltfile(self):
    """Verifies gclient respects the |recursedeps| var syntax with overridden
    target DEPS file.

    This is what we mean to check here:
    - Naming an alternate DEPS file in recursedeps pulls from that one.
    """
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo" },\n'
        ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}\n'
        'recursedeps = [("bar", "DEPS.alt")]')
    write(os.path.join('bar', 'DEPS'), 'ERROR ERROR ERROR')
    write(
        os.path.join('bar', 'DEPS.alt'),
        'deps = {\n'
        '  "baz": "/baz",\n'
        '}')

    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    self.assertEquals(
        [
          ('foo', 'svn://example.com/foo'),
          ('bar', 'svn://example.com/bar'),
          ('baz', 'svn://example.com/baz'),
        ],
        self._get_processed())

  def testGitDeps(self):
    """Verifies gclient respects a .DEPS.git deps file.

    Along the way, we also test that if both DEPS and .DEPS.git are present,
    that gclient does not read the DEPS file.  This will reliably catch bugs
    where gclient is always hitting the wrong file (DEPS).
    """
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', '.DEPS.git'),
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "baz": "/baz",\n'
        '}')

    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    self.assertEquals(
        [
          ('foo', 'svn://example.com/foo'),
          ('bar', 'svn://example.com/bar'),
        ],
        self._get_processed())

  def testGitDepsFallback(self):
    """Verifies gclient respects fallback to DEPS upon missing deps file."""
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}')

    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    self.assertEquals(
        [
          ('foo', 'svn://example.com/foo'),
          ('bar', 'svn://example.com/bar'),
        ],
        self._get_processed())

  def testDepsFromNotAllowedHostsUnspecified(self):
    """Verifies gclient works fine with DEPS without allowed_hosts."""
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}')
    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    dep = obj.dependencies[0]
    self.assertEquals([], dep.findDepsFromNotAllowedHosts())
    self.assertEquals(frozenset(), dep.allowed_hosts)
    self._get_processed()

  def testDepsFromNotAllowedHostsOK(self):
    """Verifies gclient works fine with DEPS with proper allowed_hosts."""
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', '.DEPS.git'),
        'allowed_hosts = ["example.com"]\n'
        'deps = {\n'
        '  "bar": "svn://example.com/bar",\n'
        '}')
    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    dep = obj.dependencies[0]
    self.assertEquals([], dep.findDepsFromNotAllowedHosts())
    self.assertEquals(frozenset(['example.com']), dep.allowed_hosts)
    self._get_processed()

  def testDepsFromNotAllowedHostsBad(self):
    """Verifies gclient works fine with DEPS with proper allowed_hosts."""
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', '.DEPS.git'),
        'allowed_hosts = ["other.com"]\n'
        'deps = {\n'
        '  "bar": "svn://example.com/bar",\n'
        '}')
    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    obj.RunOnDeps('None', [])
    dep = obj.dependencies[0]
    self.assertEquals(frozenset(['other.com']), dep.allowed_hosts)
    self.assertEquals([dep.dependencies[0]], dep.findDepsFromNotAllowedHosts())
    self._get_processed()

  def testDepsParseFailureWithEmptyAllowedHosts(self):
    """Verifies gclient fails with defined but empty allowed_hosts."""
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'allowed_hosts = []\n'
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}')
    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    try:
      obj.RunOnDeps('None', [])
      self.fail()
    except gclient_utils.Error, e:
      self.assertIn('allowed_hosts must be', str(e))
    finally:
      self._get_processed()

  def testDepsParseFailureWithNonIterableAllowedHosts(self):
    """Verifies gclient fails with defined but non-iterable allowed_hosts."""
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'allowed_hosts = None\n'
        'deps = {\n'
        '  "bar": "/bar",\n'
        '}')
    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    try:
      obj.RunOnDeps('None', [])
      self.fail()
    except gclient_utils.Error, e:
      self.assertIn('allowed_hosts must be', str(e))
    finally:
      self._get_processed()

  def testCreatesCipdDependencies(self):
    """Verifies something."""
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "svn://example.com/foo",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'vars = {\n'
        '  "lemur_version": "version:1234",\n'
        '}\n'
        'deps = {\n'
        '  "bar": {\n'
        '    "packages": [{\n'
        '      "package": "lemur",\n'
        '      "version": Var("lemur_version"),\n'
        '    }],\n'
        '    "dep_type": "cipd",\n'
        '  }\n'
        '}')
    options, _ = gclient.OptionParser().parse_args([])
    options.validate_syntax = True
    obj = gclient.GClient.LoadCurrentConfig(options)

    self.assertEquals(1, len(obj.dependencies))
    sol = obj.dependencies[0]
    sol._condition = 'some_condition'

    sol.ParseDepsFile()
    self.assertEquals(1, len(sol.dependencies))
    dep = sol.dependencies[0]

    self.assertIsInstance(dep, gclient.CipdDependency)
    self.assertEquals(
        'https://chrome-infra-packages.appspot.com/lemur@version:1234',
        dep.url)

  def testSameDirAllowMultipleCipdDeps(self):
    """Verifies gclient allow multiple cipd deps under same directory."""
    parser = gclient.OptionParser()
    options, _ = parser.parse_args([])
    obj = gclient.GClient('foo', options)
    cipd_root = gclient_scm.CipdRoot(
        os.path.join(self.root_dir, 'dir1'), 'https://example.com')
    obj.add_dependencies_and_close(
      [
        gclient.Dependency(
            obj, 'foo', 'svn://example.com/foo', 'svn://example.com/foo', None,
            None, None, None, 'DEPS', True,
            False, None, True),
      ],
      [])
    obj.dependencies[0].add_dependencies_and_close(
      [
        gclient.CipdDependency(obj.dependencies[0], 'foo',
                               {'package': 'foo_package',
                                'version': 'foo_version'},
                               cipd_root, None, True, False,
                               'fake_condition'),
        gclient.CipdDependency(obj.dependencies[0], 'foo',
                               {'package': 'bar_package',
                                'version': 'bar_version'},
                               cipd_root, None, True, False,
                               'fake_condition'),
      ],
      [])
    dep0 = obj.dependencies[0].dependencies[0]
    dep1 = obj.dependencies[0].dependencies[1]
    self.assertEquals('https://example.com/foo_package@foo_version', dep0.url)
    self.assertEquals('https://example.com/bar_package@bar_version', dep1.url)

  def testFuzzyMatchUrlByURL(self):
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "https://example.com/foo.git",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "bar": "https://example.com/bar.git@bar_version",\n'
        '}')
    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    foo_sol = obj.dependencies[0]
    self.assertEqual(
        'https://example.com/foo.git',
        foo_sol.FuzzyMatchUrl(['https://example.com/foo.git', 'foo'])
    )

  def testFuzzyMatchUrlByURLNoGit(self):
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "https://example.com/foo.git",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "bar": "https://example.com/bar.git@bar_version",\n'
        '}')
    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    foo_sol = obj.dependencies[0]
    self.assertEqual(
        'https://example.com/foo',
        foo_sol.FuzzyMatchUrl(['https://example.com/foo', 'foo'])
    )

  def testFuzzyMatchUrlByName(self):
    write(
        '.gclient',
        'solutions = [\n'
        '  { "name": "foo", "url": "https://example.com/foo",\n'
        '    "deps_file" : ".DEPS.git",\n'
        '  },\n'
          ']')
    write(
        os.path.join('foo', 'DEPS'),
        'deps = {\n'
        '  "bar": "https://example.com/bar.git@bar_version",\n'
        '}')
    options, _ = gclient.OptionParser().parse_args([])
    obj = gclient.GClient.LoadCurrentConfig(options)
    foo_sol = obj.dependencies[0]
    self.assertEqual('foo', foo_sol.FuzzyMatchUrl(['foo']))


if __name__ == '__main__':
  sys.stdout = gclient_utils.MakeFileAutoFlush(sys.stdout)
  sys.stdout = gclient_utils.MakeFileAnnotated(sys.stdout, include_zero=True)
  sys.stderr = gclient_utils.MakeFileAutoFlush(sys.stderr)
  sys.stderr = gclient_utils.MakeFileAnnotated(sys.stderr, include_zero=True)
  logging.basicConfig(
      level=[logging.ERROR, logging.WARNING, logging.INFO, logging.DEBUG][
        min(sys.argv.count('-v'), 3)],
      format='%(relativeCreated)4d %(levelname)5s %(module)13s('
              '%(lineno)d) %(message)s')
  unittest.main()
