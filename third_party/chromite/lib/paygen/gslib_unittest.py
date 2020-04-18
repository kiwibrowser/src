# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the gslib module."""

from __future__ import print_function

import base64
import errno
import mox
import os

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib

from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib.paygen import gslib


# Typical output for a GS failure that is not our fault, and we should retry.
GS_RETRY_FAILURE = ('GSResponseError: status=403, code=InvalidAccessKeyId,'
                    'reason="Forbidden", message="Blah Blah Blah"')
# Typical output for a failure that we should not retry.
GS_DONE_FAILURE = ('AccessDeniedException:')


class TestGsLib(cros_test_lib.MoxTestCase):
  """Test gslib module."""

  def setUp(self):
    self.bucket_name = 'somebucket'
    self.bucket_uri = 'gs://%s' % self.bucket_name

    self.cmd_result = cros_build_lib.CommandResult()
    self.cmd_error = cros_build_lib.RunCommandError('', self.cmd_result)

    self.mox.StubOutWithMock(cros_build_lib, 'RunCommand')
    # Because of autodetection, we no longer know which gsutil binary
    # will be used.
    self.gsutil = mox.IsA(str)

  def testRetryGSLib(self):
    """Test our retry decorator"""
    @gslib.RetryGSLib
    def Success():
      pass

    @gslib.RetryGSLib
    def SuccessArguments(arg1, arg2=False, arg3=False):
      self.assertEqual(arg1, 1)
      self.assertEqual(arg2, 2)
      self.assertEqual(arg3, 3)

    class RetryTestException(gslib.GSLibError):
      """Testing gslib.GSLibError exception for Retrying cases."""

      def __init__(self):
        super(RetryTestException, self).__init__(GS_RETRY_FAILURE)

    class DoneTestException(gslib.GSLibError):
      """Testing gslib.GSLibError exception for Done cases."""

      def __init__(self):
        super(DoneTestException, self).__init__(GS_DONE_FAILURE)

    @gslib.RetryGSLib
    def Fail():
      raise RetryTestException()

    @gslib.RetryGSLib
    def FailCount(counter, exception):
      """Pass in [count] times to fail before passing.

      Using [] means the same object is used each retry, but it's contents
      are mutable.
      """
      counter[0] -= 1
      if counter[0] >= 0:
        raise exception()

      if exception == RetryTestException:
        # Make sure retries ran down to -1.
        self.assertEquals(-1, counter[0])

    Success()
    SuccessArguments(1, 2, 3)
    SuccessArguments(1, arg3=3, arg2=2)

    FailCount([1], RetryTestException)
    FailCount([2], RetryTestException)

    self.assertRaises(RetryTestException, Fail)
    self.assertRaises(DoneTestException, FailCount, [1], DoneTestException)
    self.assertRaises(gslib.CopyFail, FailCount, [3], gslib.CopyFail)
    self.assertRaises(gslib.CopyFail, FailCount, [4], gslib.CopyFail)

  def testIsGsURI(self):
    self.assertTrue(gslib.IsGsURI('gs://bucket/foo/bar'))
    self.assertTrue(gslib.IsGsURI('gs://bucket'))
    self.assertTrue(gslib.IsGsURI('gs://'))

    self.assertFalse(gslib.IsGsURI('file://foo/bar'))
    self.assertFalse(gslib.IsGsURI('/foo/bar'))

  def testRunGsutilCommand(self):
    args = ['TheCommand', 'Arg1', 'Arg2']
    cmd = [self.gsutil] + args

    # Set up the test replay script.
    # Run 1.
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndReturn(1)
    # Run 2.
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=False, redirect_stderr=True).AndReturn(2)
    # Run 3.
    cros_build_lib.RunCommand(cmd, redirect_stdout=True, redirect_stderr=True,
                              error_code_ok=True).AndReturn(3)
    # Run 4.
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndRaise(
            self.cmd_error)
    # Run 5.
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndRaise(
            OSError(errno.ENOENT, 'errmsg'))
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertEqual(1, gslib.RunGsutilCommand(args))
    self.assertEqual(2, gslib.RunGsutilCommand(args, redirect_stdout=False))
    self.assertEqual(3, gslib.RunGsutilCommand(args, error_code_ok=True))
    self.assertRaises(gslib.GSLibError, gslib.RunGsutilCommand, args)
    self.assertRaises(gslib.GsutilMissingError, gslib.RunGsutilCommand, args)
    self.mox.VerifyAll()

  def testCopy(self):
    src_path = '/path/to/some/file'
    dest_path = 'gs://bucket/some/gs/path'

    # Set up the test replay script.
    # Run 1, success.
    cmd = [self.gsutil, 'cp', src_path, dest_path]
    cros_build_lib.RunCommand(cmd, redirect_stdout=True, redirect_stderr=True)
    # Run 2, failure.
    error = cros_build_lib.RunCommandError(GS_RETRY_FAILURE, self.cmd_result)
    for _ix in xrange(gslib.RETRY_ATTEMPTS + 1):
      cmd = [self.gsutil, 'cp', src_path, dest_path]
      cros_build_lib.RunCommand(
          cmd, redirect_stdout=True, redirect_stderr=True).AndRaise(error)
    self.mox.ReplayAll()

    # Run the test verification.
    gslib.Copy(src_path, dest_path)
    self.assertRaises(gslib.CopyFail, gslib.Copy, src_path, dest_path)
    self.mox.VerifyAll()

  def testMove(self):
    src_path = 'gs://bucket/some/gs/path'
    dest_path = '/some/other/path'

    # Set up the test replay script.
    cmd = [self.gsutil, 'mv', src_path, dest_path]
    cros_build_lib.RunCommand(cmd, redirect_stdout=True, redirect_stderr=True)
    self.mox.ReplayAll()

    # Run the test verification.
    gslib.Move(src_path, dest_path)
    self.mox.VerifyAll()

  def testRemove(self):
    path1 = 'gs://bucket/some/gs/path'
    path2 = 'gs://bucket/some/other/path'

    # Set up the test replay script.
    # Run 1, one path.
    cros_build_lib.RunCommand([self.gsutil, 'rm', path1],
                              redirect_stdout=True, redirect_stderr=True)
    # Run 2, two paths.
    cros_build_lib.RunCommand([self.gsutil, 'rm', path1, path2],
                              redirect_stdout=True, redirect_stderr=True)
    # Run 3, one path, recursive.
    cros_build_lib.RunCommand([self.gsutil, 'rm', '-R', path1],
                              redirect_stdout=True, redirect_stderr=True)
    self.mox.ReplayAll()

    # Run the test verification.
    gslib.Remove(path1)
    gslib.Remove(path1, path2)
    gslib.Remove(path1, recurse=True)
    self.mox.VerifyAll()

  def testRemoveNoMatch(self):
    path = 'gs://bucket/some/gs/path'

    # Set up the test replay script.
    cmd = [self.gsutil, 'rm', path]
    cros_build_lib.RunCommand(cmd, redirect_stdout=True, redirect_stderr=True)
    self.mox.ReplayAll()

    # Run the test verification.
    gslib.Remove(path, ignore_no_match=True)
    self.mox.VerifyAll()

  def testRemoveFail(self):
    path = 'gs://bucket/some/gs/path'

    # Set up the test replay script.
    cmd = [self.gsutil, 'rm', path]
    error = cros_build_lib.RunCommandError(GS_RETRY_FAILURE, self.cmd_result)
    for _ix in xrange(gslib.RETRY_ATTEMPTS + 1):
      cros_build_lib.RunCommand(
          cmd, redirect_stdout=True, redirect_stderr=True).AndRaise(error)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertRaises(gslib.RemoveFail,
                      gslib.Remove, path)
    self.mox.VerifyAll()

  def testRemoveNotFoundExceptionIgnoreNoMatch(self):
    """Verify that setting ignore_no_match to True ignores NotFoundExceptions"""
    path = 'gs://bucket/some/gs/path/that/totally/does/not/exist'

    # Set up the test replay script.
    cmd = [self.gsutil, 'rm', '-R', path]
    failure_msg = 'Removing ' + path + '...\nNotFoundException: 404 ' + path
    self.cmd_result.error = failure_msg
    error = cros_build_lib.RunCommandError(failure_msg, self.cmd_result)
    cros_build_lib.RunCommand(cmd, redirect_stdout=True,
                              redirect_stderr=True).AndRaise(error)
    self.mox.ReplayAll()

    # Run the test verification.
    gslib.Remove(path, recurse=True, ignore_no_match=True)
    self.mox.VerifyAll()

  def testRemoveNotFoundException(self):
    """Verify that a NotFoundException results in a failure."""
    path = 'gs://bucket/some/gs/path/that/totally/does/not/exist'

    # Set up the test replay script.
    cmd = [self.gsutil, 'rm', path]
    failure_msg = 'Removing ' + path + '...\nNotFoundException: 404 ' + path
    self.cmd_result.error = failure_msg
    error = cros_build_lib.RunCommandError(failure_msg, self.cmd_result)
    for _ix in xrange(gslib.RETRY_ATTEMPTS + 1):
      cros_build_lib.RunCommand(cmd, redirect_stdout=True,
                                redirect_stderr=True).AndRaise(error)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertRaises(gslib.RemoveFail, gslib.Remove, path)
    self.mox.VerifyAll()

  def testCreateWithContents(self):
    gs_path = 'gs://chromeos-releases-test/create-with-contents-test'
    contents = 'Stuff with Rocks In'

    self.mox.StubOutWithMock(gslib, 'Copy')

    gslib.Copy(mox.IsA(str), gs_path)
    self.mox.ReplayAll()

    gslib.CreateWithContents(gs_path, contents)
    self.mox.VerifyAll()

  def testCat(self):
    path = 'gs://bucket/some/gs/path'

    # Set up the test replay script.
    cmd = [self.gsutil, 'cat', path]
    result = cros_test_lib.EasyAttr(error='', output='TheContent')
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndReturn(result)
    self.mox.ReplayAll()

    # Run the test verification.
    result = gslib.Cat(path)
    self.assertEquals('TheContent', result)
    self.mox.VerifyAll()

  def testCatFail(self):
    path = 'gs://bucket/some/gs/path'

    # Set up the test replay script.
    cmd = [self.gsutil, 'cat', path]
    for _ix in xrange(gslib.RETRY_ATTEMPTS + 1):
      cros_build_lib.RunCommand(
          cmd, redirect_stdout=True, redirect_stderr=True).AndRaise(
              self.cmd_error)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertRaises(gslib.CatFail, gslib.Cat, path)
    self.mox.VerifyAll()

  def testStat(self):
    path = 'gs://bucket/some/gs/path'

    # Set up the test replay script.
    cmd = [self.gsutil, 'stat', path]
    cros_build_lib.RunCommand(cmd, redirect_stdout=True,
                              redirect_stderr=True).AndReturn(self.cmd_result)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertIs(gslib.Stat(path), None)
    self.mox.VerifyAll()

  def testStatFail(self):
    path = 'gs://bucket/some/gs/path'

    # Set up the test replay script.
    cmd = [self.gsutil, 'stat', path]
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndRaise(
            self.cmd_error)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertRaises(gslib.StatFail, gslib.Stat, path)
    self.mox.VerifyAll()

  def testFileSize(self):
    gs_uri = '%s/%s' % (self.bucket_uri, 'some/file/path')

    # Set up the test replay script.
    cmd = [self.gsutil, '-d', 'stat', gs_uri]
    size = 96
    output = '\n'.join(['header: x-goog-generation: 1386322968237000',
                        'header: x-goog-metageneration: 1',
                        'header: x-goog-stored-content-encoding: identity',
                        'header: x-goog-stored-content-length: %d' % size,
                        'header: Content-Type: application/octet-stream'])

    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndReturn(
            cros_test_lib.EasyAttr(output=output))
    self.mox.ReplayAll()

    # Run the test verification.
    result = gslib.FileSize(gs_uri)
    self.assertEqual(size, result)
    self.mox.VerifyAll()

  def _TestCatWithHeaders(self, gs_uri, cmd_output, cmd_error):
    # Set up the test replay script.
    # Run 1, versioning not enabled in bucket, one line of output.
    cmd = ['gsutil', '-d', 'cat', gs_uri]
    cmd_result = cros_test_lib.EasyAttr(output=cmd_output,
                                        error=cmd_error,
                                        cmdstr=' '.join(cmd))
    cmd[0] = mox.IsA(str)
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndReturn(cmd_result)
    self.mox.ReplayAll()

  def testCatWithHeaders(self):
    gs_uri = '%s/%s' % (self.bucket_uri, 'some/file/path')
    generation = 123454321
    metageneration = 2
    error = '\n'.join([
        'header: x-goog-generation: %d' % generation,
        'header: x-goog-metageneration: %d' % metageneration,
    ])
    expected_output = 'foo'
    self._TestCatWithHeaders(gs_uri, expected_output, error)

    # Run the test verification.
    headers = {}
    result = gslib.Cat(gs_uri, headers=headers)
    self.assertEqual(generation, int(headers['generation']))
    self.assertEqual(metageneration, int(headers['metageneration']))
    self.assertEqual(result, expected_output)
    self.mox.VerifyAll()

  def testFileSizeNoSuchFile(self):
    gs_uri = '%s/%s' % (self.bucket_uri, 'some/file/path')

    # Set up the test replay script.
    cmd = [self.gsutil, '-d', 'stat', gs_uri]
    for _ in xrange(0, gslib.RETRY_ATTEMPTS + 1):
      cros_build_lib.RunCommand(
          cmd, redirect_stdout=True, redirect_stderr=True).AndRaise(
              self.cmd_error)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertRaises(gslib.URIError, gslib.FileSize, gs_uri)
    self.mox.VerifyAll()

  def testListFiles(self):
    files = [
        '%s/some/path' % self.bucket_uri,
        '%s/some/file/path' % self.bucket_uri,
    ]
    directories = [
        '%s/some/dir/' % self.bucket_uri,
        '%s/some/dir/path/' % self.bucket_uri,
    ]

    gs_uri = '%s/**' % self.bucket_uri
    cmd = [self.gsutil, 'ls', gs_uri]

    # Prepare cmd_result for a good run.
    # Fake a trailing empty line.
    output = '\n'.join(files + directories + [''])
    cmd_result_ok = cros_test_lib.EasyAttr(output=output, returncode=0)

    # Prepare exception for a run that finds nothing.
    stderr = 'CommandException: One or more URLs matched no objects.\n'
    cmd_result_empty = cros_build_lib.CommandResult(error=stderr)
    empty_exception = cros_build_lib.RunCommandError(stderr, cmd_result_empty)

    # Prepare exception for a run that triggers a GS failure.
    error = cros_build_lib.RunCommandError(GS_RETRY_FAILURE, self.cmd_result)

    # Set up the test replay script.
    # Run 1, runs ok.
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndReturn(
            cmd_result_ok)
    # Run 2, runs ok, sorts files.
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndReturn(
            cmd_result_ok)
    # Run 3, finds nothing.
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True).AndRaise(
            empty_exception)
    # Run 4, failure in GS.
    for _ix in xrange(gslib.RETRY_ATTEMPTS + 1):
      cros_build_lib.RunCommand(
          cmd, redirect_stdout=True, redirect_stderr=True).AndRaise(error)
    self.mox.ReplayAll()

    # Run the test verification.
    result = gslib.ListFiles(self.bucket_uri, recurse=True)
    self.assertEqual(files, result)
    result = gslib.ListFiles(self.bucket_uri, recurse=True, sort=True)
    self.assertEqual(sorted(files), result)
    result = gslib.ListFiles(self.bucket_uri, recurse=True)
    self.assertEqual([], result)
    self.assertRaises(gslib.GSLibError, gslib.ListFiles,
                      self.bucket_uri, recurse=True)
    self.mox.VerifyAll()

  def testCmp(self):
    uri1 = 'gs://some/gs/path'
    uri2 = 'gs://some/other/path'
    local_path = '/some/local/path'
    md5 = 'TheMD5Sum'

    self.mox.StubOutWithMock(gslib, 'MD5Sum')
    self.mox.StubOutWithMock(gslib.filelib, 'MD5Sum')

    # Set up the test replay script.
    # Run 1, same md5, both GS.
    gslib.MD5Sum(uri1).AndReturn(md5)
    gslib.MD5Sum(uri2).AndReturn(md5)
    # Run 2, different md5, both GS.
    gslib.MD5Sum(uri1).AndReturn(md5)
    gslib.MD5Sum(uri2).AndReturn('Other' + md5)
    # Run 3, same md5, one GS on local.
    gslib.MD5Sum(uri1).AndReturn(md5)
    gslib.filelib.MD5Sum(local_path).AndReturn(md5)
    # Run 4, different md5, one GS on local.
    gslib.MD5Sum(uri1).AndReturn(md5)
    gslib.filelib.MD5Sum(local_path).AndReturn('Other' + md5)
    # Run 5, missing file, both GS.
    gslib.MD5Sum(uri1).AndReturn(None)
    # Run 6, args are None.
    gslib.filelib.MD5Sum(None).AndReturn(None)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertTrue(gslib.Cmp(uri1, uri2))
    self.assertFalse(gslib.Cmp(uri1, uri2))
    self.assertTrue(gslib.Cmp(uri1, local_path))
    self.assertFalse(gslib.Cmp(uri1, local_path))
    self.assertFalse(gslib.Cmp(uri1, uri2))
    self.assertFalse(gslib.Cmp(None, None))
    self.mox.VerifyAll()

  def testMD5SumAccessError(self):
    gs_uri = 'gs://bucket/foo/bar/somefile'
    crc32c = 'c96fd51e'
    crc32c_64 = base64.b64encode(base64.b16decode(crc32c, casefold=True))
    md5_sum = 'b026324c6904b2a9cb4b88d6d61c81d1'
    md5_sum_64 = base64.b64encode(base64.b16decode(md5_sum, casefold=True))
    output = '\n'.join([
        '%s:' % gs_uri,
        '        Creation time:          Tue, 04 Mar 2014 19:55:26 GMT',
        '        Content-Language:       en',
        '        Content-Length:         2',
        '        Content-Type:           application/octet-stream',
        '        Hash (crc32c):          %s' % crc32c_64,
        '        Hash (md5):             %s' % md5_sum_64,
        '        ETag:                   CMi938jU+bwCEAE=',
        '        Generation:             1393962926989000',
        '        Metageneration:         1',
        '        ACL:                    ACCESS DENIED. Note: you need OWNER '
        'permission',
        '                                on the object to read its ACL.',
    ])

    # Set up the test replay script.
    cmd = [self.gsutil, 'ls', '-L', gs_uri]
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True,
        error_code_ok=True).AndReturn(
            cros_test_lib.EasyAttr(output=output))
    self.mox.ReplayAll()

    # Run the test verification.
    result = gslib.MD5Sum(gs_uri)
    self.assertEqual(md5_sum, result)
    self.mox.VerifyAll()

  def testMD5SumAccessOK(self):
    gs_uri = 'gs://bucket/foo/bar/somefile'
    crc32c = 'c96fd51e'
    crc32c_64 = base64.b64encode(base64.b16decode(crc32c, casefold=True))
    md5_sum = 'b026324c6904b2a9cb4b88d6d61c81d1'
    md5_sum_64 = base64.b64encode(base64.b16decode(md5_sum, casefold=True))
    output = '\n'.join([
        '%s:' % gs_uri,
        '        Creation time:          Tue, 04 Mar 2014 19:55:26 GMT',
        '        Content-Language:       en',
        '        Content-Length:         2',
        '        Content-Type:           application/octet-stream',
        '        Hash (crc32c):          %s' % crc32c_64,
        '        Hash (md5):             %s' % md5_sum_64,
        '        ETag:                   CMi938jU+bwCEAE=',
        '        Generation:             1393962926989000',
        '        Metageneration:         1',
        '        ACL:            [',
        '  {',
        '    "entity": "project-owners-134157665460",',
        '    "projectTeam": {',
        '      "projectNumber": "134157665460",',
        '      "team": "owners"',
        '    },',
        '    "role": "OWNER"',
        '  }',
        ']',
    ])
    # Set up the test replay script.
    cmd = [self.gsutil, 'ls', '-L', gs_uri]
    cros_build_lib.RunCommand(
        cmd, redirect_stdout=True, redirect_stderr=True,
        error_code_ok=True).AndReturn(
            cros_test_lib.EasyAttr(output=output))
    self.mox.ReplayAll()

    # Run the test verification.
    result = gslib.MD5Sum(gs_uri)
    self.assertEqual(md5_sum, result)
    self.mox.VerifyAll()


class TestGsLibAccess(cros_test_lib.MoxTempDirTestCase):
  """Test access to gs lib functionality.

  The tests here require GS .boto access to the gs://chromeos-releases-public
  bucket, which is world-readable.  Any .boto setup should do, but without
  a .boto there will be failures.
  """
  def populateUri(self, uri):
    local_path = os.path.join(self.tempdir, 'remote_content')
    osutils.WriteFile(local_path, 'some sample content')
    gslib.Copy(local_path, uri)
    return local_path

  @cros_test_lib.NetworkTest()
  def testCopyAndMD5Sum(self):
    """Higher-level functional test. Test MD5Sum OK."""
    with gs.TemporaryURL('chromite.gslib.md5') as tempuri:
      local_path = self.populateUri(tempuri)
      local_md5 = gslib.filelib.MD5Sum(local_path)
      gs_md5 = gslib.MD5Sum(tempuri)
      self.assertEqual(gs_md5, local_md5)

  @cros_test_lib.NetworkTest()
  def testExists(self):
    with gs.TemporaryURL('chromite.gslib.exists') as tempuri:
      self.populateUri(tempuri)
      self.assertTrue(gslib.Exists(tempuri))

    bogus_gs_path = 'gs://chromeos-releases-test/bogus/non-existent-url'
    self.assertFalse(gslib.Exists(bogus_gs_path))

  @cros_test_lib.NetworkTest()
  def testExistsFalse(self):
    """Test Exists logic with non-standard output from gsutil."""
    expected_output = ('GSResponseError: status=404, code=NoSuchKey,'
                       ' reason="Not Found",'
                       ' message="The specified key does not exist."')
    err1 = gslib.StatFail(expected_output)
    err2 = gslib.StatFail('You are using a deprecated alias, "getacl",'
                          'for the "acl" command.\n' +
                          expected_output)

    uri = 'gs://any/fake/uri/will/do'
    cmd = ['stat', uri]

    self.mox.StubOutWithMock(gslib, 'RunGsutilCommand')

    # Set up the test replay script.
    # Run 1, normal.
    gslib.RunGsutilCommand(cmd, failed_exception=gslib.StatFail,
                           get_headers_from_stdout=True).AndRaise(err1)
    # Run 2, extra output.
    gslib.RunGsutilCommand(cmd, failed_exception=gslib.StatFail,
                           get_headers_from_stdout=True).AndRaise(err2)
    self.mox.ReplayAll()

    # Run the test verification
    self.assertFalse(gslib.Exists(uri))
    self.assertFalse(gslib.Exists(uri))
    self.mox.VerifyAll()

  @cros_test_lib.NetworkTest()
  def testMD5SumBadPath(self):
    """Higher-level functional test.  Test MD5Sum bad path:

    1) Make up random, non-existent gs path
    2) Ask for MD5Sum.  Make sure it fails, but with no exeption.
    """

    gs_path = 'gs://chromeos-releases/awsedrftgyhujikol'
    gs_md5 = gslib.MD5Sum(gs_path)
    self.assertIsNone(gs_md5)

  @cros_test_lib.NetworkTest()
  def testMD5SumBadBucket(self):
    """Higher-level functional test.  Test MD5Sum bad bucket:

    1) Make up random, non-existent gs bucket and path
    2) Ask for MD5Sum.  Make sure it fails, with exception
    """

    gs_path = 'gs://lokijuhygtfrdesxcv/awsedrftgyhujikol'
    gs_md5 = gslib.MD5Sum(gs_path)
    self.assertIsNone(gs_md5)
