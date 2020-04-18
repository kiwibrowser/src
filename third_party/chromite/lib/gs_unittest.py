# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the gs.py module."""

from __future__ import print_function

import contextlib
import functools
import datetime
import mock
import os
import string

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.lib import retry_stats


def PatchGS(*args, **kwargs):
  """Convenience method for patching GSContext."""
  return mock.patch.object(gs.GSContext, *args, **kwargs)


class GSContextMock(partial_mock.PartialCmdMock):
  """Used to mock out the GSContext class."""
  TARGET = 'chromite.lib.gs.GSContext'
  ATTRS = ('__init__', 'DoCommand', 'DEFAULT_SLEEP_TIME',
           'DEFAULT_RETRIES', 'DEFAULT_BOTO_FILE', 'DEFAULT_GSUTIL_BIN',
           'DEFAULT_GSUTIL_BUILDER_BIN', 'GSUTIL_URL')
  DEFAULT_ATTR = 'DoCommand'

  GSResponsePreconditionFailed = """\
Copying file:///dev/null [Content-Type=application/octet-stream]...
Uploading   gs://chromeos-throw-away-bucket/vapier/null:         0 B    \r\
Uploading   gs://chromeos-throw-away-bucket/vapier/null:         0 B    \r\
PreconditionException: 412 Precondition Failed"""

  DEFAULT_SLEEP_TIME = 0
  DEFAULT_RETRIES = 2
  TMP_ROOT = '/tmp/cros_unittest'
  DEFAULT_BOTO_FILE = '%s/boto_file' % TMP_ROOT
  DEFAULT_GSUTIL_BIN = '%s/gsutil_bin' % TMP_ROOT
  DEFAULT_GSUTIL_BUILDER_BIN = DEFAULT_GSUTIL_BIN
  GSUTIL_URL = None

  def __init__(self):
    partial_mock.PartialCmdMock.__init__(self, create_tempdir=True)
    self.raw_gs_cmds = []

  def _SetGSUtilUrl(self):
    tempfile = os.path.join(self.tempdir, 'tempfile')
    osutils.WriteFile(tempfile, 'some content')
    gsutil_path = os.path.join(self.tempdir, gs.GSContext.GSUTIL_TAR)
    cros_build_lib.CreateTarball(gsutil_path, self.tempdir, inputs=[tempfile])
    self.GSUTIL_URL = 'file://%s' % gsutil_path

  def PreStart(self):
    os.environ.pop('BOTO_CONFIG', None)
    # Set it here for now, instead of mocking out Cached() directly because
    # python-mock has a bug with mocking out class methods with autospec=True.
    # TODO(rcui): Change this when this is fixed in PartialMock.
    self._SetGSUtilUrl()

  def _target__init__(self, *args, **kwargs):
    with PatchGS('_CheckFile', return_value=True):
      self.backup['__init__'](*args, **kwargs)

  def DoCommand(self, inst, gsutil_cmd, **kwargs):
    result = self._results['DoCommand'].LookupResult(
        (gsutil_cmd,), hook_args=(inst, gsutil_cmd), hook_kwargs=kwargs)

    rc_mock = cros_test_lib.RunCommandMock()
    rc_mock.AddCmdResult(
        partial_mock.ListRegex('gsutil'), result.returncode, result.output,
        result.error)

    with rc_mock:
      try:
        return self.backup['DoCommand'](inst, gsutil_cmd, **kwargs)
      finally:
        self.raw_gs_cmds.extend(args[0] for args, _ in rc_mock.call_args_list)


class AbstractGSContextTest(cros_test_lib.MockTempDirTestCase):
  """Base class for GSContext tests."""

  def setUp(self):
    self.gs_mock = self.StartPatcher(GSContextMock())
    self.gs_mock.SetDefaultCmdResult()
    self.ctx = gs.GSContext()


class CanonicalizeURLTest(cros_test_lib.TestCase):
  """Tests for the CanonicalizeURL function."""

  def _checkit(self, in_url, exp_url):
    self.assertEqual(gs.CanonicalizeURL(in_url), exp_url)

  def testPublicUrl(self):
    """Test public https URLs."""
    self._checkit(
        'https://commondatastorage.googleapis.com/releases/some/file/t.gz',
        'gs://releases/some/file/t.gz')

  def testPrivateUrl(self):
    """Test private https URLs."""
    self._checkit(
        'https://storage.cloud.google.com/releases/some/file/t.gz',
        'gs://releases/some/file/t.gz')

  def testDuplicateBase(self):
    """Test multiple prefixes in a single URL."""
    self._checkit(
        ('https://storage.cloud.google.com/releases/some/'
         'https://storage.cloud.google.com/some/file/t.gz'),
        ('gs://releases/some/'
         'https://storage.cloud.google.com/some/file/t.gz'))


class GsUrlToHttpTest(cros_test_lib.TestCase):
  """Tests for the CanonicalizeURL function."""

  def setUp(self):
    self.testUrls = [
        'gs://releases',
        'gs://releases/',
        'gs://releases/path',
        'gs://releases/path/',
        'gs://releases/path/file',
    ]

  def testPublicUrls(self):
    """Test public https URLs."""
    expected = [
        'https://storage.googleapis.com/releases',
        'https://storage.googleapis.com/releases/',
        'https://storage.googleapis.com/releases/path',
        'https://storage.googleapis.com/releases/path/',
        'https://storage.googleapis.com/releases/path/file',
    ]

    for gs_url, http_url in zip(self.testUrls, expected):
      self.assertEqual(gs.GsUrlToHttp(gs_url), http_url)
      self.assertEqual(gs.GsUrlToHttp(gs_url, directory=True), http_url)

  def testPrivateUrls(self):
    """Test public https URLs."""
    expected = [
        'https://storage.cloud.google.com/releases',
        'https://pantheon.corp.google.com/storage/browser/releases/',
        'https://storage.cloud.google.com/releases/path',
        'https://pantheon.corp.google.com/storage/browser/releases/path/',
        'https://storage.cloud.google.com/releases/path/file',
    ]

    for gs_url, http_url in zip(self.testUrls, expected):
      self.assertEqual(gs.GsUrlToHttp(gs_url, public=False), http_url)

  def testPrivateDirectoryUrls(self):
    """Test public https URLs."""
    expected = [
        'https://pantheon.corp.google.com/storage/browser/releases',
        'https://pantheon.corp.google.com/storage/browser/releases/',
        'https://pantheon.corp.google.com/storage/browser/releases/path',
        'https://pantheon.corp.google.com/storage/browser/releases/path/',
        'https://pantheon.corp.google.com/storage/browser/releases/path/file',
    ]

    for gs_url, http_url in zip(self.testUrls, expected):
      self.assertEqual(
          gs.GsUrlToHttp(gs_url, public=False, directory=True), http_url)


class VersionTest(AbstractGSContextTest):
  """Tests GSContext.gsutil_version functionality."""

  LOCAL_PATH = '/tmp/file'
  GIVEN_REMOTE = EXPECTED_REMOTE = 'gs://test/path/file'

  def testGetVersionStdout(self):
    """Simple gsutil_version fetch test from stdout."""
    self.gs_mock.AddCmdResult(partial_mock.In('version'), returncode=0,
                              output='gsutil version 3.35\n')
    self.assertEquals('3.35', self.ctx.gsutil_version)

  def testGetVersionStderr(self):
    """Simple gsutil_version fetch test from stderr."""
    self.gs_mock.AddCmdResult(partial_mock.In('version'), returncode=0,
                              error='gsutil version 3.36\n')
    self.assertEquals('3.36', self.ctx.gsutil_version)

  def testGetVersionCached(self):
    """Simple gsutil_version fetch test from cache."""
    # pylint: disable=protected-access
    self.ctx._gsutil_version = '3.37'
    self.assertEquals('3.37', self.ctx.gsutil_version)

  def testGetVersionNewFormat(self):
    """Simple gsutil_version fetch test for new gsutil output format."""
    self.gs_mock.AddCmdResult(partial_mock.In('version'), returncode=0,
                              output='gsutil version: 4.5\n')
    self.assertEquals('4.5', self.ctx.gsutil_version)

  def testGetVersionBadOutput(self):
    """Simple gsutil_version fetch test from cache."""
    self.gs_mock.AddCmdResult(partial_mock.In('version'), returncode=0,
                              output='gobblety gook\n')
    self.assertRaises(gs.GSContextException, getattr, self.ctx,
                      'gsutil_version')


class GetSizeTest(AbstractGSContextTest):
  """Tests GetSize functionality."""

  GETSIZE_PATH = 'gs://abc/1'

  def _GetSize(self, ctx, path, **kwargs):
    return ctx.GetSize(path, **kwargs)

  def GetSize(self, ctx=None, **kwargs):
    if ctx is None:
      ctx = self.ctx
    return self._GetSize(ctx, self.GETSIZE_PATH, **kwargs)

  def testBasic(self):
    """Simple test."""
    self.gs_mock.AddCmdResult(['stat', '--', self.GETSIZE_PATH],
                              output=StatTest.STAT_OUTPUT)
    self.assertEqual(self.GetSize(), 74)


class UnmockedGetSizeTest(cros_test_lib.TempDirTestCase):
  """Tests GetSize functionality w/out mocks."""

  @cros_test_lib.NetworkTest()
  def testBasic(self):
    """Simple test."""
    ctx = gs.GSContext()

    local_file = os.path.join(self.tempdir, 'foo')
    osutils.WriteFile(local_file, '!' * 5)

    with gs.TemporaryURL('chromite.getsize') as tempuri:
      ctx.Copy(local_file, tempuri)
      self.assertEqual(ctx.GetSize(tempuri), 5)

  def testLocal(self):
    """Test local files."""
    ctx = gs.GSContext()
    f = os.path.join(self.tempdir, 'f')

    osutils.Touch(f)
    self.assertEqual(ctx.GetSize(f), 0)

    osutils.WriteFile(f, 'f' * 10)
    self.assertEqual(ctx.GetSize(f), 10)


class LSTest(AbstractGSContextTest):
  """Tests LS/List functionality."""

  LS_PATH = 'gs://test/path/to/list'
  LS_OUTPUT_LINES = [
      '%s/foo' % LS_PATH,
      '%s/bar bell' % LS_PATH,
      '%s/nada/' % LS_PATH,
  ]
  LS_OUTPUT = '\n'.join(LS_OUTPUT_LINES)

  SIZE1 = 12345
  SIZE2 = 654321
  DT1 = datetime.datetime(2000, 1, 2, 10, 10, 10)
  DT2 = datetime.datetime(2010, 3, 14)
  DT_STR1 = DT1.strftime(gs.DATETIME_FORMAT)
  DT_STR2 = DT2.strftime(gs.DATETIME_FORMAT)
  DETAILED_LS_OUTPUT_LINES = [
      '%10d  %s  %s/foo' % (SIZE1, DT_STR1, LS_PATH),
      '%10d  %s  %s/bar bell' % (SIZE2, DT_STR2, LS_PATH),
      '          %s/nada/' % LS_PATH,
      'TOTAL: 3 objects, XXXXX bytes (X.XX GB)',
  ]
  DETAILED_LS_OUTPUT = '\n'.join(DETAILED_LS_OUTPUT_LINES)

  LIST_RESULT = [
      gs.GSListResult(
          content_length=SIZE1,
          creation_time=DT1,
          url='%s/foo' % LS_PATH,
          generation=None,
          metageneration=None),
      gs.GSListResult(
          content_length=SIZE2,
          creation_time=DT2,
          url='%s/bar bell' % LS_PATH,
          generation=None,
          metageneration=None),
      gs.GSListResult(
          content_length=None,
          creation_time=None,
          url='%s/nada/' % LS_PATH,
          generation=None,
          metageneration=None),
  ]

  def _LS(self, ctx, path, **kwargs):
    return ctx.LS(path, **kwargs)

  def LS(self, ctx=None, **kwargs):
    if ctx is None:
      ctx = self.ctx
    return self._LS(ctx, self.LS_PATH, **kwargs)

  def _List(self, ctx, path, **kwargs):
    return ctx.List(path, **kwargs)

  def List(self, ctx=None, **kwargs):
    if ctx is None:
      ctx = self.ctx
    return self._List(ctx, self.LS_PATH, **kwargs)

  def testBasicLS(self):
    """Simple LS test."""
    self.gs_mock.SetDefaultCmdResult(output=self.LS_OUTPUT)
    result = self.LS()
    self.gs_mock.assertCommandContains(['ls', '--', self.LS_PATH])

    self.assertEqual(self.LS_OUTPUT_LINES, result)

  def testBasicList(self):
    """Simple List test."""
    self.gs_mock.SetDefaultCmdResult(output=self.DETAILED_LS_OUTPUT)
    result = self.List(details=True)
    self.gs_mock.assertCommandContains(['ls', '-l', '--', self.LS_PATH])

    self.assertEqual(self.LIST_RESULT, result)


class UnmockedLSTest(cros_test_lib.TempDirTestCase):
  """Tests LS/List functionality w/out mocks."""

  def testLocalPaths(self):
    """Tests listing local paths."""
    ctx = gs.GSContext()

    # The tempdir should exist, but be empty, by default.
    self.assertEqual([], ctx.LS(self.tempdir))

    # Create a few random files.
    files = ['a', 'b', 'c!@', 'd e f', 'k\tj']
    for f in files:
      osutils.Touch(os.path.join(self.tempdir, f))

    # See what the code finds -- order is not guaranteed.
    found = ctx.LS(self.tempdir)
    files.sort()
    found.sort()
    self.assertEqual(files, found)

  @cros_test_lib.NetworkTest()
  def testRemotePath(self):
    """Tests listing remote paths."""
    ctx = gs.GSContext()

    with gs.TemporaryURL('chromite.ls') as tempuri:
      # The path shouldn't exist by default.
      with self.assertRaises(gs.GSNoSuchKey):
        ctx.LS(tempuri)

      # Create some files with known sizes.
      files = ['a', 'b', 'c!@', 'd e f', 'k\tj']
      uris = []
      for f in files:
        filename = os.path.join(self.tempdir, f)
        osutils.WriteFile(filename, f * 10)
        uri = os.path.join(tempuri, f)
        uris.append(uri)
        ctx.Copy(filename, uri)

      # Check the plain listing -- order is not guaranteed.
      found = ctx.LS(tempuri)
      uris.sort()
      found.sort()
      self.assertEqual(uris, found)

      # Check the detailed listing.
      found = ctx.List(tempuri, details=True)
      self.assertEqual(files, sorted([os.path.basename(x.url) for x in found]))

      # Make sure sizes line up.
      for f in found:
        l = len(os.path.basename(f.url)) * 10
        self.assertEqual(f.content_length, l)


class CopyTest(AbstractGSContextTest, cros_test_lib.TempDirTestCase):
  """Tests GSContext.Copy() functionality."""

  GIVEN_REMOTE = EXPECTED_REMOTE = 'gs://test/path/file'
  ACL = 'public-read'

  def setUp(self):
    self.local_path = os.path.join(self.tempdir, 'file')
    osutils.WriteFile(self.local_path, '')

  def _Copy(self, ctx, src, dst, **kwargs):
    return ctx.Copy(src, dst, **kwargs)

  def Copy(self, ctx=None, **kwargs):
    if ctx is None:
      ctx = self.ctx
    return self._Copy(ctx, self.local_path, self.GIVEN_REMOTE, **kwargs)

  def testBasic(self):
    """Simple copy test."""
    self.Copy()
    self.gs_mock.assertCommandContains(
        ['cp', '--', self.local_path, self.EXPECTED_REMOTE])

  def testWithACL(self):
    """ACL specified during init."""
    ctx = gs.GSContext(acl=self.ACL)
    self.Copy(ctx=ctx)
    self.gs_mock.assertCommandContains(['cp', '-a', self.ACL])

  def testWithACL2(self):
    """ACL specified during invocation."""
    self.Copy(acl=self.ACL)
    self.gs_mock.assertCommandContains(['cp', '-a', self.ACL])

  def testWithACL3(self):
    """ACL specified during invocation that overrides init."""
    ctx = gs.GSContext(acl=self.ACL)
    self.Copy(ctx=ctx, acl=self.ACL)
    self.gs_mock.assertCommandContains(['cp', '-a', self.ACL])

  def testRunCommandError(self):
    """Test RunCommandError is propagated."""
    self.gs_mock.AddCmdResult(partial_mock.In('cp'), returncode=1)
    self.assertRaises(cros_build_lib.RunCommandError, self.Copy)

  def testGSContextPreconditionFailed(self):
    """GSContextPreconditionFailed is raised properly."""
    self.gs_mock.AddCmdResult(
        partial_mock.In('cp'), returncode=1,
        error=self.gs_mock.GSResponsePreconditionFailed)
    self.assertRaises(gs.GSContextPreconditionFailed, self.Copy)

  def testNonRecursive(self):
    """Test non-recursive copy."""
    self.Copy(recursive=False)
    self.gs_mock.assertCommandContains(['-r'], expected=False)

  def testRecursive(self):
    """Test recursive copy."""
    self.Copy(recursive=True)
    self.gs_mock.assertCommandContains(['-r'], expected=False)
    self._Copy(self.ctx, self.tempdir, self.GIVEN_REMOTE, recursive=True)
    self.gs_mock.assertCommandContains(['cp', '-r'])

  def testCompress(self):
    """Test auto_compress behavior."""
    path = os.path.join(self.tempdir, 'ok.txt')
    self._Copy(self.ctx, path, self.GIVEN_REMOTE, auto_compress=True)
    self.gs_mock.assertCommandContains(['-Z'], expected=True)

  def testGeneration(self):
    """Test generation return value."""
    exp_gen = 1413571271901000
    error = (
        'Copying file:///dev/null [Content-Type=application/octet-stream]...\n'
        'Uploading   %(uri)s:               0 B    \r'
        'Uploading   %(uri)s:               0 B    \r'
        'Created: %(uri)s#%(gen)s\n'
    ) % {'uri': self.GIVEN_REMOTE, 'gen': exp_gen}
    self.gs_mock.AddCmdResult(partial_mock.In('cp'), returncode=0, error=error)
    gen = self.Copy()
    self.assertEqual(gen, exp_gen)

  def testGeneration404(self):
    """Test behavior when we get weird output."""
    error = (
        # This is a bit verbose, but it's from real output, so should be fine.
        'Copying file:///tmp/tmpyUUPg1 [Content-Type=application/octet-stream]'
        '...\n'
        'Uploading   ...recovery-R38-6158.66.0-mccloud.instructions.lock:'
        ' 0 B/38 B    \r'
        'Uploading   ...recovery-R38-6158.66.0-mccloud.instructions.lock:'
        ' 38 B/38 B    \r'
        'NotFoundException: 404 Attempt to get key for "gs://chromeos-releases'
        '/tobesigned/50,beta-\n'
        'channel,mccloud,6158.66.0,ChromeOS-\n'
        'recovery-R38-6158.66.0-mccloud.instructions.lock" failed. This can '
        'happen if the\n'
        'URI refers to a non-existent object or if you meant to operate on a '
        'directory\n'
        '(e.g., leaving off -R option on gsutil cp, mv, or ls of a bucket)\n'
    )
    self.gs_mock.AddCmdResult(partial_mock.In('cp'), returncode=1, error=error)
    self.assertEqual(self.Copy(), None)


class UnmockedCopyTest(cros_test_lib.TempDirTestCase):
  """Tests Copy functionality w/out mocks."""

  @cros_test_lib.NetworkTest()
  def testNormal(self):
    """Test normal upload/download behavior."""
    ctx = gs.GSContext()

    content = 'foooooooooooooooo!@!'

    local_src_file = os.path.join(self.tempdir, 'src.txt')
    local_dst_file = os.path.join(self.tempdir, 'dst.txt')

    osutils.WriteFile(local_src_file, content)

    with gs.TemporaryURL('chromite.cp') as tempuri:
      # Upload the file.
      gen = ctx.Copy(local_src_file, tempuri)

      # Verify the generation is sane.  All we can assume is that it's a valid
      # whole number greater than 0.
      self.assertNotEqual(gen, None)
      self.assertIn(type(gen), (int, long))
      self.assertGreater(gen, 0)

      # Verify the size is what we expect.
      self.assertEqual(ctx.GetSize(tempuri), os.path.getsize(local_src_file))

      # Copy it back down and verify the content is unchanged.
      ctx.Copy(tempuri, local_dst_file)
      new_content = osutils.ReadFile(local_dst_file)
      self.assertEqual(content, new_content)

  @cros_test_lib.NetworkTest()
  def testCompress(self):
    """Test auto_compress behavior."""
    ctx = gs.GSContext()

    # Need a string that compresses well.
    content = ('zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz'
               'zzzzzlkasjdf89j2;3o4kqmnioruasddfv89uxdp;foiasjdf0892qn5kln')

    local_src_file = os.path.join(self.tempdir, 'src.txt')
    local_dst_file = os.path.join(self.tempdir, 'dst.txt')

    osutils.WriteFile(local_src_file, content)

    with gs.TemporaryURL('chromite.cp') as tempuri:
      # Upload & compress the file.
      gen = ctx.Copy(local_src_file, tempuri, auto_compress=True)

      # Verify the generation is sane.  All we can assume is that it's a valid
      # whole number greater than 0.
      self.assertNotEqual(gen, None)
      self.assertGreater(gen, 0)

      # Verify the size is smaller (because it's compressed).
      self.assertLess(ctx.GetSize(tempuri), os.path.getsize(local_src_file))

      # Copy it back down and verify the content is decompressed & unchanged.
      ctx.Copy(tempuri, local_dst_file)
      new_content = osutils.ReadFile(local_dst_file)
      self.assertEqual(content, new_content)

  @cros_test_lib.NetworkTest()
  def testVersion(self):
    """Test version (generation) behavior."""
    ctx = gs.GSContext()

    local_src_file = os.path.join(self.tempdir, 'src.txt')

    with gs.TemporaryURL('chromite.cp') as tempuri:
      # Upload the file.
      osutils.WriteFile(local_src_file, 'gen0')
      gen = ctx.Copy(local_src_file, tempuri, version=0)

      # Verify the generation is sane.  All we can assume is that it's a valid
      # whole number greater than 0.
      self.assertNotEqual(gen, None)
      self.assertGreater(gen, 0)

      # The file should exist, so this will die due to wrong generation.
      osutils.WriteFile(local_src_file, 'gen-bad')
      self.assertRaises(gs.GSContextPreconditionFailed, ctx.Copy,
                        local_src_file, tempuri, version=0)

      # Sanity check the content is unchanged.
      self.assertEquals(ctx.Cat(tempuri), 'gen0')

      # Upload the file, but with the right generation.
      osutils.WriteFile(local_src_file, 'gen-new')
      gen = ctx.Copy(local_src_file, tempuri, version=gen)
      self.assertEquals(ctx.Cat(tempuri), 'gen-new')


class CopyIntoTest(CopyTest):
  """Test CopyInto functionality."""

  FILE = 'ooga'
  GIVEN_REMOTE = 'gs://test/path/file'
  EXPECTED_REMOTE = '%s/%s' % (GIVEN_REMOTE, FILE)

  def _Copy(self, ctx, *args, **kwargs):
    return ctx.CopyInto(*args, filename=self.FILE, **kwargs)


class RemoveTest(AbstractGSContextTest):
  """Tests GSContext.Remove() functionality."""

  def testNormal(self):
    """Test normal remove behavior."""
    self.assertEqual(self.ctx.Remove('gs://foo/bar'), None)

  def testMissing(self):
    """Test behavior w/missing files."""
    self.gs_mock.AddCmdResult(['rm', 'gs://foo/bar'],
                              error='CommandException: No URLs matched: '
                                    'gs://foo/bar',
                              returncode=1)
    self.assertRaises(gs.GSNoSuchKey, self.ctx.Remove, 'gs://foo/bar')
    # This one should not throw an exception.
    self.ctx.Remove('gs://foo/bar', ignore_missing=True)

  def testRecursive(self):
    """Verify we pass down -R in recursive mode."""
    self.ctx.Remove('gs://foo/bar', recursive=True)
    self.gs_mock.assertCommandContains(['rm', '-R'])


class UnmockedRemoveTest(cros_test_lib.TestCase):
  """Tests Remove functionality w/out mocks."""

  @cros_test_lib.NetworkTest()
  def testNormal(self):
    """Test normal remove behavior."""
    ctx = gs.GSContext()
    with gs.TemporaryURL('chromite.rm') as tempuri:
      ctx.Copy('/dev/null', tempuri)
      self.assertEqual(ctx.Remove(tempuri), None)

  @cros_test_lib.NetworkTest()
  def testMissing(self):
    """Test behavior w/missing files."""
    ctx = gs.GSContext()
    with gs.TemporaryURL('chromite.rm') as tempuri:
      self.assertRaises(gs.GSNoSuchKey, ctx.Remove, tempuri)
      # This one should not throw an exception.
      ctx.Remove(tempuri, ignore_missing=True)

  @cros_test_lib.NetworkTest()
  def testRecursive(self):
    """Verify recursive mode works."""
    files = ('a', 'b/c', 'd/e/ffff')
    ctx = gs.GSContext()
    with gs.TemporaryURL('chromite.rm') as tempuri:
      for p in files:
        ctx.Copy('/dev/null', os.path.join(tempuri, p))
      ctx.Remove(tempuri, recursive=True)
      for p in files:
        self.assertFalse(ctx.Exists(os.path.join(tempuri, p)))

  @cros_test_lib.NetworkTest()
  def testGeneration(self):
    """Test conditional remove behavior."""
    ctx = gs.GSContext()
    with gs.TemporaryURL('chromite.rm') as tempuri:
      ctx.Copy('/dev/null', tempuri)
      gen, _ = ctx.GetGeneration(tempuri)
      self.assertRaises(gs.GSContextPreconditionFailed, ctx.Remove,
                        tempuri, version=gen + 1)
      self.assertTrue(ctx.Exists(tempuri))
      ctx.Remove(tempuri, version=gen)
      self.assertFalse(ctx.Exists(tempuri))


class MoveTest(AbstractGSContextTest, cros_test_lib.TempDirTestCase):
  """Tests GSContext.Move() functionality."""

  GIVEN_REMOTE = EXPECTED_REMOTE = 'gs://test/path/file'

  def setUp(self):
    self.local_path = os.path.join(self.tempdir, 'file')
    osutils.WriteFile(self.local_path, '')

  def _Move(self, ctx, src, dst, **kwargs):
    return ctx.Move(src, dst, **kwargs)

  def Move(self, ctx=None, **kwargs):
    if ctx is None:
      ctx = self.ctx
    return self._Move(ctx, self.local_path, self.GIVEN_REMOTE, **kwargs)

  def testBasic(self):
    """Simple move test."""
    self.Move()
    self.gs_mock.assertCommandContains(
        ['mv', '--', self.local_path, self.EXPECTED_REMOTE])


class GSContextInitTest(cros_test_lib.MockTempDirTestCase):
  """Tests GSContext.__init__() functionality."""

  def setUp(self):
    os.environ.pop('BOTO_CONFIG', None)
    self.bad_path = os.path.join(self.tempdir, 'nonexistent')

    file_list = ['gsutil_bin', 'boto_file', 'acl_file']
    cros_test_lib.CreateOnDiskHierarchy(self.tempdir, file_list)
    for f in file_list:
      setattr(self, f, os.path.join(self.tempdir, f))
    self.StartPatcher(PatchGS('DEFAULT_BOTO_FILE', new=self.boto_file))
    self.StartPatcher(PatchGS('DEFAULT_GSUTIL_BIN', new=self.gsutil_bin))

  def testInitGsutilBin(self):
    """Test we use the given gsutil binary, erroring where appropriate."""
    self.assertEquals(gs.GSContext().gsutil_bin, self.gsutil_bin)
    self.assertRaises(gs.GSContextException,
                      gs.GSContext, gsutil_bin=self.bad_path)

  def testBadGSUtilBin(self):
    """Test exception thrown for bad gsutil paths."""
    self.assertRaises(gs.GSContextException, gs.GSContext,
                      gsutil_bin=self.bad_path)

  def testInitBotoFileEnv(self):
    """Test boto file environment is set correctly."""
    os.environ['BOTO_CONFIG'] = self.gsutil_bin
    self.assertTrue(gs.GSContext().boto_file, self.gsutil_bin)
    self.assertEqual(gs.GSContext(boto_file=self.acl_file).boto_file,
                     self.acl_file)
    self.assertEqual(gs.GSContext(boto_file=self.bad_path).boto_file,
                     self.bad_path)

  def testInitBotoFileEnvError(self):
    """Boto file through env var error."""
    self.assertEquals(gs.GSContext().boto_file, self.boto_file)
    # Check env usage next; no need to cleanup, teardown handles it,
    # and we want the env var to persist for the next part of this test.
    os.environ['BOTO_CONFIG'] = self.bad_path
    self.assertEqual(gs.GSContext().boto_file, self.bad_path)

  def testInitBotoFileError(self):
    """Test bad boto file."""
    self.assertEqual(gs.GSContext(boto_file=self.bad_path).boto_file,
                     self.bad_path)

  def testDoNotUseDefaultBotoFileIfItDoesNotExist(self):
    """Do not set boto file if the default path does not exist."""
    if 'BOTO_CONFIG' in os.environ:
      del os.environ['BOTO_CONFIG']
    gs.GSContext.DEFAULT_BOTO_FILE = 'foo/bar/doesnotexist'
    self.assertEqual(gs.GSContext().boto_file, None)

  def testInitAclFile(self):
    """Test ACL selection logic in __init__."""
    self.assertEqual(gs.GSContext().acl, None)
    self.assertEqual(gs.GSContext(acl=self.acl_file).acl,
                     self.acl_file)

  def _testHTTPProxySettings(self, d):
    flags = gs.GSContext().gsutil_flags
    for key in d:
      flag = 'Boto:%s=%s' % (key, d[key])
      error_msg = '%s not in %s' % (flag, ' '.join(flags))
      self.assertTrue(flag in flags, error_msg)

  def testHTTPProxy(self):
    """Test we set http proxy correctly."""
    d = {'proxy': 'fooserver', 'proxy_user': 'foouser',
         'proxy_pass': 'foopasswd', 'proxy_port': '8080'}
    os.environ['http_proxy'] = 'http://%s:%s@%s:%s/' % (
        d['proxy_user'], d['proxy_pass'], d['proxy'], d['proxy_port'])
    self._testHTTPProxySettings(d)

  def testHTTPProxyNoPort(self):
    """Test we accept http proxy without port number."""
    d = {'proxy': 'fooserver', 'proxy_user': 'foouser',
         'proxy_pass': 'foopasswd'}
    os.environ['http_proxy'] = 'http://%s:%s@%s/' % (
        d['proxy_user'], d['proxy_pass'], d['proxy'])
    self._testHTTPProxySettings(d)

  def testHTTPProxyNoUserPasswd(self):
    """Test we accept http proxy without user and password."""
    d = {'proxy': 'fooserver', 'proxy_port': '8080'}
    os.environ['http_proxy'] = 'http://%s:%s/' % (d['proxy'], d['proxy_port'])
    self._testHTTPProxySettings(d)

  def testHTTPProxyNoPasswd(self):
    """Test we accept http proxy without password."""
    d = {'proxy': 'fooserver', 'proxy_user': 'foouser',
         'proxy_port': '8080'}
    os.environ['http_proxy'] = 'http://%s@%s:%s/' % (
        d['proxy_user'], d['proxy'], d['proxy_port'])
    self._testHTTPProxySettings(d)


class GSDoCommandTest(cros_test_lib.TestCase):
  """Tests of gs.DoCommand behavior.

  This test class inherits from cros_test_lib.TestCase instead of from
  AbstractGSContextTest, because the latter unnecessarily mocks out
  cros_build_lib.RunCommand, in a way that breaks _testDoCommand (changing
  cros_build_lib.RunCommand to refer to a mock instance after the
  GenericRetry mock has already been set up to expect a reference to the
  original RunCommand).
  """

  def setUp(self):
    self.ctx = gs.GSContext()

  def _testDoCommand(self, ctx, headers=(), retries=None, sleep=None,
                     version=None, recursive=False):
    if retries is None:
      retries = ctx.DEFAULT_RETRIES
    if sleep is None:
      sleep = ctx.DEFAULT_SLEEP_TIME

    result = cros_build_lib.CommandResult(error='')
    with mock.patch.object(retry_stats, 'RetryWithStats', autospec=True,
                           return_value=result):
      ctx.Copy('/blah', 'gs://foon', version=version, recursive=recursive)
      cmd = [self.ctx.gsutil_bin] + self.ctx.gsutil_flags + list(headers)
      cmd += ['cp', '-v']
      if recursive:
        cmd += ['-r', '-e']
      cmd += ['--', '/blah', 'gs://foon']

      # pylint: disable=protected-access
      retry_stats.RetryWithStats.assert_called_once_with(
          retry_stats.GSUTIL,
          ctx._RetryFilter, retries,
          cros_build_lib.RunCommand,
          cmd, sleep=sleep,
          redirect_stderr=True,
          capture_output=True,
          extra_env=mock.ANY)

  def testDoCommandDefault(self):
    """Verify the internal DoCommand function works correctly."""
    self._testDoCommand(self.ctx)

  def testDoCommandCustom(self):
    """Test that retries and sleep parameters are honored."""
    ctx = gs.GSContext(retries=4, sleep=1)
    self._testDoCommand(ctx, retries=4, sleep=1)

  def testVersion(self):
    """Test that the version field expands into the header."""
    self._testDoCommand(self.ctx, version=3,
                        headers=['-h', 'x-goog-if-generation-match:3'])

  def testDoCommandRecursiveCopy(self):
    """Test that recursive copy command is honored."""
    self._testDoCommand(self.ctx, recursive=True)


class GSRetryFilterTest(cros_test_lib.TestCase):
  """Verifies that we filter and process gsutil errors correctly."""

  # pylint: disable=protected-access

  LOCAL_PATH = '/tmp/file'
  REMOTE_PATH = ('gs://chromeos-prebuilt/board/beltino/paladin-R33-4926.0.0'
                 '-rc2/packages/chromeos-base/autotest-tests-0.0.1-r4679.tbz2')
  GSUTIL_TRACKER_DIR = '/foo'
  UPLOAD_TRACKER_FILE = (
      'upload_TRACKER_9263880a80e4a582aec54eaa697bfcdd9c5621ea.9.tbz2__JSON.url'
  )
  DOWNLOAD_TRACKER_FILE = (
      'download_TRACKER_5a695131f3ef6e4c903f594783412bb996a7f375._file__JSON.'
      'etag')
  RETURN_CODE = 3

  def setUp(self):
    self.ctx = gs.GSContext()
    self.ctx.DEFAULT_GSUTIL_TRACKER_DIR = self.GSUTIL_TRACKER_DIR

  def _getException(self, cmd, error, returncode=RETURN_CODE):
    result = cros_build_lib.CommandResult(
        error=error,
        cmd=cmd,
        returncode=returncode)
    return cros_build_lib.RunCommandError('blah', result)

  def assertNoSuchKey(self, error_msg):
    cmd = ['gsutil', 'ls', self.REMOTE_PATH]
    e = self._getException(cmd, error_msg)
    self.assertRaises(gs.GSNoSuchKey, self.ctx._RetryFilter, e)

  def assertPreconditionFailed(self, error_msg):
    cmd = ['gsutil', 'ls', self.REMOTE_PATH]
    e = self._getException(cmd, error_msg)
    self.assertRaises(gs.GSContextPreconditionFailed,
                      self.ctx._RetryFilter, e)

  def testRetryOnlyFlakyErrors(self):
    """Test that we retry only flaky errors."""
    cmd = ['gsutil', 'ls', self.REMOTE_PATH]
    e = self._getException(cmd, 'ServiceException: 503')
    self.assertTrue(self.ctx._RetryFilter(e))

    e = self._getException(cmd, 'UnknownException: 603')
    self.assertFalse(self.ctx._RetryFilter(e))

  def testRaiseGSErrors(self):
    """Test that we raise appropriate exceptions."""
    self.assertNoSuchKey('CommandException: No URLs matched.')
    self.assertNoSuchKey('NotFoundException: 404')
    self.assertPreconditionFailed(
        'PreconditionException: 412 Precondition Failed')

  @mock.patch('chromite.lib.osutils.SafeUnlink')
  @mock.patch('chromite.lib.osutils.ReadFile')
  @mock.patch('os.path.exists')
  def testRemoveUploadTrackerFile(self, exists_mock, readfile_mock,
                                  unlink_mock):
    """Test removal of tracker files for resumable upload failures."""
    cmd = ['gsutil', 'cp', self.LOCAL_PATH, self.REMOTE_PATH]
    e = self._getException(cmd, self.ctx.RESUMABLE_UPLOAD_ERROR)
    exists_mock.return_value = True
    readfile_mock.return_value = 'foohash'
    self.ctx._RetryFilter(e)
    tracker_file_path = os.path.join(self.GSUTIL_TRACKER_DIR,
                                     self.UPLOAD_TRACKER_FILE)
    unlink_mock.assert_called_once_with(tracker_file_path)

  @mock.patch('chromite.lib.osutils.SafeUnlink')
  @mock.patch('chromite.lib.osutils.ReadFile')
  @mock.patch('os.path.exists')
  def testRemoveDownloadTrackerFile(self, exists_mock, readfile_mock,
                                    unlink_mock):
    """Test removal of tracker files for resumable download failures."""
    cmd = ['gsutil', 'cp', self.REMOTE_PATH, self.LOCAL_PATH]
    e = self._getException(cmd, self.ctx.RESUMABLE_DOWNLOAD_ERROR)
    exists_mock.return_value = True
    readfile_mock.return_value = 'foohash'
    self.ctx._RetryFilter(e)
    tracker_file_path = os.path.join(self.GSUTIL_TRACKER_DIR,
                                     self.DOWNLOAD_TRACKER_FILE)
    unlink_mock.assert_called_once_with(tracker_file_path)

  def testRemoveTrackerFileOnlyForCP(self):
    """Test that we remove tracker files only for 'gsutil cp'."""
    cmd = ['gsutil', 'ls', self.REMOTE_PATH]
    e = self._getException(cmd, self.ctx.RESUMABLE_DOWNLOAD_ERROR)

    with mock.MagicMock() as self.ctx.GetTrackerFilenames:
      self.ctx._RetryFilter(e)
      self.assertFalse(self.ctx.GetTrackerFilenames.called)

  def testNoRemoveTrackerFileOnOtherErrors(self):
    """Test that we do not attempt to delete tracker files for other errors."""
    cmd = ['gsutil', 'cp', self.REMOTE_PATH, self.LOCAL_PATH]
    e = self._getException(cmd, 'One or more URLs matched no objects')

    with mock.MagicMock() as self.ctx.GetTrackerFilenames:
      self.assertRaises(gs.GSNoSuchKey, self.ctx._RetryFilter, e)
      self.assertFalse(self.ctx.GetTrackerFilenames.called)

  def testRetryTransient(self):
    """Verify retry behavior when hitting b/11762375"""
    error = (
        'Removing gs://foo/bar/monkey...\n'
        'GSResponseError: status=403, code=InvalidAccessKeyId, '
        'reason="Forbidden", message="The User Id you provided '
        'does not exist in our records.", detail="GOOGBWPADTH7OV25KJXZ"'
    )
    e = self._getException(['gsutil', 'rm', 'gs://foo/bar/monkey'], error)
    self.assertEqual(self.ctx._RetryFilter(e), True)

  def testRetrySSLEOF(self):
    """Verify retry behavior when EOF occurs in violation of SSL protocol."""
    error = ('ssl.SSLError: [Errno 8] _ssl.c:510: EOF occurred in violation of'
             ' protocol')
    e = self._getException(['gsutil', 'cat', 'gs://totally/legit/uri'], error)
    self.assertEqual(self.ctx._RetryFilter(e), True)

  def testRetrySSLTimeout(self):
    """Verify retry behavior when read operation timed out."""
    error = 'ssl.SSLError: (\'The read operation timed out\',)'
    e = self._getException(['gsutil', 'cp', self.REMOTE_PATH, self.LOCAL_PATH],
                           error)
    self.assertEqual(self.ctx._RetryFilter(e), True)

  def testRetrySSLHandshakeTimeout(self):
    """Verify retry behavior when handshake operation timed out."""
    error = 'ssl.SSLError: _ssl.c:495: The handshake operation timed out'
    e = self._getException(['gsutil', 'cp', self.REMOTE_PATH, self.LOCAL_PATH],
                           error)
    self.assertEqual(self.ctx._RetryFilter(e), True)

  def testRetryAccessDeniedException(self):
    """Verify retry behavior when transient AccessDeniedException happens."""
    error = ('AccessDeniedException: 403 XXX@gmail.com does not have '
             'storage.objects.delete access to XXX'
             'CommandException: 1 file/object could not be transferred.')
    e = self._getException(['gsutil', 'cp', self.REMOTE_PATH, self.LOCAL_PATH],
                           error)
    self.assertEqual(self.ctx._RetryFilter(e), True)


class GSContextTest(AbstractGSContextTest):
  """Tests for GSContext()"""

  URL = 'gs://chromeos-image-archive/x86-mario-release/''R17-1413.0.0-a1-b1346'
  FILE_NAME = 'chromeos_R17-1413.0.0-a1_x86-mario_full_dev.bin'
  GS_PATH = 'gs://test/path/to/list'
  DT = datetime.datetime(2000, 1, 2, 10, 10, 10)
  DT_STR = DT.strftime(gs.DATETIME_FORMAT)
  DETAILED_LS_OUTPUT_LINES = [
      '%10d  %s  %s/mock_data' % (100, DT_STR, GS_PATH),
      '%10d  %s  %s/mock_data' % (100, DT_STR, GS_PATH),
      '%10d  %s  %s/%s' % (100, DT_STR, GS_PATH, FILE_NAME),
      'TOTAL: 3 objects, XXXXX bytes (X.XX GB)',
  ]

  LIST_RESULT = [
      gs.GSListResult(
          content_length=100,
          creation_time=DT,
          url='%s/mock_data' % GS_PATH,
          generation=None,
          metageneration=None),
      gs.GSListResult(
          content_length=100,
          creation_time=DT,
          url='%s/mock_data' % GS_PATH,
          generation=None,
          metageneration=None),
      gs.GSListResult(
          content_length=100,
          creation_time=DT,
          url='%s/%s' % (GS_PATH, FILE_NAME),
          generation=None,
          metageneration=None),
  ]

  def testTemporaryUrl(self):
    """Just verify the url helper generates valid URLs."""
    with gs.TemporaryURL('mock') as url:
      base = url[0:len(constants.TRASH_BUCKET)]
      self.assertEqual(base, constants.TRASH_BUCKET)

      valid_chars = set(string.ascii_letters + string.digits + '/-')
      used_chars = set(url[len(base) + 1:])
      self.assertEqual(used_chars - valid_chars, set())

  def testSetAclError(self):
    """Ensure SetACL blows up if the acl isn't specified."""
    self.assertRaises(gs.GSContextException, self.ctx.SetACL, 'gs://abc/3')

  def testSetDefaultAcl(self):
    """Test default ACL behavior."""
    self.ctx.SetACL('gs://abc/1', 'monkeys')
    self.gs_mock.assertCommandContains(['acl', 'set', 'monkeys', 'gs://abc/1'])

  def testSetAcl(self):
    """Base ACL setting functionality."""
    ctx = gs.GSContext(acl='/my/file/acl')
    ctx.SetACL('gs://abc/1')
    self.gs_mock.assertCommandContains(['acl', 'set', '/my/file/acl',
                                        'gs://abc/1'])

  def testChangeAcl(self):
    """Test changing an ACL."""
    basic_file = """
-g foo:READ

-u bar:FULL_CONTROL"""
    comment_file = """
# Give foo READ permission
-g foo:READ # Now foo can read this
  # This whole line should be removed
-u bar:FULL_CONTROL
# A comment at the end"""
    tempfile = os.path.join(self.tempdir, 'tempfile')
    ctx = gs.GSContext()

    osutils.WriteFile(tempfile, basic_file)
    ctx.ChangeACL('gs://abc/1', acl_args_file=tempfile)
    self.gs_mock.assertCommandContains([
        'acl', 'ch', '-g', 'foo:READ', '-u', 'bar:FULL_CONTROL', 'gs://abc/1'
    ])

    osutils.WriteFile(tempfile, comment_file)
    ctx.ChangeACL('gs://abc/1', acl_args_file=tempfile)
    self.gs_mock.assertCommandContains([
        'acl', 'ch', '-g', 'foo:READ', '-u', 'bar:FULL_CONTROL', 'gs://abc/1'
    ])

    ctx.ChangeACL('gs://abc/1',
                  acl_args=['-g', 'foo:READ', '-u', 'bar:FULL_CONTROL'])
    self.gs_mock.assertCommandContains([
        'acl', 'ch', '-g', 'foo:READ', '-u', 'bar:FULL_CONTROL', 'gs://abc/1'
    ])

    with self.assertRaises(gs.GSContextException):
      ctx.ChangeACL('gs://abc/1', acl_args_file=tempfile, acl_args=['foo'])

    with self.assertRaises(gs.GSContextException):
      ctx.ChangeACL('gs://abc/1')

  def testIncrement(self):
    """Test ability to atomically increment a counter."""
    ctx = gs.GSContext()

    with mock.patch.object(ctx, 'GetGeneration', return_value=(0, 0)):
      ctx.Counter('gs://abc/1').Increment()

    self.gs_mock.assertCommandContains(['cp', 'gs://abc/1'])

  def testGetGeneration(self):
    """Test ability to get the generation of a file."""
    self.gs_mock.AddCmdResult(['stat', '--', 'gs://abc/1'],
                              output=StatTest.STAT_OUTPUT)
    ctx = gs.GSContext()
    ctx.GetGeneration('gs://abc/1')
    self.gs_mock.assertCommandContains(['stat', '--', 'gs://abc/1'])

  def testCreateCached(self):
    """Test that the function runs through."""
    gs.GSContext(cache_dir=self.tempdir)

  def testReuseCached(self):
    """Test that second fetch is a cache hit."""
    gs.GSContext(cache_dir=self.tempdir)
    gs.GSUTIL_URL = None
    gs.GSContext(cache_dir=self.tempdir)

  def testUnknownError(self):
    """Test that when gsutil fails in an unknown way, we do the right thing."""
    self.gs_mock.AddCmdResult(['cat', '/asdf'], returncode=1)

    ctx = gs.GSContext()
    self.assertRaises(gs.GSCommandError, ctx.DoCommand, ['cat', '/asdf'])

  def testWaitForGsPathsAllPresent(self):
    """Test for waiting when all paths exist already."""
    ctx = gs.GSContext()

    with mock.patch.object(ctx, 'Exists', return_value=True):
      ctx.WaitForGsPaths(['/path1', '/path2'], 20)

  def testWaitForGsPathsDelayedSuccess(self):
    """Test for waiting, but not all paths exist so we timeout."""
    ctx = gs.GSContext()

    # First they both don't exist, then one does, then remaining does.
    exists = [False, False, True, False, True]
    with mock.patch.object(ctx, 'Exists', side_effect=exists):
      ctx.WaitForGsPaths(['/path1', '/path2'], 20, period=0.02)

  def testWaitForGsPathsTimeout(self):
    """Test for waiting, but not all paths exist so we timeout."""
    ctx = gs.GSContext()

    exists = {'/path1': True, '/path2': False}
    with mock.patch.object(ctx, 'Exists', side_effect=lambda p: exists[p]):
      self.assertRaises(gs.timeout_util.TimeoutError,
                        ctx.WaitForGsPaths, ['/path1', '/path2'],
                        timeout=1, period=0.02)

  def testParallelFalse(self):
    """Tests that "-m" is not used by default."""
    ctx = gs.GSContext()
    ctx.Copy('-', 'gs://abc/1')
    self.assertFalse(any('-m' in cmd for cmd in self.gs_mock.raw_gs_cmds))

  def testParallelTrue(self):
    """Tests that "-m" is used when you pass parallel=True."""
    ctx = gs.GSContext()
    ctx.Copy('gs://abc/1', 'gs://abc/2', parallel=True)
    self.assertTrue(all('-m' in cmd for cmd in self.gs_mock.raw_gs_cmds))

  def testNoParallelOpWithStdin(self):
    """Tests that "-m" is not used when we pipe the input."""
    ctx = gs.GSContext()
    ctx.Copy('gs://abc/1', 'gs://abc/2', input='foo', parallel=True)
    self.assertFalse(any('-m' in cmd for cmd in self.gs_mock.raw_gs_cmds))

  def testGetGsNamesWithWait(self):
    """Test that we get the target artifact that is available."""
    pattern = '*_full_*'

    ctx = gs.GSContext()

    # GSUtil ls gs://archive_url_prefix/.
    self.gs_mock.SetDefaultCmdResult(
        output='\n'.join(self.DETAILED_LS_OUTPUT_LINES))

    # Timeout explicitly set to 0 to test that we always run at least once.
    result = ctx.GetGsNamesWithWait(pattern, self.URL, period=1, timeout=0)
    self.assertEqual([self.FILE_NAME], result)

  def testGetGsNamesWithWaitWithDirectStat(self):
    """We should directly stat an artifact whose name is fully spelled out."""
    pattern = self.FILE_NAME

    ctx = gs.GSContext()

    exists = {'%s/%s' % (self.URL, self.FILE_NAME): True}
    with mock.patch.object(ctx, 'Exists', side_effect=lambda p: exists[p]):
      # Timeout explicitly set to 0 to test that we always run at least once.
      result = ctx.GetGsNamesWithWait(pattern, self.URL, period=1, timeout=0)
      self.assertEqual([self.FILE_NAME], result)

  def testGetGsNamesWithWaitWithRetry(self):
    """Test that we can poll until all target artifacts are available."""
    pattern = '*_full_*'

    ctx = gs.GSContext()

    # GSUtil ls gs://archive_url_prefix/.
    exists = [[], self.LIST_RESULT]
    with mock.patch.object(ctx, 'List', side_effect=exists):
      # Timeout explicitly set to 0 to test that we always run at least once.
      result = ctx.GetGsNamesWithWait(pattern, self.URL, period=1, timeout=4)
      self.assertEqual([self.FILE_NAME], result)

  def testGetGsNamesWithWaitTimeout(self):
    """Test that we can poll until all target artifacts are available."""
    pattern = '*_full_*'

    ctx = gs.GSContext()

    # GSUtil ls gs://archive_url_prefix/.
    self.gs_mock.SetDefaultCmdResult(output=[])

    # Timeout explicitly set to 0 to test that we always run at least once.
    result = ctx.GetGsNamesWithWait(pattern, self.URL, period=2, timeout=1)
    self.assertEqual(None, result)


class UnmockedGSContextTest(cros_test_lib.TempDirTestCase):
  """Tests for GSContext that go over the network."""

  @cros_test_lib.NetworkTest()
  def testIncrement(self):
    ctx = gs.GSContext()
    with gs.TemporaryURL('testIncrement') as url:
      counter = ctx.Counter(url)
      self.assertEqual(0, counter.Get())
      for i in xrange(1, 4):
        self.assertEqual(i, counter.Increment())
        self.assertEqual(i, counter.Get())

  @cros_test_lib.NetworkTest()
  def testGetGsNamesWithWait(self):
    """Tests getting files from remote paths."""
    file_name = 'chromeos_R17-1413.0.0-a1_x86-mario_full_dev.bin'
    pattern = '*_full_*'

    ctx = gs.GSContext()

    with gs.TemporaryURL('testGetGsNamesWithWait') as url:
      # The path shouldn't exist by default.
      with self.assertRaises(gs.GSNoSuchKey):
        ctx.GetGsNamesWithWait(pattern, url, period=2, timeout=1)

      # Create files in bucket.
      files = ['mock_data', file_name]
      for f in files:
        filename = os.path.join(self.tempdir, f)
        osutils.WriteFile(filename, f * 10)
        uri = os.path.join(url, f)
        ctx.Copy(filename, uri)

      # Use pattern to get google storage names.
      self.assertEqual(
          ctx.GetGsNamesWithWait(pattern, url, period=1, timeout=0),
          [file_name])
      # Use direct file name to get google storage names.
      self.assertEqual(
          ctx.GetGsNamesWithWait(file_name, url, period=1, timeout=0),
          [file_name])

      # Remove the matched file, verify that GetGsNamesWithWait returns None.
      ctx.Remove(os.path.join(url, file_name), ignore_missing=True)

      self.assertEqual(
          ctx.GetGsNamesWithWait(pattern, url, period=1, timeout=3), None)
      self.assertEqual(
          ctx.GetGsNamesWithWait(file_name, url, period=1, timeout=3), None)


class StatTest(AbstractGSContextTest):
  """Tests Stat functionality."""

  # Convenient constant for mocking Stat results.
  STAT_OUTPUT = """gs://abc/1:
        Creation time:    Sat, 23 Aug 2014 06:53:20 GMT
        Content-Language: en
        Content-Length:   74
        Content-Type:   application/octet-stream
        Hash (crc32c):    BBPMPA==
        Hash (md5):   ms+qSYvgI9SjXn8tW/5UpQ==
        ETag:     CNCgocbmqMACEAE=
        Generation:   1408776800850000
        Metageneration:   1
      """

  # Stat output can vary based on how/when the file was created.
  STAT_OUTPUT_OLDER = """gs://abc/1:
        Creation time:    Sat, 23 Aug 2014 06:53:20 GMT
        Content-Length:   74
        Content-Type:   application/octet-stream
        Hash (crc32c):    BBPMPA==
        Hash (md5):   ms+qSYvgI9SjXn8tW/5UpQ==
        ETag:     CNCgocbmqMACEAE=
        Generation:   1408776800850000
        Metageneration:   1
      """

  # Stat output with no MD5 (this is not guaranteed by GS, and
  # can be omitted if files are uploaded as composite objects).
  STAT_OUTPUT_NO_MD5 = """gs://abc/1:
        Creation time:    Sat, 23 Aug 2014 06:53:20 GMT
        Content-Language: en
        Content-Length:   74
        Content-Type:   application/octet-stream
        Hash (crc32c):    BBPMPA==
        ETag:     CNCgocbmqMACEAE=
        Generation:   1408776800850000
        Metageneration:   1
      """

  # When stat throws an error.  It's a special snow flake.
  STAT_ERROR_OUTPUT = 'No URLs matched gs://abc/1'

  def testStat(self):
    """Test ability to get the generation of a file."""
    self.gs_mock.AddCmdResult(['stat', '--', 'gs://abc/1'],
                              output=self.STAT_OUTPUT)
    ctx = gs.GSContext()
    result = ctx.Stat('gs://abc/1')
    self.gs_mock.assertCommandContains(['stat', '--', 'gs://abc/1'])

    self.assertEqual(result.creation_time,
                     datetime.datetime(2014, 8, 23, 6, 53, 20))
    self.assertEqual(result.content_length, 74)
    self.assertEqual(result.content_type, 'application/octet-stream')
    self.assertEqual(result.hash_crc32c, 'BBPMPA==')
    self.assertEqual(result.hash_md5, 'ms+qSYvgI9SjXn8tW/5UpQ==')
    self.assertEqual(result.etag, 'CNCgocbmqMACEAE=')
    self.assertEqual(result.generation, 1408776800850000)
    self.assertEqual(result.metageneration, 1)

  def testStatOlderOutput(self):
    """Test ability to get the generation of a file."""
    self.gs_mock.AddCmdResult(['stat', '--', 'gs://abc/1'],
                              output=self.STAT_OUTPUT_OLDER)
    ctx = gs.GSContext()
    result = ctx.Stat('gs://abc/1')
    self.gs_mock.assertCommandContains(['stat', '--', 'gs://abc/1'])

    self.assertEqual(result.creation_time,
                     datetime.datetime(2014, 8, 23, 6, 53, 20))
    self.assertEqual(result.content_length, 74)
    self.assertEqual(result.content_type, 'application/octet-stream')
    self.assertEqual(result.hash_crc32c, 'BBPMPA==')
    self.assertEqual(result.hash_md5, 'ms+qSYvgI9SjXn8tW/5UpQ==')
    self.assertEqual(result.etag, 'CNCgocbmqMACEAE=')
    self.assertEqual(result.generation, 1408776800850000)
    self.assertEqual(result.metageneration, 1)

  def testStatNoMD5(self):
    """Make sure GSContext works without an MD5."""
    self.gs_mock.AddCmdResult(['stat', '--', 'gs://abc/1'],
                              output=self.STAT_OUTPUT_NO_MD5)
    ctx = gs.GSContext()
    result = ctx.Stat('gs://abc/1')
    self.gs_mock.assertCommandContains(['stat', '--', 'gs://abc/1'])

    self.assertEqual(result.creation_time,
                     datetime.datetime(2014, 8, 23, 6, 53, 20))
    self.assertEqual(result.content_length, 74)
    self.assertEqual(result.content_type, 'application/octet-stream')
    self.assertEqual(result.hash_crc32c, 'BBPMPA==')
    self.assertEqual(result.hash_md5, None)
    self.assertEqual(result.etag, 'CNCgocbmqMACEAE=')
    self.assertEqual(result.generation, 1408776800850000)
    self.assertEqual(result.metageneration, 1)

  def testStatNoExist(self):
    """Test ability to get the generation of a file."""
    self.gs_mock.AddCmdResult(['stat', '--', 'gs://abc/1'],
                              error=self.STAT_ERROR_OUTPUT,
                              returncode=1)
    ctx = gs.GSContext()
    self.assertRaises(gs.GSNoSuchKey, ctx.Stat, 'gs://abc/1')
    self.gs_mock.assertCommandContains(['stat', '--', 'gs://abc/1'])


class UnmockedStatTest(cros_test_lib.TempDirTestCase):
  """Tests Stat functionality w/out mocks."""

  @cros_test_lib.NetworkTest()
  def testStat(self):
    """Test ability to get the generation of a file."""
    ctx = gs.GSContext()
    with gs.TemporaryURL('testStat') as url:

      # The URL doesn't exist. Test Stat for this case.
      self.assertRaises(gs.GSNoSuchKey, ctx.Stat, url)

      # Populate the URL.
      ctx.CreateWithContents(url, 'test file contents')

      # Stat a URL that exists.
      result = ctx.Stat(url)

    # Verify the Stat results.
    self.assertIsInstance(result.creation_time, datetime.datetime)
    self.assertEqual(result.content_length, 18)
    self.assertEqual(result.content_type, 'application/octet-stream')
    self.assertEqual(result.hash_crc32c, 'wUc4sQ==')
    self.assertEqual(result.hash_md5, 'iRvNNwBhmvUVG/lbg2/5sQ==')
    self.assertIsInstance(result.etag, str)
    self.assertIsInstance(result.generation, int)
    self.assertEqual(result.metageneration, 1)

  @cros_test_lib.NetworkTest()
  def testMissing(self):
    """Test exceptions when the file doesn't exist."""
    ctx = gs.GSContext()
    with gs.TemporaryURL('testStat') as url:
      self.assertRaises(gs.GSNoSuchKey, ctx.Stat, url)
      self.assertFalse(ctx.Exists(url))

  def testExists(self):
    """Test Exists behavior with local files."""
    ctx = gs.GSContext()
    f = os.path.join(self.tempdir, 'f')

    self.assertFalse(ctx.Exists(f))

    osutils.Touch(f)
    self.assertTrue(ctx.Exists(f))


class CatTest(cros_test_lib.TempDirTestCase):
  """Tests GSContext.Copy() functionality."""

  def testLocalFile(self):
    """Tests catting a local file."""
    ctx = gs.GSContext()
    filename = os.path.join(self.tempdir, 'myfile')
    content = 'foo'
    osutils.WriteFile(filename, content)
    self.assertEqual(content, ctx.Cat(filename))

  def testLocalMissingFile(self):
    """Tests catting a missing local file."""
    ctx = gs.GSContext()
    with self.assertRaises(gs.GSNoSuchKey):
      ctx.Cat(os.path.join(self.tempdir, 'does/not/exist'))

  def testLocalForbiddenFile(self):
    """Tests catting a local file that we don't have access to."""
    ctx = gs.GSContext()
    filename = os.path.join(self.tempdir, 'myfile')
    content = 'foo'
    osutils.WriteFile(filename, content)
    os.chmod(filename, 000)
    with self.assertRaises(gs.GSContextException):
      ctx.Cat(filename)

  @cros_test_lib.NetworkTest()
  def testNetworkFile(self):
    """Tests catting a GS file."""
    ctx = gs.GSContext()
    filename = os.path.join(self.tempdir, 'myfile')
    content = 'fOoOoOoo1\n\thi@!*!(\r\r\nend'
    osutils.WriteFile(filename, content)

    with gs.TemporaryURL('chromite.cat') as tempuri:
      ctx.Copy(filename, tempuri)
      self.assertEqual(content, ctx.Cat(tempuri))

  @cros_test_lib.NetworkTest()
  def testNetworkMissingFile(self):
    """Tests catting a missing GS file."""
    ctx = gs.GSContext()
    with gs.TemporaryURL('chromite.cat') as tempuri:
      with self.assertRaises(gs.GSNoSuchKey):
        ctx.Cat(tempuri)

  @cros_test_lib.NetworkTest()
  def testStreamingRemoteFile(self):
    """Test streaming a remote file."""
    ctx = gs.GSContext()
    with gs.TemporaryURL('chromite.cat') as url:
      # The default chunksize is 0x100000 (1MB).
      first_chunk = 'a' * 0x100000
      second_chunk = 'aaaaaaaaabbbbbbccc'
      ctx.CreateWithContents(url, first_chunk + second_chunk)

      result = ctx.StreamingCat(url)
      # Get the 1st chunk.
      self.assertEquals(next(result), first_chunk)
      # Then the second chunk.
      self.assertEquals(next(result), second_chunk)
      # At last, no more.
      with self.assertRaises(StopIteration):
        next(result)


class DryRunTest(cros_test_lib.RunCommandTestCase):
  """Verify dry_run works for all of GSContext."""

  def setUp(self):
    self.ctx = gs.GSContext(dry_run=True)

  def tearDown(self):
    # Verify we don't try to call gsutil at all.
    for call_args in self.rc.call_args_list:
      self.assertNotIn('gsutil', call_args[0][0])

  def testCat(self):
    """Test Cat in dry_run mode."""
    self.assertEqual(self.ctx.Cat('gs://foo/bar'), '')

  def testChangeACL(self):
    """Test ChangeACL in dry_run mode."""
    self.assertEqual(
        self.ctx.ChangeACL('gs://foo/bar', acl_args_file='/dev/null'),
        None)

  def testCopy(self):
    """Test Copy in dry_run mode."""
    self.ctx.Copy('/dev/null', 'gs://foo/bar')
    self.ctx.Copy('gs://foo/bar', '/dev/null')

  def testCreateWithContents(self):
    """Test Copy in dry_run mode."""
    self.ctx.CreateWithContents('gs://foo/bar', 'My Little Content(tm)')

  def testCopyInto(self):
    """Test CopyInto in dry_run mode."""
    self.ctx.CopyInto('/dev/null', 'gs://foo/bar')

  def testDoCommand(self):
    """Test DoCommand in dry_run mode."""
    self.ctx.DoCommand(['a-bad-command'])

  def testExists(self):
    """Test Exists in dry_run mode."""
    self.assertEqual(self.ctx.Exists('gs://foo/bar'), True)

  def testGetGeneration(self):
    """Test GetGeneration in dry_run mode."""
    self.assertEqual(self.ctx.GetGeneration('gs://foo/bar'), (0, 0))

  def testGetSize(self):
    """Test GetSize in dry_run mode."""
    self.assertEqual(self.ctx.GetSize('gs://foo/bar'), 0)

  def testGetTrackerFilenames(self):
    """Test GetTrackerFilenames in dry_run mode."""
    self.ctx.GetTrackerFilenames('foo')

  def testLS(self):
    """Test LS in dry_run mode."""
    self.assertEqual(self.ctx.LS('gs://foo/bar'), [])

  def testList(self):
    """Test List in dry_run mode."""
    self.assertEqual(self.ctx.List('gs://foo/bar'), [])

  def testMove(self):
    """Test Move in dry_run mode."""
    self.ctx.Move('gs://foo/bar', 'gs://foo/bar2')

  def testRemove(self):
    """Test Remove in dry_run mode."""
    self.ctx.Remove('gs://foo/bar')

  def testSetACL(self):
    """Test SetACL in dry_run mode."""
    self.assertEqual(self.ctx.SetACL('gs://foo/bar', 'bad-acl'), None)

  def testStat(self):
    """Test Stat in dry_run mode."""
    result = self.ctx.Stat('gs://foo/bar')
    self.assertEqual(result.content_length, 0)
    self.assertNotEqual(result.creation_time, None)

  def testStreamingCat(self):
    """Test StreamingCat in dry_run mode."""
    result = self.ctx.StreamingCat('gs://foo/bar')
    self.assertEqual(next(result), '')
    with self.assertRaises(StopIteration):
      next(result)

  def testVersion(self):
    """Test gsutil_version in dry_run mode."""
    self.assertEqual(self.ctx.gsutil_version, gs.GSContext.GSUTIL_VERSION)


class InitBotoTest(AbstractGSContextTest):
  """Test boto file interactive initialization."""

  # pylint: disable=protected-access

  GS_LS_ERROR = """\
You are attempting to access protected data with no configured credentials.
Please see http://code.google.com/apis/storage/docs/signup.html for
details about activating the Google Cloud Storage service and then run the
"gsutil config" command to configure gsutil to use these credentials."""

  GS_LS_ERROR2 = """\
GSResponseError: status=400, code=MissingSecurityHeader, reason=Bad Request, \
detail=Authorization."""

  GS_LS_BENIGN = """\
"GSResponseError: status=400, code=MissingSecurityHeader, reason=Bad Request,
detail=A nonempty x-goog-project-id header is required for this request."""

  def setUp(self):
    self.boto_file = os.path.join(self.tempdir, 'boto_file')
    self.ctx = gs.GSContext(boto_file=self.boto_file)
    self.auth_cmd = ['ls', gs.AUTHENTICATION_BUCKET]

  def testGSLsSkippableError(self):
    """Benign GS error."""
    self.gs_mock.AddCmdResult(self.auth_cmd,
                              returncode=1, error=self.GS_LS_BENIGN)
    self.assertTrue(self.ctx._TestGSLs())

  def testGSLsAuthorizationError1(self):
    """GS authorization error 1."""
    self.gs_mock.AddCmdResult(self.auth_cmd,
                              returncode=1, error=self.GS_LS_ERROR)
    self.assertFalse(self.ctx._TestGSLs())

  def testGSLsError2(self):
    """GS authorization error 2."""
    self.gs_mock.AddCmdResult(self.auth_cmd,
                              returncode=1, error=self.GS_LS_ERROR2)
    self.assertFalse(self.ctx._TestGSLs())

  def _WriteBotoFile(self, contents, *_args, **_kwargs):
    osutils.WriteFile(self.ctx.boto_file, contents)

  def testInitGSLsFailButSuccess(self):
    """Invalid GS Config, but we config properly."""
    self.gs_mock.AddCmdResult(self.auth_cmd,
                              returncode=1, error=self.GS_LS_ERROR)
    self.ctx._InitBoto()

  def _AddLsConfigResult(self, side_effect=None):
    self.gs_mock.AddCmdResult(self.auth_cmd,
                              returncode=1, error=self.GS_LS_ERROR)
    self.gs_mock.AddCmdResult(['config'], returncode=1, side_effect=side_effect)

  def testGSLsFailAndConfigError(self):
    """Invalid GS Config, and we fail to config."""
    self._AddLsConfigResult(
        side_effect=functools.partial(self._WriteBotoFile, 'monkeys'))
    self.assertRaises(cros_build_lib.RunCommandError, self.ctx._InitBoto)

  def testGSLsFailAndEmptyConfigFile(self):
    """Invalid GS Config, and we raise error on empty config file."""
    self._AddLsConfigResult(
        side_effect=functools.partial(self._WriteBotoFile, ''))
    self.assertRaises(gs.GSContextException, self.ctx._InitBoto)


class GSCounterTest(AbstractGSContextTest):
  """Tests GSCounter functionality."""

  COUNTER_URI = 'gs://foo/mock/counter'
  INITIAL_VALUE = 100

  def setUp(self):
    self.counter = gs.GSCounter(self.ctx, self.COUNTER_URI)
    self.cat_mock = self.PatchObject(self.ctx, 'Cat')
    self.gen_mock = self.PatchObject(self.ctx, 'GetGeneration',
                                     return_value=(1, 1))
    self._SetCounter(self.INITIAL_VALUE)

  def _SetCounter(self, value):
    """Set the test counter to |value|."""
    self.cat_mock.return_value = str(value)

  def testGetInitial(self):
    """Test Get when the counter doesn't exist."""
    self.cat_mock.side_effect = gs.GSNoSuchKey
    self.assertEqual(self.counter.Get(), 0)

  def testGet(self):
    """Basic Get() test."""
    self.assertEqual(self.counter.Get(), self.INITIAL_VALUE)

  def testIncrement(self):
    """Basic Increment() test."""
    self.assertEqual(self.counter.Increment(), self.INITIAL_VALUE + 1)

  def testDecrement(self):
    """Basic Decrement() test."""
    self.assertEqual(self.counter.Decrement(), self.INITIAL_VALUE - 1)

  def testReset(self):
    """Basic Reset() test."""
    self.assertEqual(self.counter.Reset(), 0)

  def testStreakIncrement(self):
    """Basic StreakIncrement() test."""
    self._SetCounter(10)
    self.assertEqual(self.counter.StreakIncrement(), 11)

  def testStreakIncrementReset(self):
    """Test StreakIncrement() when the counter is negative."""
    self._SetCounter(-10)
    self.assertEqual(self.counter.StreakIncrement(), 1)

  def testStreakDecrement(self):
    """Basic StreakDecrement() test."""
    self._SetCounter(-10)
    self.assertEqual(self.counter.StreakDecrement(), -11)

  def testStreakDecrementReset(self):
    """Test StreakDecrement() when the counter is positive."""
    self._SetCounter(10)
    self.assertEqual(self.counter.StreakDecrement(), -1)


class UnmockedGSCounterTest(cros_test_lib.TestCase):
  """Tests GSCounter functionality w/out mocks."""

  @staticmethod
  @contextlib.contextmanager
  def _Counter():
    ctx = gs.GSContext()
    with gs.TemporaryURL('chromite.counter') as tempuri:
      yield gs.GSCounter(ctx, tempuri)

  @staticmethod
  def _SetCounter(counter, value):
    """Set the test counter to |value|."""
    counter.AtomicCounterOperation(value, lambda x: value)

  @cros_test_lib.NetworkTest()
  def testGetInitial(self):
    """Test Get when the counter doesn't exist."""
    with self._Counter() as counter:
      self.assertEqual(counter.Get(), 0)

  @cros_test_lib.NetworkTest()
  def testGet(self):
    """Basic Get() test."""
    with self._Counter() as counter:
      self._SetCounter(counter, 100)
      self.assertEqual(counter.Get(), 100)

  @cros_test_lib.NetworkTest()
  def testIncrement(self):
    """Basic Increment() test."""
    with self._Counter() as counter:
      self._SetCounter(counter, 100)
      self.assertEqual(counter.Increment(), 101)

  @cros_test_lib.NetworkTest()
  def testDecrement(self):
    """Basic Decrement() test."""
    with self._Counter() as counter:
      self._SetCounter(counter, 100)
      self.assertEqual(counter.Decrement(), 99)

  @cros_test_lib.NetworkTest()
  def testReset(self):
    """Basic Reset() test."""
    with self._Counter() as counter:
      self._SetCounter(counter, 100)
      self.assertEqual(counter.Reset(), 0)
      self.assertEqual(counter.Get(), 0)

  @cros_test_lib.NetworkTest()
  def testStreakIncrement(self):
    """Basic StreakIncrement() test."""
    with self._Counter() as counter:
      self._SetCounter(counter, 100)
      self.assertEqual(counter.StreakIncrement(), 101)

  @cros_test_lib.NetworkTest()
  def testStreakIncrementReset(self):
    """Test StreakIncrement() when the counter is negative."""
    with self._Counter() as counter:
      self._SetCounter(counter, -100)
      self.assertEqual(counter.StreakIncrement(), 1)

  @cros_test_lib.NetworkTest()
  def testStreakDecrement(self):
    """Basic StreakDecrement() test."""
    with self._Counter() as counter:
      self._SetCounter(counter, -100)
      self.assertEqual(counter.StreakDecrement(), -101)

  @cros_test_lib.NetworkTest()
  def testStreakDecrementReset(self):
    """Test StreakDecrement() when the counter is positive."""
    with self._Counter() as counter:
      self._SetCounter(counter, 100)
      self.assertEqual(counter.StreakDecrement(), -1)
