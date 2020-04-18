# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for the androidbuild module."""

from __future__ import print_function

import apiclient
import httplib2
import mock
import oauth2client
import os
import pwd

from chromite.lib import androidbuild
from chromite.lib import cros_test_lib

TESTDATA_PATH = os.path.join(os.path.dirname(__file__), 'testdata')


class AndroidBuildTests(cros_test_lib.TestCase):
  """Tests for the androidbuild module."""

  def testFindCredentialsFile(self):
    """Checks that we can correctly locate the JSON files."""

    def RelativeToHome(fpath):
      """Builds paths relative to the home directory."""
      return os.path.relpath(fpath, os.environ['HOME'])

    authorized_user_path = os.path.join(
        TESTDATA_PATH, 'androidbuild', 'test_creds_authorized_user.json')
    service_account_path = os.path.join(
        TESTDATA_PATH, 'androidbuild', 'test_creds_service_account.json')
    nonexistent_path = os.path.join(
        TESTDATA_PATH, 'androidbuild', 'nonexistent.json')

    # Check that we find the authorized user file if it exists.
    json_path = androidbuild.FindCredentialsFile(
        homedir_json_credentials_path=RelativeToHome(authorized_user_path))
    self.assertEqual(os.path.abspath(json_path),
                     os.path.abspath(authorized_user_path))

    # Check that we fail if it can't be found.
    with self.assertRaises(androidbuild.CredentialsNotFoundError):
      json_path = androidbuild.FindCredentialsFile(
          homedir_json_credentials_path=RelativeToHome(nonexistent_path))

    # Check that the ovverride with the service account takes precedence.
    json_path = androidbuild.FindCredentialsFile(
        service_account_path,
        homedir_json_credentials_path=RelativeToHome(authorized_user_path))
    self.assertEqual(os.path.abspath(json_path),
                     os.path.abspath(service_account_path))

  def testFindCredentialsFile_PortageUsername(self):
    """Checks that we can locate the JSON files under $PORTAGE_USERNAME."""
    copy_environ = os.environ.copy()
    copy_environ['HOME'] = '/nonexistent'
    copy_environ['PORTAGE_USERNAME'] = 'fakeuser'

    fakeuser_homedir = os.path.join(TESTDATA_PATH, 'androidbuild')
    fakeuser_pwent = pwd.struct_passwd(('fakeuser', 'x', 1234, 1234,
                                        'Fake User', fakeuser_homedir,
                                        '/bin/sh'))

    service_account_name = 'test_creds_authorized_user.json'

    with mock.patch.dict(os.environ, copy_environ), \
        mock.patch.object(pwd, 'getpwnam') as mock_getpwnam:
      mock_getpwnam.return_value = fakeuser_pwent

      json_path = androidbuild.FindCredentialsFile(
          homedir_json_credentials_path=service_account_name)

      mock_getpwnam.assert_called_once_with('fakeuser')

      self.assertEqual(json_path,
                       os.path.join(fakeuser_homedir, service_account_name))

  def testLoadCredentials_ServiceAccount(self):
    """Checks that loading a service account from JSON works."""
    creds = androidbuild.LoadCredentials(os.path.join(
        TESTDATA_PATH, 'androidbuild', 'test_creds_service_account.json'))

    # In this method, for service accounts, we need to check inside private
    # attributes, so pylint: disable=protected-access

    # Check that this was indeed recognized as a service account.
    self.assertIsInstance(
        creds, oauth2client.service_account._ServiceAccountCredentials)

    # Check identification of this service account.
    self.assertEqual(
        creds._service_account_id,
        '78539106351-7rmgdpp1v64a3dpqqqpoi2qeir3tse0k'
        '.apps.googleusercontent.com')
    self.assertEqual(
        creds._service_account_email,
        '78539106351-7rmgdpp1v64a3dpqqqpoi2qeir3tse0k'
        '@developer.gserviceaccount.com')

    # Check that the test private key matches the one we expected to see.
    self.assertEqual(
        creds._private_key_id,
        'c3f27df17b54445bf56fab801323563537405f35')

    # Check that the scope has been set correctly.
    self.assertEqual(
        creds._scopes,
        'https://www.googleapis.com/auth/androidbuild.internal')

  def testLoadCredentials_AuthorizedUser(self):
    """Checks that loading authorized user credentials works."""
    creds = androidbuild.LoadCredentials(os.path.join(
        TESTDATA_PATH, 'androidbuild', 'test_creds_authorized_user.json'))

    # Check that this was indeed recognized as a service account.
    self.assertIsInstance(creds, oauth2client.client.GoogleCredentials)

    # Check identification of the client.
    self.assertEqual(
        creds.client_id,
        '78539106351-iijlq710v1ia1g6dadm3bcvb1vmiotq5'
        '.apps.googleusercontent.com')

    # Check that the client secret matches the one we expected to see.
    self.assertEqual(
        creds.client_secret,
        'W40GFz7qWi8RdtTh5dx7J_zv')

    # Authorized user credentials are scopeless.
    self.assertFalse(creds.create_scoped_required())

  def testGetApiClient(self):
    """Checks that the correct calls are used to connect the API client."""
    creds = mock.Mock()
    with mock.patch.object(httplib2, 'Http') as mock_http, \
        mock.patch.object(apiclient.discovery, 'build') as mock_build:

      # Create the ab_client.
      ab_client = androidbuild.GetApiClient(creds)

      # Check that creds were used to authorize the Http server.
      mock_http.assert_called_once_with()
      creds.authorize.assert_called_once_with(mock_http.return_value)
      mock_build.assert_called_once_with(
          'androidbuildinternal', 'v2beta1',
          http=creds.authorize.return_value)
      self.assertEqual(ab_client, mock_build.return_value)

  def testFetchArtifact(self):
    """Checks that FetchArtifact makes the correct androidbuild API calls."""
    ab_client = mock.Mock()

    # Import the 'builtins' module to be able to mock open(). The snippet below
    # will work on either Python 2 or Python 3. The linter might not be able to
    # access it, so pylint: disable=import-error
    from sys import version_info
    if version_info.major == 2:
      import __builtin__ as builtins
    else:
      import builtins

    with mock.patch.object(apiclient.http, 'MediaIoBaseDownload') \
        as mock_download, \
        mock.patch.object(builtins, 'open') as mock_open, \
        mock.patch.object(os, 'makedirs') as mock_makedirs:

      # Make sure downloader.next_chunk() will return done=True.
      mock_download.return_value.next_chunk.return_value = (None, True)

      androidbuild.FetchArtifact(
          ab_client, 'git_mnc-dev', 'mickey-userdebug', 123456,
          'abc/mickey-img-123456.zip')

      mock_makedirs.assert_called_once_with('abc')
      mock_open.assert_called_once_with('abc/mickey-img-123456.zip', 'wb')

      ab_client.buildartifact.assert_called_once_with()
      ab_client.buildartifact.return_value.get_media.assert_called_once_with(
          target='mickey-userdebug',
          buildId=123456,
          attemptId='latest',
          resourceId='abc/mickey-img-123456.zip')

      # Check that first argument is open('...', 'wb') file handle (as a
      # context manager), the second is the media_id returned from the
      # get_media(...) call and the third is a 20MiB chunk size setting.
      mock_download.assert_called_once_with(
          mock_open.return_value.__enter__.return_value,
          ab_client.buildartifact.return_value.get_media.return_value,
          chunksize=20971520)

  def testFindRecentBuilds(self):
    """Checks that FindRecentBuilds uses the correct API."""
    ab_client = mock.Mock()
    ab_client.build.return_value.list.return_value.execute.return_value = {
        'builds': [{'buildId': '122556'}, {'buildId': '123456'}]}
    build_ids = androidbuild.FindRecentBuilds(
        ab_client, 'git_mnc-dev', 'mickey-userdebug')
    ab_client.build.assert_called_once_with()
    ab_client.build.return_value.list.assert_called_once_with(
        buildType='submitted',
        branch='git_mnc-dev',
        target='mickey-userdebug')
    # Confirm that it returns them newest to oldest.
    self.assertEqual(build_ids, [123456, 122556])

  def testFindLatestGreenBuildId(self):
    """Checks that FindLatestGreenBuildId uses the correct API."""
    ab_client = mock.Mock()
    ab_client.build.return_value.list.return_value.execute.return_value = {
        'builds': [{'buildId': '122556'}, {'buildId': '123456'}]}
    build_id = androidbuild.FindLatestGreenBuildId(
        ab_client, 'git_mnc-dev', 'mickey-userdebug')
    ab_client.build.assert_called_once_with()
    ab_client.build.return_value.list.assert_called_once_with(
        buildType='submitted',
        branch='git_mnc-dev',
        target='mickey-userdebug',
        successful=True)
    self.assertEqual(build_id, 123456)

  def testSplitAbUrl(self):
    """Checks that SplitAbUrl works as expected."""

    # Full URL.
    branch, target, build_id, filepath = androidbuild.SplitAbUrl(
        'ab://android-build/git_mnc-dev/mickey-userdebug/123456/'
        'abc/mickey-img-123456.zip')
    self.assertEqual(branch, 'git_mnc-dev')
    self.assertEqual(target, 'mickey-userdebug')
    self.assertEqual(build_id, 123456)
    self.assertEqual(filepath, 'abc/mickey-img-123456.zip')

    # Without the filepath.
    branch, target, build_id, filepath = androidbuild.SplitAbUrl(
        'ab://android-build/git_mnc-dev/mickey-userdebug/123456')
    self.assertEqual(branch, 'git_mnc-dev')
    self.assertEqual(target, 'mickey-userdebug')
    self.assertEqual(build_id, 123456)
    self.assertIsNone(filepath)

    # Only branch and target.
    branch, target, build_id, filepath = androidbuild.SplitAbUrl(
        'ab://android-build/git_mnc-dev/mickey-userdebug')
    self.assertEqual(branch, 'git_mnc-dev')
    self.assertEqual(target, 'mickey-userdebug')
    self.assertIsNone(build_id)
    self.assertIsNone(filepath)

    # Less than that it's an error.
    with self.assertRaisesRegexp(ValueError, r'\btoo short\b'):
      branch, target, build_id, filepath = androidbuild.SplitAbUrl(
          'ab://android-build/git_mnc-dev')

    with self.assertRaisesRegexp(ValueError, r'\btoo short\b'):
      branch, target, build_id, filepath = androidbuild.SplitAbUrl(
          'ab://android-build')

    with self.assertRaisesRegexp(ValueError, r'\btoo short\b'):
      branch, target, build_id, filepath = androidbuild.SplitAbUrl(
          'ab://android-build/')

    with self.assertRaisesRegexp(ValueError, r'\bempty target\b'):
      branch, target, build_id, filepath = androidbuild.SplitAbUrl(
          'ab://android-build/git_mnc-dev/')

    # Non-numeric build_id.
    with self.assertRaisesRegexp(ValueError, r'\bnon-numeric build_id\b'):
      branch, target, build_id, filepath = androidbuild.SplitAbUrl(
          'ab://android-build/git_mnc-dev/mickey-userdebug/NaN/test.zip')

    # Wrong protocol.
    with self.assertRaisesRegexp(ValueError, r'\bab:// protocol\b'):
      branch, target, build_id, filepath = androidbuild.SplitAbUrl(
          'gs://android-build/git_mnc-dev/mickey-userdebug/123456/'
          'abc/mickey-img-123456.zip')

    with self.assertRaisesRegexp(ValueError, r'\bab:// protocol\b'):
      branch, target, build_id, filepath = androidbuild.SplitAbUrl(
          'http://android-build/git_mnc-dev/mickey-userdebug/123456')

    # Wrong bucket.
    with self.assertRaisesRegexp(ValueError, r'\s"android-build" bucket\b'):
      branch, target, build_id, filepath = androidbuild.SplitAbUrl(
          'ab://cros-build/git_mnc-dev/mickey-userdebug/123456')
