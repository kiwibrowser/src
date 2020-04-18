#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import StringIO
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from testing_support.super_mox import SuperMoxTestBase
from testing_support import trial_dir

import gclient_utils
import subprocess2


class GclientUtilBase(SuperMoxTestBase):
  def setUp(self):
    super(GclientUtilBase, self).setUp()
    gclient_utils.sys.stdout.flush = lambda: None
    self.mox.StubOutWithMock(subprocess2, 'Popen')
    self.mox.StubOutWithMock(subprocess2, 'communicate')


class CheckCallAndFilterTestCase(GclientUtilBase):
  class ProcessIdMock(object):
    def __init__(self, test_string):
      self.stdout = StringIO.StringIO(test_string)
      self.pid = 9284
    # pylint: disable=no-self-use
    def wait(self):
      return 0

  def _inner(self, args, test_string):
    cwd = 'bleh'
    gclient_utils.sys.stdout.write(
        '________ running \'boo foo bar\' in \'bleh\'\n')
    for i in test_string:
      gclient_utils.sys.stdout.write(i)
    # pylint: disable=no-member
    subprocess2.Popen(
        args,
        cwd=cwd,
        stdout=subprocess2.PIPE,
        stderr=subprocess2.STDOUT,
        bufsize=0).AndReturn(self.ProcessIdMock(test_string))

    os.getcwd()
    self.mox.ReplayAll()
    compiled_pattern = gclient_utils.re.compile(r'a(.*)b')
    line_list = []
    capture_list = []
    def FilterLines(line):
      line_list.append(line)
      assert isinstance(line, str), type(line)
      match = compiled_pattern.search(line)
      if match:
        capture_list.append(match.group(1))
    gclient_utils.CheckCallAndFilterAndHeader(
        args, cwd=cwd, always=True, filter_fn=FilterLines)
    self.assertEquals(line_list, ['ahah', 'accb', 'allo', 'addb'])
    self.assertEquals(capture_list, ['cc', 'dd'])

  def testCheckCallAndFilter(self):
    args = ['boo', 'foo', 'bar']
    test_string = 'ahah\naccb\nallo\naddb\n'
    self._inner(args, test_string)
    self.checkstdout('________ running \'boo foo bar\' in \'bleh\'\n'
        'ahah\naccb\nallo\naddb\n'
        '________ running \'boo foo bar\' in \'bleh\'\nahah\naccb\nallo\naddb'
        '\n')


class SplitUrlRevisionTestCase(GclientUtilBase):
  def testSSHUrl(self):
    url = "ssh://test@example.com/test.git"
    rev = "ac345e52dc"
    out_url, out_rev = gclient_utils.SplitUrlRevision(url)
    self.assertEquals(out_rev, None)
    self.assertEquals(out_url, url)
    out_url, out_rev = gclient_utils.SplitUrlRevision("%s@%s" % (url, rev))
    self.assertEquals(out_rev, rev)
    self.assertEquals(out_url, url)
    url = "ssh://example.com/test.git"
    out_url, out_rev = gclient_utils.SplitUrlRevision(url)
    self.assertEquals(out_rev, None)
    self.assertEquals(out_url, url)
    out_url, out_rev = gclient_utils.SplitUrlRevision("%s@%s" % (url, rev))
    self.assertEquals(out_rev, rev)
    self.assertEquals(out_url, url)
    url = "ssh://example.com/git/test.git"
    out_url, out_rev = gclient_utils.SplitUrlRevision(url)
    self.assertEquals(out_rev, None)
    self.assertEquals(out_url, url)
    out_url, out_rev = gclient_utils.SplitUrlRevision("%s@%s" % (url, rev))
    self.assertEquals(out_rev, rev)
    self.assertEquals(out_url, url)
    rev = "test-stable"
    out_url, out_rev = gclient_utils.SplitUrlRevision("%s@%s" % (url, rev))
    self.assertEquals(out_rev, rev)
    self.assertEquals(out_url, url)
    url = "ssh://user-name@example.com/~/test.git"
    out_url, out_rev = gclient_utils.SplitUrlRevision(url)
    self.assertEquals(out_rev, None)
    self.assertEquals(out_url, url)
    out_url, out_rev = gclient_utils.SplitUrlRevision("%s@%s" % (url, rev))
    self.assertEquals(out_rev, rev)
    self.assertEquals(out_url, url)
    url = "ssh://user-name@example.com/~username/test.git"
    out_url, out_rev = gclient_utils.SplitUrlRevision(url)
    self.assertEquals(out_rev, None)
    self.assertEquals(out_url, url)
    out_url, out_rev = gclient_utils.SplitUrlRevision("%s@%s" % (url, rev))
    self.assertEquals(out_rev, rev)
    self.assertEquals(out_url, url)
    url = "git@github.com:dart-lang/spark.git"
    out_url, out_rev = gclient_utils.SplitUrlRevision(url)
    self.assertEquals(out_rev, None)
    self.assertEquals(out_url, url)
    out_url, out_rev = gclient_utils.SplitUrlRevision("%s@%s" % (url, rev))
    self.assertEquals(out_rev, rev)
    self.assertEquals(out_url, url)

  def testSVNUrl(self):
    url = "svn://example.com/test"
    rev = "ac345e52dc"
    out_url, out_rev = gclient_utils.SplitUrlRevision(url)
    self.assertEquals(out_rev, None)
    self.assertEquals(out_url, url)
    out_url, out_rev = gclient_utils.SplitUrlRevision("%s@%s" % (url, rev))
    self.assertEquals(out_rev, rev)
    self.assertEquals(out_url, url)


class GClientUtilsTest(trial_dir.TestCase):
  def testHardToDelete(self):
    # Use the fact that tearDown will delete the directory to make it hard to do
    # so.
    l1 = os.path.join(self.root_dir, 'l1')
    l2 = os.path.join(l1, 'l2')
    l3 = os.path.join(l2, 'l3')
    f3 = os.path.join(l3, 'f3')
    os.mkdir(l1)
    os.mkdir(l2)
    os.mkdir(l3)
    gclient_utils.FileWrite(f3, 'foo')
    os.chmod(f3, 0)
    os.chmod(l3, 0)
    os.chmod(l2, 0)
    os.chmod(l1, 0)

  def testUpgradeToHttps(self):
    values = [
        ['', ''],
        [None, None],
        ['foo', 'https://foo'],
        ['http://foo', 'https://foo'],
        ['foo/', 'https://foo/'],
        ['ssh-svn://foo', 'ssh-svn://foo'],
        ['ssh-svn://foo/bar/', 'ssh-svn://foo/bar/'],
        ['codereview.chromium.org', 'https://codereview.chromium.org'],
        ['codereview.chromium.org/', 'https://codereview.chromium.org/'],
        ['http://foo:10000', 'http://foo:10000'],
        ['http://foo:10000/bar', 'http://foo:10000/bar'],
        ['foo:10000', 'http://foo:10000'],
        ['foo:', 'https://foo:'],
    ]
    for content, expected in values:
      self.assertEquals(
          expected, gclient_utils.UpgradeToHttps(content))

  def testParseCodereviewSettingsContent(self):
    values = [
        ['# bleh\n', {}],
        ['\t# foo : bar\n', {}],
        ['Foo:bar', {'Foo': 'bar'}],
        ['Foo:bar:baz\n', {'Foo': 'bar:baz'}],
        [' Foo : bar ', {'Foo': 'bar'}],
        [' Foo : bar \n', {'Foo': 'bar'}],
        ['a:b\n\rc:d\re:f', {'a': 'b', 'c': 'd', 'e': 'f'}],
        ['an_url:http://value/', {'an_url': 'http://value/'}],
        [
          'CODE_REVIEW_SERVER : http://r/s',
          {'CODE_REVIEW_SERVER': 'https://r/s'}
        ],
        ['VIEW_VC:http://r/s', {'VIEW_VC': 'https://r/s'}],
    ]
    for content, expected in values:
      self.assertEquals(
          expected, gclient_utils.ParseCodereviewSettingsContent(content))


if __name__ == '__main__':
  import unittest
  unittest.main()

# vim: ts=2:sw=2:tw=80:et:
