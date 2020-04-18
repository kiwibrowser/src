# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the urilib module."""

from __future__ import print_function

import os

from chromite.lib import cros_test_lib
from chromite.lib import osutils

from chromite.lib.paygen import filelib
from chromite.lib.paygen import gslib
from chromite.lib.paygen import urilib


# We access private members to test them.
# pylint: disable=protected-access


class FakeHttpResponse(object):
  """For simulating http response objects."""

  class FakeHeaders(object):
    """Helper class for faking HTTP headers in a response."""

    def __init__(self, headers_dict):
      self.headers_dict = headers_dict

    def getheader(self, name):
      return self.headers_dict.get(name)

  def __init__(self, code, headers_dict=None):
    self.code = code
    self.headers = FakeHttpResponse.FakeHeaders(headers_dict)

  def getcode(self):
    return self.code


class TestFileManipulation(cros_test_lib.TempDirTestCase):
  """Test general urilib file methods together."""

  # pylint: disable=attribute-defined-outside-init

  FILE1 = 'file1'
  FILE2 = 'file2'
  SUBDIR = 'subdir'
  SUBFILE = '%s/file3' % SUBDIR

  FILE1_CONTENTS = 'Howdy doody there dandy'
  FILE2_CONTENTS = 'Once upon a time in a galaxy far far away.'
  SUBFILE_CONTENTS = 'Five little monkeys jumped on the bed.'

  GS_DIR = 'gs://chromeos-releases-public/unittest'

  def setUp(self):
    # Use a subdir specifically for the cache so we can use the tempdir for
    # other things (including tempfiles by gsutil/etc...).
    self.filesdir = os.path.join(self.tempdir, 'unittest-cache')
    osutils.SafeMakedirs(self.filesdir)

  def _SetUpDirs(self):
    self.file1_local = os.path.join(self.filesdir, self.FILE1)
    self.file2_local = os.path.join(self.filesdir, self.FILE2)
    self.subdir_local = os.path.join(self.filesdir, self.SUBDIR)
    self.subfile_local = os.path.join(self.filesdir, self.SUBFILE)

    self.file1_gs = os.path.join(self.GS_DIR, self.FILE1)
    self.file2_gs = os.path.join(self.GS_DIR, self.FILE2)
    self.subdir_gs = os.path.join(self.GS_DIR, self.SUBDIR)
    self.subfile_gs = os.path.join(self.GS_DIR, self.SUBFILE)

    # Pre-populate local dir with contents.
    with open(self.file1_local, 'w') as out1:
      out1.write(self.FILE1_CONTENTS)

    with open(self.file2_local, 'w') as out2:
      out2.write(self.FILE2_CONTENTS)

    os.makedirs(self.subdir_local)

    with open(self.subfile_local, 'w') as out3:
      out3.write(self.SUBFILE_CONTENTS)

    # Make sure gs:// directory is ready (empty).
    gslib.Remove(os.path.join(self.GS_DIR, '*'), recurse=True,
                 ignore_no_match=True)

  @cros_test_lib.NetworkTest()
  def testIntegration(self):
    self._SetUpDirs()

    self.assertTrue(urilib.Exists(self.filesdir, as_dir=True))
    self.assertTrue(urilib.Exists(self.file1_local))
    self.assertTrue(urilib.Exists(self.file2_local))
    self.assertTrue(urilib.Exists(self.subfile_local))
    self.assertTrue(urilib.Exists(self.subdir_local, as_dir=True))

    self.assertFalse(urilib.Exists(self.file1_gs))
    self.assertFalse(urilib.Exists(self.file2_gs))
    self.assertFalse(urilib.Exists(self.subfile_gs))

    shallow_local_files = [self.file1_local, self.file2_local]
    deep_local_files = shallow_local_files + [self.subfile_local]
    shallow_gs_files = [self.file1_gs, self.file2_gs]
    deep_gs_files = shallow_gs_files + [self.subfile_gs]

    # Test ListFiles, local version.
    self.assertEquals(set(shallow_local_files),
                      set(urilib.ListFiles(self.filesdir)))
    self.assertEquals(set(deep_local_files),
                      set(urilib.ListFiles(self.filesdir, recurse=True)))

    # Test CopyFiles, from local to GS.
    self.assertEquals(set(deep_gs_files),
                      set(urilib.CopyFiles(self.filesdir, self.GS_DIR)))

    # Test ListFiles, GS version.
    self.assertEquals(set(shallow_gs_files),
                      set(urilib.ListFiles(self.GS_DIR)))
    self.assertEquals(set(deep_gs_files),
                      set(urilib.ListFiles(self.GS_DIR, recurse=True)))

    # Test Cmp between some files.
    self.assertTrue(urilib.Cmp(self.file1_local, self.file1_gs))
    self.assertFalse(urilib.Cmp(self.file2_local, self.file1_gs))

    # Test RemoveDirContents, local version.
    urilib.RemoveDirContents(self.filesdir)
    self.assertFalse(urilib.ListFiles(self.filesdir))

    # Test CopyFiles, from GS to local.
    self.assertEquals(set(deep_local_files),
                      set(urilib.CopyFiles(self.GS_DIR, self.filesdir)))

    # Test RemoveDirContents, GS version.
    urilib.RemoveDirContents(self.GS_DIR)
    self.assertFalse(urilib.ListFiles(self.GS_DIR))


class TestUrilib(cros_test_lib.MoxTempDirTestCase):
  """Test urilib module."""

  def testExtractProtocol(self):
    tests = {'gs': ['gs://',
                    'gs://foo',
                    'gs://foo/bar'],
             'abc': ['abc://',
                     'abc://foo',
                     'abc://foo/bar'],
             None: ['foo/bar',
                    '/foo/bar',
                    '://garbage/path']}

    for protocol in tests:
      for uri in tests[protocol]:
        self.assertEquals(protocol, urilib.ExtractProtocol(uri))

  def testGetUriType(self):
    tests = {'gs': ['gs://',
                    'gs://foo',
                    'gs://foo/bar'],
             'abc': ['abc://',
                     'abc://foo',
                     'abc://foo/bar'],
             'file': ['foo/bar',
                      '/foo/bar',
                      '://garbage/path',
                      '/cnsfoo/bar']}

    for uri_type in tests:
      for uri in tests[uri_type]:
        self.assertEquals(uri_type, urilib.GetUriType(uri))

  def testSplitURI(self):
    tests = [
        ['gs', 'foo', 'gs://foo'],
        ['gs', 'foo/bar', 'gs://foo/bar'],
        ['file', '/foo/bar', 'file:///foo/bar'],
        [None, '/foo/bar', '/foo/bar'],
    ]

    for test in tests:
      uri = test[2]
      protocol, path = urilib.SplitURI(uri)
      self.assertEquals(test[0], protocol)
      self.assertEquals(test[1], path)

  def testIsGsURI(self):
    tests_true = ('gs://',
                  'gs://foo',
                  'gs://foo/bar')
    for test in tests_true:
      self.assertTrue(urilib.IsGsURI(test))

    tests_false = ('gsfoo/bar',
                   'gs/foo/bar',
                   'gs',
                   '/foo/bar',
                   '/gs',
                   '/gs/foo/bar'
                   'file://foo/bar',
                   'http://foo/bar')
    for test in tests_false:
      self.assertFalse(urilib.IsGsURI(test))

  def testIsFileURI(self):
    tests_true = ('file://',
                  'file://foo/bar',
                  'file:///foo/bar',
                  '/foo/bar',
                  'foo/bar',
                  'foo',
                  '')
    for test in tests_true:
      self.assertTrue(urilib.IsFileURI(test))

    tests_false = ('gs://',
                   'foo://',
                   'gs://foo/bar')
    for test in tests_false:
      self.assertFalse(urilib.IsFileURI(test))

  def testIsHttpURI(self):
    tests_true = ('http://',
                  'http://foo',
                  'http://foo/bar')
    for test in tests_true:
      self.assertTrue(urilib.IsHttpURI(test))

    tests_https_true = ('https://',
                        'https://foo',
                        'https://foo/bar')
    for test in tests_https_true:
      self.assertTrue(urilib.IsHttpURI(test, https_ok=True))
    for test in tests_https_true:
      self.assertFalse(urilib.IsHttpURI(test))

    tests_false = ('httpfoo/bar',
                   'http/foo/bar',
                   'http',
                   '/foo/bar',
                   '/http',
                   '/http/foo/bar'
                   'file:///foo/bar',
                   'gs://foo/bar')
    for test in tests_false:
      self.assertFalse(urilib.IsHttpURI(test))

  def testIsHttpsURI(self):
    tests_true = ('https://',
                  'https://foo',
                  'https://foo/bar')
    for test in tests_true:
      self.assertTrue(urilib.IsHttpsURI(test))

    tests_false = ('http://',
                   'http://foo',
                   'http://foo/bar',
                   'httpfoo/bar',
                   'http/foo/bar',
                   'http',
                   '/foo/bar',
                   '/http',
                   '/http/foo/bar'
                   'file:///foo/bar',
                   'gs://foo/bar')
    for test in tests_false:
      self.assertFalse(urilib.IsHttpsURI(test))

  def testMD5Sum(self):
    gs_path = 'gs://bucket/some/path'
    local_path = '/some/local/path'
    http_path = 'http://host.domain/some/path'

    self.mox.StubOutWithMock(gslib, 'MD5Sum')
    self.mox.StubOutWithMock(filelib, 'MD5Sum')

    # Set up the test replay script.
    # Run 1, GS.
    gslib.MD5Sum(gs_path).AndReturn('TheResult')
    # Run 3, local file.
    filelib.MD5Sum(local_path).AndReturn('TheResult')
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertEquals('TheResult', urilib.MD5Sum(gs_path))
    self.assertEquals('TheResult', urilib.MD5Sum(local_path))
    self.assertRaises(urilib.NotSupportedForType, urilib.MD5Sum, http_path)
    self.mox.VerifyAll()

  def testCmp(self):
    gs_path = 'gs://bucket/some/path'
    local_path = '/some/local/path'
    http_path = 'http://host.domain/some/path'

    result = 'TheResult'

    self.mox.StubOutWithMock(gslib, 'Cmp')
    self.mox.StubOutWithMock(filelib, 'Cmp')

    # Set up the test replay script.
    # Run 1, two local files.
    filelib.Cmp(local_path, local_path + '.1').AndReturn(result)
    # Run 2, local and GS.
    gslib.Cmp(local_path, gs_path).AndReturn(result)
    # Run 4, GS and GS
    gslib.Cmp(gs_path, gs_path + '.1').AndReturn(result)
    # Run 7, local and HTTP
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertEquals(result, urilib.Cmp(local_path, local_path + '.1'))
    self.assertEquals(result, urilib.Cmp(local_path, gs_path))
    self.assertEquals(result, urilib.Cmp(gs_path, gs_path + '.1'))
    self.assertRaises(urilib.NotSupportedBetweenTypes, urilib.Cmp,
                      local_path, http_path)
    self.mox.VerifyAll()

  @cros_test_lib.NetworkTest()
  def testURLRetrieve(self):
    good_url = 'https://codereview.chromium.org/download/issue11731004_1_2.diff'
    bad_domain_url = 'http://notarealdomainireallyhope.com/some/path'
    bad_path_url = 'https://dl.google.com/dl/edgedl/x/y/z/a/b/c/foobar'
    local_path = os.path.join(self.tempdir, 'downloaded_file')
    bad_local_path = '/tmp/a/b/c/d/x/y/z/foobar'

    git_index1 = 'e6c0d72a5122171deb4c458991d1c7547f31a2f0'
    git_index2 = '3d0f7d3edfd8146031e66dc3f45926920d3ded78'
    expected_contents = """Index: LICENSE
diff --git a/LICENSE b/LICENSE
index %s..%s 100644
--- a/LICENSE
+++ b/LICENSE
@@ -1,4 +1,4 @@
-// Copyright (c) 2012 The Chromium Authors. All rights reserved.
+// Copyright (c) 2013 The Chromium Authors. All rights reserved.
 //
 // Redistribution and use in source and binary forms, with or without
 // modification, are permitted provided that the following conditions are
""" % (git_index1, git_index2)

    self.assertRaises(urilib.MissingURLError, urilib.URLRetrieve,
                      bad_path_url, local_path)
    self.assertRaises(urilib.MissingURLError, urilib.URLRetrieve,
                      bad_domain_url, local_path)

    urilib.URLRetrieve(good_url, local_path)
    with open(local_path, 'r') as f:
      actual_contents = f.read()
    self.assertEqual(expected_contents, actual_contents)

    self.assertRaises(IOError, urilib.URLRetrieve, good_url, bad_local_path)

  def testCopy(self):
    gs_path = 'gs://bucket/some/path'
    local_path = '/some/local/path'
    http_path = 'http://host.domain/some/path'

    result = 'TheResult'

    self.mox.StubOutWithMock(gslib, 'Copy')
    self.mox.StubOutWithMock(filelib, 'Copy')
    self.mox.StubOutWithMock(urilib, 'URLRetrieve')

    # Set up the test replay script.
    # Run 1, two local files.
    filelib.Copy(local_path, local_path + '.1').AndReturn(result)
    # Run 2, local and GS.
    gslib.Copy(local_path, gs_path).AndReturn(result)
    # Run 4, GS and GS
    gslib.Copy(gs_path, gs_path + '.1').AndReturn(result)
    # Run 7, HTTP and local
    urilib.URLRetrieve(http_path, local_path).AndReturn(result)
    # Run 8, local and HTTP
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertEquals(result, urilib.Copy(local_path, local_path + '.1'))
    self.assertEquals(result, urilib.Copy(local_path, gs_path))
    self.assertEquals(result, urilib.Copy(gs_path, gs_path + '.1'))
    self.assertEquals(result, urilib.Copy(http_path, local_path))
    self.assertRaises(urilib.NotSupportedBetweenTypes, urilib.Copy,
                      local_path, http_path)
    self.mox.VerifyAll()

  def testRemove(self):
    gs_path = 'gs://bucket/some/path'
    local_path = '/some/local/path'
    http_path = 'http://host.domain/some/path'

    self.mox.StubOutWithMock(gslib, 'Remove')
    self.mox.StubOutWithMock(filelib, 'Remove')

    # Set up the test replay script.
    # Run 1, two local files.
    filelib.Remove(local_path, local_path + '.1')
    # Run 2, local and GS.
    gslib.Remove(local_path, gs_path, ignore_no_match=True)
    # Run 4, GS and GS
    gslib.Remove(gs_path, gs_path + '.1',
                 ignore_no_match=True, recurse=True)
    # Run 7, local and HTTP
    self.mox.ReplayAll()

    # Run the test verification.
    urilib.Remove(local_path, local_path + '.1')
    urilib.Remove(local_path, gs_path, ignore_no_match=True)
    urilib.Remove(gs_path, gs_path + '.1', ignore_no_match=True, recurse=True)
    self.assertRaises(urilib.NotSupportedForTypes, urilib.Remove,
                      local_path, http_path)
    self.mox.VerifyAll()

  def testSize(self):
    gs_path = 'gs://bucket/some/path'
    local_path = '/some/local/path'
    http_path = 'http://host.domain/some/path'
    ftp_path = 'ftp://host.domain/some/path'

    result = 100
    http_response = FakeHttpResponse(200, {'Content-Length': str(result)})

    self.mox.StubOutWithMock(gslib, 'FileSize')
    self.mox.StubOutWithMock(filelib, 'Size')
    self.mox.StubOutWithMock(urilib.urllib2, 'urlopen')

    # Set up the test replay script.
    # Run 1, local.
    filelib.Size(local_path).AndReturn(result)
    # Run 2, GS.
    gslib.FileSize(gs_path).AndReturn(result)
    # Run 4, HTTP.
    urilib.urllib2.urlopen(http_path).AndReturn(http_response)
    # Run 5, FTP.
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertEquals(result, urilib.Size(local_path))
    self.assertEquals(result, urilib.Size(gs_path))
    self.assertEquals(result, urilib.Size(http_path))
    self.assertRaises(urilib.NotSupportedForType, urilib.Size, ftp_path)
    self.mox.VerifyAll()

  def testExists(self):
    gs_path = 'gs://bucket/some/path'
    local_path = '/some/local/path'
    http_path = 'http://host.domain/some/path'
    ftp_path = 'ftp://host.domain/some/path'

    result = 'TheResult'

    self.mox.StubOutWithMock(gslib, 'Exists')
    self.mox.StubOutWithMock(filelib, 'Exists')
    self.mox.StubOutWithMock(urilib.urllib2, 'urlopen')

    # Set up the test replay script.
    # Run 1, local, as_dir=False
    filelib.Exists(local_path, as_dir=False).AndReturn(result)
    # Run 2, GS, as_dir=False.
    gslib.Exists(gs_path).AndReturn(result)
    # Run 3, GS, as_dir=True.
    # Run 6, HTTP, as_dir=False, code=200.
    urilib.urllib2.urlopen(http_path).AndReturn(FakeHttpResponse(200))
    # Run 7, HTTP, as_dir=False, code=404.
    urilib.urllib2.urlopen(http_path).AndReturn(FakeHttpResponse(404))
    # Run 8, HTTP, as_dir=False, HTTPError.
    urilib.urllib2.urlopen(http_path).AndRaise(
        urilib.urllib2.HTTPError('url', 404, 'msg', None, None))
    # Run 9, HTTP, as_dir=True.
    # Run 10, FTP, as_dir=False.
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertEquals(result, urilib.Exists(local_path))
    self.assertEquals(result, urilib.Exists(gs_path))
    self.assertEquals(False, urilib.Exists(gs_path, as_dir=True))
    self.assertTrue(urilib.Exists(http_path))
    self.assertFalse(urilib.Exists(http_path))
    self.assertFalse(urilib.Exists(http_path))
    self.assertRaises(urilib.NotSupportedForType,
                      urilib.Exists, http_path, as_dir=True)
    self.assertRaises(urilib.NotSupportedForType, urilib.Exists, ftp_path)
    self.mox.VerifyAll()

  def testListFiles(self):
    gs_path = 'gs://bucket/some/path'
    local_path = '/some/local/path'
    http_path = 'http://host.domain/some/path'

    result = 'TheResult'
    patt = 'TheFilePattern'

    self.mox.StubOutWithMock(gslib, 'ListFiles')
    self.mox.StubOutWithMock(filelib, 'ListFiles')

    # Set up the test replay script.
    # Run 1, local.
    filelib.ListFiles(
        local_path, recurse=True, filepattern=None,
        sort=False).AndReturn(result)
    # Run 2, GS.
    gslib.ListFiles(
        gs_path, recurse=False, filepattern=patt, sort=True).AndReturn(result)
    # Run 4, HTTP.
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertEquals(result, urilib.ListFiles(local_path, recurse=True))
    self.assertEquals(result, urilib.ListFiles(gs_path, filepattern=patt,
                                               sort=True))
    self.assertRaises(urilib.NotSupportedForType, urilib.ListFiles, http_path)
    self.mox.VerifyAll()
