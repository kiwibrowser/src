# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for upload_prebuilts.py."""

from __future__ import print_function

import copy
import mock
import os
import multiprocessing
import tempfile

from chromite.scripts import upload_prebuilts as prebuilt
from chromite.lib import cros_test_lib
from chromite.lib import gs
from chromite.lib import binpkg
from chromite.lib import osutils
from chromite.lib import parallel_unittest
from chromite.lib import portage_util


# pylint: disable=E1120,W0212,R0904
PUBLIC_PACKAGES = [{'CPV': 'gtk+/public1', 'SHA1': '1', 'MTIME': '1'},
                   {'CPV': 'gtk+/public2', 'SHA1': '2',
                    'PATH': 'gtk+/foo.tgz', 'MTIME': '2'}]
PRIVATE_PACKAGES = [{'CPV': 'private', 'SHA1': '3', 'MTIME': '3'}]


def SimplePackageIndex(header=True, packages=True):
  pkgindex = binpkg.PackageIndex()
  if header:
    pkgindex.header['URI'] = 'gs://example'
  if packages:
    pkgindex.packages = copy.deepcopy(PUBLIC_PACKAGES + PRIVATE_PACKAGES)
  return pkgindex


class TestUpdateFile(cros_test_lib.TempDirTestCase):
  """Tests for the UpdateLocalFile function."""

  def setUp(self):
    self.contents_str = [
        '# comment that should be skipped',
        'PKGDIR="/var/lib/portage/pkgs"',
        'PORTAGE_BINHOST="http://no.thanks.com"',
        'portage portage-20100310.tar.bz2',
        'COMPILE_FLAGS="some_value=some_other"',
    ]
    self.version_file = os.path.join(self.tempdir, 'version')
    osutils.WriteFile(self.version_file, '\n'.join(self.contents_str))

  def _read_version_file(self, version_file=None):
    """Read the contents of self.version_file and return as a list."""
    if not version_file:
      version_file = self.version_file

    version_fh = open(version_file)
    try:
      return [line.strip() for line in version_fh.readlines()]
    finally:
      version_fh.close()

  def _verify_key_pair(self, key, val):
    file_contents = self._read_version_file()
    # ensure key for verify is wrapped on quotes
    if '"' not in val:
      val = '"%s"' % val
    for entry in file_contents:
      if '=' not in entry:
        continue
      file_key, file_val = entry.split('=')
      if file_key == key:
        if val == file_val:
          break
    else:
      self.fail('Could not find "%s=%s" in version file' % (key, val))

  def testAddVariableThatDoesNotExist(self):
    """Add in a new variable that was no present in the file."""
    key = 'PORTAGE_BINHOST'
    value = '1234567'
    prebuilt.UpdateLocalFile(self.version_file, value)
    print(self.version_file)
    self._read_version_file()
    self._verify_key_pair(key, value)
    print(self.version_file)

  def testUpdateVariable(self):
    """Test updating a variable that already exists."""
    key, val = self.contents_str[2].split('=')
    new_val = 'test_update'
    self._verify_key_pair(key, val)
    prebuilt.UpdateLocalFile(self.version_file, new_val)
    self._verify_key_pair(key, new_val)

  def testUpdateNonExistentFile(self):
    key = 'PORTAGE_BINHOST'
    value = '1234567'
    non_existent_file = tempfile.mktemp()
    try:
      prebuilt.UpdateLocalFile(non_existent_file, value)
      file_contents = self._read_version_file(non_existent_file)
      self.assertEqual(file_contents, ['%s="%s"' % (key, value)])
    finally:
      if os.path.exists(non_existent_file):
        os.remove(non_existent_file)


class TestPrebuilt(cros_test_lib.MockTestCase):
  """Tests for Prebuilt logic."""

  def setUp(self):
    self._base_local_path = '/b/cbuild/build/chroot/build/x86-dogfood/'
    self._gs_bucket_path = 'gs://chromeos-prebuilt/host/version'
    self._local_path = os.path.join(self._base_local_path, 'public1.tbz2')

  def testGenerateUploadDict(self):
    self.PatchObject(prebuilt.os.path, 'exists', return_true=True)
    pkgs = [{'CPV': 'public1'}]
    result = prebuilt.GenerateUploadDict(self._base_local_path,
                                         self._gs_bucket_path, pkgs)
    expected = {self._local_path: self._gs_bucket_path + '/public1.tbz2', }
    self.assertEqual(result, expected)

  def testGenerateUploadDictWithDebug(self):
    self.PatchObject(prebuilt.os.path, 'exists', return_true=True)
    pkgs = [{'CPV': 'public1', 'DEBUG_SYMBOLS': 'yes'}]
    result = prebuilt.GenerateUploadDict(self._base_local_path,
                                         self._gs_bucket_path, pkgs)
    expected = {self._local_path: self._gs_bucket_path + '/public1.tbz2',
                self._local_path.replace('.tbz2', '.debug.tbz2'):
                self._gs_bucket_path + '/public1.debug.tbz2'}
    self.assertEqual(result, expected)

  def testDeterminePrebuiltConfHost(self):
    """Test that the host prebuilt path comes back properly."""
    expected_path = os.path.join(prebuilt._PREBUILT_MAKE_CONF['amd64'])
    self.assertEqual(prebuilt.DeterminePrebuiltConfFile('fake_path', 'amd64'),
                     expected_path)


class TestPkgIndex(cros_test_lib.TestCase):
  """Helper for tests that update the Packages index file."""

  def setUp(self):
    self.db = {}
    self.pkgindex = SimplePackageIndex()
    self.empty = SimplePackageIndex(packages=False)

  def assertURIs(self, uris):
    """Verify that the duplicate DB has the specified URLs."""
    expected = [v.uri for _, v in sorted(self.db.items())]
    self.assertEqual(expected, uris)


class TestPackagesFileFiltering(TestPkgIndex):
  """Tests for Packages filtering behavior."""

  def testFilterPkgIndex(self):
    """Test filtering out of private packages."""
    self.pkgindex.RemoveFilteredPackages(lambda pkg: pkg in PRIVATE_PACKAGES)
    self.assertEqual(self.pkgindex.packages, PUBLIC_PACKAGES)
    self.assertEqual(self.pkgindex.modified, True)


class TestPopulateDuplicateDB(TestPkgIndex):
  """Tests for the _PopulateDuplicateDB function."""

  def testEmptyIndex(self):
    """Test population of the duplicate DB with an empty index."""
    self.empty._PopulateDuplicateDB(self.db, 0)
    self.assertEqual(self.db, {})

  def testNormalIndex(self):
    """Test population of the duplicate DB with a full index."""
    self.pkgindex._PopulateDuplicateDB(self.db, 0)
    self.assertURIs(['gs://example/gtk+/public1.tbz2',
                     'gs://example/gtk+/foo.tgz',
                     'gs://example/private.tbz2'])

  def testMissingSHA1(self):
    """Test population of the duplicate DB with a missing SHA1."""
    del self.pkgindex.packages[0]['SHA1']
    self.pkgindex._PopulateDuplicateDB(self.db, 0)
    self.assertURIs(['gs://example/gtk+/foo.tgz',
                     'gs://example/private.tbz2'])

  def testFailedPopulate(self):
    """Test failure conditions for the populate method."""
    headerless = SimplePackageIndex(header=False)
    self.assertRaises(KeyError, headerless._PopulateDuplicateDB, self.db, 0)
    del self.pkgindex.packages[0]['CPV']
    self.assertRaises(KeyError, self.pkgindex._PopulateDuplicateDB, self.db, 0)


class TestResolveDuplicateUploads(cros_test_lib.MockTestCase, TestPkgIndex):
  """Tests for the ResolveDuplicateUploads function."""

  def setUp(self):
    self.PatchObject(binpkg.time, 'time', return_value=binpkg.TWO_WEEKS)
    self.db = {}
    self.dup = SimplePackageIndex()
    self.expected_pkgindex = SimplePackageIndex()

  def assertNoDuplicates(self, candidates):
    """Verify no duplicates are found with the specified candidates."""
    uploads = self.pkgindex.ResolveDuplicateUploads(candidates)
    self.assertEqual(uploads, self.pkgindex.packages)
    self.assertEqual(len(self.pkgindex.packages),
                     len(self.expected_pkgindex.packages))
    for pkg1, pkg2 in zip(self.pkgindex.packages,
                          self.expected_pkgindex.packages):
      self.assertNotEqual(pkg1['MTIME'], pkg2['MTIME'])
      del pkg1['MTIME']
      del pkg2['MTIME']
    self.assertEqual(self.pkgindex.modified, False)
    self.assertEqual(self.pkgindex.packages, self.expected_pkgindex.packages)

  def assertAllDuplicates(self, candidates):
    """Verify every package is a duplicate in the specified list."""
    for pkg in self.expected_pkgindex.packages:
      pkg.setdefault('PATH', pkg['CPV'] + '.tbz2')
    self.pkgindex.ResolveDuplicateUploads(candidates)
    self.assertEqual(self.pkgindex.packages, self.expected_pkgindex.packages)

  def testEmptyList(self):
    """If no candidates are supplied, no duplicates should be found."""
    self.assertNoDuplicates([])

  def testEmptyIndex(self):
    """If no packages are supplied, no duplicates should be found."""
    self.assertNoDuplicates([self.empty])

  def testDifferentURI(self):
    """If the URI differs, no duplicates should be found."""
    self.dup.header['URI'] = 'gs://example2'
    self.assertNoDuplicates([self.dup])

  def testUpdateModificationTime(self):
    """When duplicates are found, we should use the latest mtime."""
    for pkg in self.expected_pkgindex.packages:
      pkg['MTIME'] = '10'
    for pkg in self.dup.packages:
      pkg['MTIME'] = '4'
    self.assertAllDuplicates([self.expected_pkgindex, self.dup])

  def testCanonicalUrl(self):
    """If the URL is in a different format, we should still find duplicates."""
    self.dup.header['URI'] = gs.PUBLIC_BASE_HTTPS_URL + 'example'
    self.assertAllDuplicates([self.dup])

  def testMissingSHA1(self):
    """We should not find duplicates if there is no SHA1."""
    del self.pkgindex.packages[0]['SHA1']
    del self.expected_pkgindex.packages[0]['SHA1']
    for pkg in self.expected_pkgindex.packages[1:]:
      pkg.setdefault('PATH', pkg['CPV'] + '.tbz2')
    self.pkgindex.ResolveDuplicateUploads([self.dup])
    self.assertNotEqual(self.pkgindex.packages[0]['MTIME'],
                        self.expected_pkgindex.packages[0]['MTIME'])
    del self.pkgindex.packages[0]['MTIME']
    del self.expected_pkgindex.packages[0]['MTIME']
    self.assertEqual(self.pkgindex.packages, self.expected_pkgindex.packages)

  def testSymbolsAvailable(self):
    """If symbols are available remotely, re-use them and set DEBUG_SYMBOLS."""
    self.dup.packages[0]['DEBUG_SYMBOLS'] = 'yes'

    uploads = self.pkgindex.ResolveDuplicateUploads([self.dup])
    self.assertEqual(uploads, [])
    self.assertEqual(self.pkgindex.packages[0].get('DEBUG_SYMBOLS'), 'yes')

  def testSymbolsAvailableLocallyOnly(self):
    """If the symbols are only available locally, reupload them."""
    self.pkgindex.packages[0]['DEBUG_SYMBOLS'] = 'yes'

    uploads = self.pkgindex.ResolveDuplicateUploads([self.dup])
    self.assertEqual(uploads, [self.pkgindex.packages[0]])


class TestWritePackageIndex(cros_test_lib.MockTestCase, TestPkgIndex):
  """Tests for the WriteToNamedTemporaryFile function."""

  def testSimple(self):
    """Test simple call of WriteToNamedTemporaryFile()"""
    self.PatchObject(self.pkgindex, 'Write')
    f = self.pkgindex.WriteToNamedTemporaryFile()
    self.assertEqual(f.read(), '')


class TestUploadPrebuilt(cros_test_lib.MockTempDirTestCase):
  """Tests for the _UploadPrebuilt function."""

  def setUp(self):
    class MockTemporaryFile(object):
      """Mock out the temporary file logic."""
      def __init__(self, name):
        self.name = name
    self.pkgindex = SimplePackageIndex()
    self.PatchObject(binpkg, 'GrabLocalPackageIndex',
                     return_value=self.pkgindex)
    self.PatchObject(self.pkgindex, 'ResolveDuplicateUploads',
                     return_value=PRIVATE_PACKAGES)
    self.PatchObject(self.pkgindex, 'WriteToNamedTemporaryFile',
                     return_value=MockTemporaryFile('fake'))
    self.remote_up_mock = self.PatchObject(prebuilt, 'RemoteUpload')
    self.gs_up_mock = self.PatchObject(prebuilt, '_GsUpload')

  def testSuccessfulGsUpload(self):
    uploads = {
        os.path.join(self.tempdir, 'private.tbz2'): 'gs://foo/private.tbz2'}
    packages = list(PRIVATE_PACKAGES)
    packages.append({'CPV': 'dev-only-extras'})
    osutils.Touch(os.path.join(self.tempdir, 'dev-only-extras.tbz2'))
    self.PatchObject(prebuilt, 'GenerateUploadDict',
                     return_value=uploads)
    uploads = uploads.copy()
    uploads['fake'] = 'gs://foo/suffix/Packages'
    acl = 'public-read'
    uri = self.pkgindex.header['URI']
    uploader = prebuilt.PrebuiltUploader('gs://foo', acl, uri, [], '/', [],
                                         False, 'foo', False, 'x86-foo', [], '')
    uploader._UploadPrebuilt(self.tempdir, 'suffix')
    self.remote_up_mock.assert_called_once_with(mock.ANY, acl, uploads)
    self.assertTrue(self.gs_up_mock.called)


class TestSyncPrebuilts(cros_test_lib.MockTestCase):
  """Tests for the SyncHostPrebuilts function."""

  def setUp(self):
    self.rev_mock = self.PatchObject(prebuilt, 'RevGitFile', return_value=None)
    self.update_binhost_mock = self.PatchObject(
        prebuilt, 'UpdateBinhostConfFile', return_value=None)
    self.build_path = '/trunk'
    self.upload_location = 'gs://upload/'
    self.version = '1'
    self.binhost = 'http://prebuilt/'
    self.key = 'PORTAGE_BINHOST'
    self.upload_mock = self.PatchObject(prebuilt.PrebuiltUploader,
                                        '_UploadPrebuilt', return_value=True)

  def testSyncHostPrebuilts(self):
    board = 'x86-foo'
    target = prebuilt.BuildTarget(board, 'aura')
    slave_targets = [prebuilt.BuildTarget('x86-bar', 'aura')]
    package_path = os.path.join(self.build_path,
                                prebuilt._HOST_PACKAGES_PATH)
    url_suffix = prebuilt._REL_HOST_PATH % {
        'version': self.version,
        'host_arch': prebuilt._HOST_ARCH,
        'target': target,
    }
    packages_url_suffix = '%s/packages' % url_suffix.rstrip('/')
    url_value = '%s/%s/' % (self.binhost.rstrip('/'),
                            packages_url_suffix.rstrip('/'))
    urls = [url_value.replace('foo', 'bar'), url_value]
    binhost = ' '.join(urls)
    uploader = prebuilt.PrebuiltUploader(
        self.upload_location, 'public-read', self.binhost, [],
        self.build_path, [], False, 'foo', False, target, slave_targets,
        self.version)
    uploader.SyncHostPrebuilts(self.key, True, True)
    self.upload_mock.assert_called_once_with(package_path, packages_url_suffix)
    self.rev_mock.assert_called_once_with(
        mock.ANY, {self.key: binhost}, dryrun=False)
    self.update_binhost_mock.assert_called_once_with(
        mock.ANY, self.key, binhost)

  def testSyncBoardPrebuilts(self):
    board = 'x86-foo'
    target = prebuilt.BuildTarget(board, 'aura')
    slave_targets = [prebuilt.BuildTarget('x86-bar', 'aura')]
    board_path = os.path.join(
        self.build_path, prebuilt._BOARD_PATH % {'board': board})
    package_path = os.path.join(board_path, 'packages')
    url_suffix = prebuilt._REL_BOARD_PATH % {
        'version': self.version,
        'target': target,
    }
    packages_url_suffix = '%s/packages' % url_suffix.rstrip('/')
    url_value = '%s/%s/' % (self.binhost.rstrip('/'),
                            packages_url_suffix.rstrip('/'))
    bar_binhost = url_value.replace('foo', 'bar')
    determine_mock = self.PatchObject(prebuilt, 'DeterminePrebuiltConfFile',
                                      side_effect=('bar', 'foo'))
    self.PatchObject(prebuilt.PrebuiltUploader, '_UploadSdkTarball')
    with parallel_unittest.ParallelMock():
      multiprocessing.Process.exitcode = 0
      uploader = prebuilt.PrebuiltUploader(
          self.upload_location, 'public-read', self.binhost, [],
          self.build_path, [], False, 'foo', False, target, slave_targets,
          self.version)
      uploader.SyncBoardPrebuilts(self.key, True, True, True, None, None, None,
                                  None, None)
    determine_mock.assert_has_calls([
        mock.call(self.build_path, slave_targets[0]),
        mock.call(self.build_path, target),
    ])
    self.upload_mock.assert_called_once_with(package_path, packages_url_suffix)
    self.rev_mock.assert_has_calls([
        mock.call('bar', {self.key: bar_binhost}, dryrun=False),
        mock.call('foo', {self.key: url_value}, dryrun=False),
    ])
    self.update_binhost_mock.assert_has_calls([
        mock.call(mock.ANY, self.key, bar_binhost),
        mock.call(mock.ANY, self.key, url_value),
    ])


class TestMain(cros_test_lib.MockTestCase):
  """Tests for the main() function."""

  def testMain(self):
    """Test that the main function works."""
    options = mock.MagicMock()
    old_binhost = 'http://prebuilt/1'
    options.previous_binhost_url = [old_binhost]
    options.board = 'x86-foo'
    options.profile = None
    target = prebuilt.BuildTarget(options.board, options.profile)
    options.build_path = '/trunk'
    options.dryrun = False
    options.private = True
    options.packages = []
    options.sync_host = True
    options.git_sync = True
    options.upload_board_tarball = True
    options.prepackaged_tarball = None
    options.toolchains_overlay_tarballs = []
    options.toolchains_overlay_upload_path = ''
    options.toolchain_tarballs = []
    options.toolchain_upload_path = ''
    options.upload = 'gs://upload/'
    options.binhost_base_url = options.upload
    options.prepend_version = True
    options.set_version = None
    options.skip_upload = False
    options.filters = True
    options.key = 'PORTAGE_BINHOST'
    options.binhost_conf_dir = None
    options.sync_binhost_conf = True
    options.slave_targets = [prebuilt.BuildTarget('x86-bar', 'aura')]
    self.PatchObject(prebuilt, 'ParseOptions',
                     return_value=tuple([options, target]))
    self.PatchObject(binpkg, 'GrabRemotePackageIndex', return_value=True)
    init_mock = self.PatchObject(prebuilt.PrebuiltUploader, '__init__',
                                 return_value=None)
    expected_gs_acl_path = os.path.join('/fake_path',
                                        prebuilt._GOOGLESTORAGE_GSUTIL_FILE)
    self.PatchObject(portage_util, 'FindOverlayFile',
                     return_value=expected_gs_acl_path)
    host_mock = self.PatchObject(
        prebuilt.PrebuiltUploader, 'SyncHostPrebuilts', return_value=None)
    board_mock = self.PatchObject(
        prebuilt.PrebuiltUploader, 'SyncBoardPrebuilts', return_value=None)

    prebuilt.main([])

    init_mock.assert_called_once_with(options.upload, expected_gs_acl_path,
                                      options.upload, mock.ANY,
                                      options.build_path, options.packages,
                                      False, None, False,
                                      target, options.slave_targets,
                                      mock.ANY)
    board_mock.assert_called_once_with(
        options.key, options.git_sync,
        options.sync_binhost_conf, options.upload_board_tarball, None,
        [], '', [], '')
    host_mock.assert_called_once_with(
        options.key, options.git_sync, options.sync_binhost_conf)


class TestSdk(cros_test_lib.MockTestCase):
  """Test logic related to uploading SDK binaries"""

  def setUp(self):
    self.PatchObject(prebuilt, '_GsUpload',
                     side_effect=Exception('should not get called'))
    self.PatchObject(prebuilt, 'UpdateBinhostConfFile',
                     side_effect=Exception('should not get called'))
    self.upload_mock = self.PatchObject(prebuilt.PrebuiltUploader, '_Upload')

    self.acl = 'magic-acl'

    # All these args pretty much get ignored.  Whee.
    self.uploader = prebuilt.PrebuiltUploader(
        'gs://foo', self.acl, 'prebuilt', [], '/', [],
        False, 'foo', False, 'x86-foo', [], 'chroot-1234')

  def testSdkUpload(self, to_tarballs=(), to_upload_path=None,
                    tc_tarballs=(), tc_upload_path=None):
    """Make sure we can upload just an SDK tarball"""
    tar = 'sdk.tar.xz'
    ver = '1234'
    vtar = 'cros-sdk-%s.tar.xz' % ver

    calls = [
        mock.call('%s.Manifest' % tar,
                  'gs://chromiumos-sdk/%s.Manifest' % vtar),
        mock.call(tar, 'gs://chromiumos-sdk/%s' % vtar),
    ]
    for to in to_tarballs:
      to = to.split(':')
      calls.append(mock.call(
          to[1],
          ('gs://chromiumos-sdk/' + to_upload_path) % {'toolchains': to[0]}))
    for tc in tc_tarballs:
      tc = tc.split(':')
      calls.append(mock.call(
          tc[1], ('gs://chromiumos-sdk/' + tc_upload_path) % {'target': tc[0]}))
    calls.append(mock.call(
        mock.ANY, 'gs://chromiumos-sdk/cros-sdk-latest.conf'))

    self.uploader._UploadSdkTarball('amd64-host', '',
                                    tar, to_tarballs, to_upload_path,
                                    tc_tarballs, tc_upload_path)
    self.upload_mock.assert_has_calls(calls)

  def testBoardOverlayTarballUpload(self):
    """Make sure processing of board-specific overlay tarballs works."""
    to_tarballs = (
        ('i686-pc-linux-gnu:'
         '/some/path/built-sdk-overlay-toolchains-i686-pc-linux-gnu.tar.xz'),
        ('armv7a-cros-linux-gnueabi-arm-none-eabi:'
         '/some/path/built-sdk-overlay-toolchains-armv7a-cros-linux-gnueabi-'
         'arm-none-eabi'),
    )
    to_upload_path = (
        '1994/04/cros-sdk-overlay-toolchains-%(toolchains)s-1994.04.02.tar.xz')
    self.testSdkUpload(to_tarballs=to_tarballs, to_upload_path=to_upload_path)

  def testToolchainTarballUpload(self):
    """Make sure processing of toolchain tarballs works."""
    tc_tarballs = (
        'i686:/some/i686.tar.xz',
        'arm-none:/some/arm.tar.xz',
    )
    tc_upload_path = '1994/04/%(target)s-1994.04.02.tar.xz'
    self.testSdkUpload(tc_tarballs=tc_tarballs, tc_upload_path=tc_upload_path)
