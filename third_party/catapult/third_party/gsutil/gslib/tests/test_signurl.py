# -*- coding: utf-8 -*-
# Copyright 2014 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Tests for signurl command."""
from datetime import datetime
from datetime import timedelta
import pkgutil

import gslib.commands.signurl
from gslib.commands.signurl import HAVE_OPENSSL
from gslib.exception import CommandException
import gslib.tests.testcase as testcase
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.util import ObjectToURI as suri
from gslib.tests.util import SetBotoConfigForTest
from gslib.tests.util import unittest


# pylint: disable=protected-access
@unittest.skipUnless(HAVE_OPENSSL, 'signurl requires pyopenssl.')
@SkipForS3('Signed URLs are only supported for gs:// URLs.')
class TestSignUrl(testcase.GsUtilIntegrationTestCase):
  """Integration tests for signurl command."""

  def _GetJSONKsFile(self):
    if not hasattr(self, 'json_ks_file'):
      # Dummy json keystore constructed from test.p12.
      contents = pkgutil.get_data('gslib', 'tests/test_data/test.json')
      self.json_ks_file = self.CreateTempFile(contents=contents)
    return self.json_ks_file

  def _GetKsFile(self):
    if not hasattr(self, 'ks_file'):
      # Dummy pkcs12 keystore generated with the command

      # openssl req -new -passout pass:notasecret -batch \
      # -x509 -keyout signed_url_test.key -out signed_url_test.pem \
      # -subj '/CN=test.apps.googleusercontent.com'

      # &&

      # openssl pkcs12 -export -passin pass:notasecret \
      # -passout pass:notasecret -inkey signed_url_test.key \
      # -in signed_url_test.pem -out test.p12

      # &&

      # rm signed_url_test.key signed_url_test.pem
      contents = pkgutil.get_data('gslib', 'tests/test_data/test.p12')
      self.ks_file = self.CreateTempFile(contents=contents)
    return self.ks_file

  def testSignUrlOutputP12(self):
    """Tests signurl output of a sample object with pkcs12 keystore."""
    self._DoTestSignUrlOutput(self._GetKsFile())

  def testSignUrlOutputJSON(self):
    """Tests signurl output of a sample object with json keystore."""
    self._DoTestSignUrlOutput(self._GetJSONKsFile(), json_keystore=True)

  def _DoTestSignUrlOutput(self, ks_file, json_keystore=False):
    """Tests signurl output of a sample object."""

    bucket_uri = self.CreateBucket()
    object_uri = self.CreateObject(bucket_uri=bucket_uri, contents='z')
    cmd_base = ['signurl'] if json_keystore else ['signurl', '-p', 'notasecret']
    stdout = self.RunGsUtil(cmd_base + ['-m', 'PUT', ks_file, suri(object_uri)],
                            return_stdout=True)

    self.assertIn(
        'x-goog-credential=test%40developer.gserviceaccount.com', stdout)
    self.assertIn('x-goog-expires=3600', stdout)
    self.assertIn('%2Fus-central1%2F', stdout)
    self.assertIn('\tPUT\t', stdout)

  def testSignUrlWithURLEncodeRequiredChars(self):
    objs = ['gs://example.org/test 1', 'gs://example.org/test/test 2',
            'gs://example.org/Аудиоарi хив']
    expected_partial_urls = [
        'https://storage.googleapis.com/example.org/test%201?x-goog-signature=',
        ('https://storage.googleapis.com/example.org/test/test%202'
         '?x-goog-signature='),
        ('https://storage.googleapis.com/example.org/%D0%90%D1%83%D0%B4%D0%B8%D'
         '0%BE%D0%B0%D1%80i%20%D1%85%D0%B8%D0%B2?x-goog-signature=')
        ]

    self.assertEquals(len(objs), len(expected_partial_urls))

    cmd_args = ['signurl', '-m', 'PUT', '-p', 'notasecret',
                '-r', 'us', self._GetKsFile()]
    cmd_args.extend(objs)

    stdout = self.RunGsUtil(cmd_args, return_stdout=True)

    lines = stdout.split('\n')
    # Header, signed urls, trailing newline.
    self.assertEquals(len(lines), len(objs) + 2)

    # Strip the header line to make the indices line up.
    lines = lines[1:]

    for obj, line, partial_url in zip(objs, lines, expected_partial_urls):
      self.assertIn(obj, line)
      self.assertIn(partial_url, line)
      self.assertIn('x-goog-credential=test%40developer.gserviceaccount.com',
                    line)
    self.assertIn('%2Fus%2F', stdout)

  def testSignUrlWithWildcard(self):
    objs = ['test1', 'test2', 'test3']
    obj_urls = []
    bucket = self.CreateBucket()

    for obj_name in objs:
      obj_urls.append(self.CreateObject(bucket_uri=bucket,
                                        object_name=obj_name, contents=''))

    stdout = self.RunGsUtil(['signurl', '-p',
                             'notasecret', self._GetKsFile(),
                             suri(bucket) + '/*'], return_stdout=True)

    # Header, 3 signed urls, trailing newline
    self.assertEquals(len(stdout.split('\n')), 5)

    for obj_url in obj_urls:
      self.assertIn(suri(obj_url), stdout)

  def testSignUrlOfNonObjectUrl(self):
    """Tests the signurl output of a non-existent file."""
    self.RunGsUtil(['signurl', self._GetKsFile(), 'gs://'],
                   expected_status=1, stdin='notasecret')
    self.RunGsUtil(['signurl', 'file://tmp/abc'], expected_status=1)


@unittest.skipUnless(HAVE_OPENSSL, 'signurl requires pyopenssl.')
class UnitTestSignUrl(testcase.GsUtilUnitTestCase):
  """Unit tests for the signurl command."""

  def setUp(self):
    super(UnitTestSignUrl, self).setUp()
    ks_contents = pkgutil.get_data('gslib', 'tests/test_data/test.p12')
    self.key, self.client_email = gslib.commands.signurl._ReadKeystore(
        ks_contents, 'notasecret')

    def fake_now():
      return datetime(1900, 01, 01, 00, 05, 55)

    gslib.commands.signurl._NowUTC = fake_now

  def testDurationSpec(self):
    tests = [('1h', timedelta(hours=1)),
             ('2d', timedelta(days=2)),
             ('5D', timedelta(days=5)),
             ('35s', timedelta(seconds=35)),
             ('1h', timedelta(hours=1)),
             ('33', timedelta(hours=33)),
             ('22m', timedelta(minutes=22)),
             ('3.7', None),
             ('27Z', None),
            ]

    for inp, expected in tests:
      try:
        td = gslib.commands.signurl._DurationToTimeDelta(inp)
        self.assertEquals(td, expected)
      except CommandException:
        if expected is not None:
          self.fail('{0} failed to parse')

  def testSignPut(self):
    """Tests the _GenSignedUrl function with a PUT method."""
    expected = ('https://storage.googleapis.com/test/test.txt?x-goog-signature='
                '8c4d7226d8db1c939381d421c422c8724a762250d7ab9f79eaf943f8c0d05e'
                '8eac43ef94cec44d8ab3f15d0f0243ad07bb1de470cc31099bdcbdf5555e1c'
                '41d060fca84ea64681d7a926b5e2faafac97cf1bbb1d66f0167fc7144566a2'
                '5fe2f5a708961046d6b195ba08a04b501d8b014f4fa203a5ac3d6c5effc5ea'
                '549a68c9f353b050d5ea23786845307512bc051424151d2f515391ade2304d'
                'db5bb44146ac83b89850b77ffeedbdd0682c9a1d1ae2e8dd75ad43c8263e35'
                '8592c84f879fdb8b733feec0b516963bd17990d0e89a306744ca1de6d6fbaa'
                '16ca9e82aacd1f64f2d43ae261ada2104ff481a1754b6f357d2c54fc2d127f'
                '0b0bbe0f300776d0&x-goog-algorithm=GOOG4-RSA-SHA256&x-goog-cred'
                'ential=test%40developer.gserviceaccount.com%2F19000101%2Fus-ea'
                'st%2Fstorage%2Fgoog4_request&x-goog-date=19000101T000555Z&x-go'
                'og-expires=3600&x-goog-signedheaders=host%3Bx-goog-resumable')

    duration = timedelta(seconds=3600)
    with SetBotoConfigForTest([
        ('Credentials', 'gs_host', 'storage.googleapis.com')]):
      signed_url = gslib.commands.signurl._GenSignedUrl(
          self.key,
          client_id=self.client_email,
          method='RESUMABLE',
          gcs_path='test/test.txt',
          duration=duration,
          logger=self.logger,
          region='us-east',
          content_type='')
    self.assertEquals(expected, signed_url)

  def testSignResumable(self):
    """Tests the _GenSignedUrl function with a RESUMABLE method."""
    expected = ('https://storage.googleapis.com/test/test.txt?x-goog-signature='
                '8c4d7226d8db1c939381d421c422c8724a762250d7ab9f79eaf943f8c0d05e'
                '8eac43ef94cec44d8ab3f15d0f0243ad07bb1de470cc31099bdcbdf5555e1c'
                '41d060fca84ea64681d7a926b5e2faafac97cf1bbb1d66f0167fc7144566a2'
                '5fe2f5a708961046d6b195ba08a04b501d8b014f4fa203a5ac3d6c5effc5ea'
                '549a68c9f353b050d5ea23786845307512bc051424151d2f515391ade2304d'
                'db5bb44146ac83b89850b77ffeedbdd0682c9a1d1ae2e8dd75ad43c8263e35'
                '8592c84f879fdb8b733feec0b516963bd17990d0e89a306744ca1de6d6fbaa'
                '16ca9e82aacd1f64f2d43ae261ada2104ff481a1754b6f357d2c54fc2d127f'
                '0b0bbe0f300776d0&x-goog-algorithm=GOOG4-RSA-SHA256&x-goog-cred'
                'ential=test%40developer.gserviceaccount.com%2F19000101%2Fus-ea'
                'st%2Fstorage%2Fgoog4_request&x-goog-date=19000101T000555Z&x-go'
                'og-expires=3600&x-goog-signedheaders=host%3Bx-goog-resumable')

    class MockLogger(object):

      def __init__(self):
        self.warning_issued = False

      def warn(self, unused_msg):
        self.warning_issued = True

    mock_logger = MockLogger()
    duration = timedelta(seconds=3600)
    with SetBotoConfigForTest([
        ('Credentials', 'gs_host', 'storage.googleapis.com')]):
      signed_url = gslib.commands.signurl._GenSignedUrl(
          self.key,
          client_id=self.client_email,
          method='RESUMABLE',
          gcs_path='test/test.txt',
          duration=duration,
          logger=mock_logger,
          region='us-east',
          content_type='')
    self.assertEquals(expected, signed_url)
    # Resumable uploads with no content-type should issue a warning.
    self.assertTrue(mock_logger.warning_issued)

    mock_logger2 = MockLogger()
    with SetBotoConfigForTest([
        ('Credentials', 'gs_host', 'storage.googleapis.com')]):
      signed_url = gslib.commands.signurl._GenSignedUrl(
          self.key,
          client_id=self.client_email,
          method='RESUMABLE',
          gcs_path='test/test.txt',
          duration=duration,
          logger=mock_logger2,
          region='us-east',
          content_type='image/jpeg')
    # No warning, since content type was included.
    self.assertFalse(mock_logger2.warning_issued)

  def testSignurlPutContentype(self):
    """Tests the _GenSignedUrl function a PUT method and content type."""
    expected = ('https://storage.googleapis.com/test/test.txt?x-goog-signature='
                '590b52cb0be515032578f372029a72dd7fc253ceb1b50b8cd5761af835b119'
                '2d461adbb16b6d292e48a5b17f9d4078327a7f1ceed7fa3e15155c1d251398'
                'a445b6346075a22bf7a6250264c983503e819eada2a3895213439ce3c9f590'
                '564e54cbca436e1bcd677c36ec33224c1a074c376953fcd7514a6a7ea93cde'
                '2dd698e9b461a697c9e4e30539cd5c3bd88172797c867955b388bc28e60d6b'
                'b8a7fb302d2eb988ef5056843c2105f177c44fc98c202ece26bf288c02ded4'
                'e7cdb85cb29584879e9765027a8ce99a4fedfda995d5e035114c5f8a8bfa94'
                '8c438b2714e4a128dc46986336573139d4009f3a75fdbbb757603cff491c0b'
                '014698ce171c9fe9&x-goog-algorithm=GOOG4-RSA-SHA256&x-goog-cred'
                'ential=test%40developer.gserviceaccount.com%2F19000101%2Feu%2F'
                'storage%2Fgoog4_request&x-goog-date=19000101T000555Z&x-goog-ex'
                'pires=3600&x-goog-signedheaders=content-type%3Bhost')

    duration = timedelta(seconds=3600)
    with SetBotoConfigForTest([
        ('Credentials', 'gs_host', 'storage.googleapis.com')]):
      signed_url = gslib.commands.signurl._GenSignedUrl(
          self.key,
          client_id=self.client_email,
          method='PUT',
          gcs_path='test/test.txt',
          duration=duration,
          logger=self.logger,
          region='eu',
          content_type='text/plain')
    self.assertEquals(expected, signed_url)

  def testSignurlGet(self):
    """Tests the _GenSignedUrl function with a GET method."""
    expected = ('https://storage.googleapis.com/test/test.txt?x-goog-signature='
                '2ed227f18d31cdf2b01da7cd4fcea45330fbfcc0dda1d327a8c27124a276ee'
                'e0de835e9cd4b0bee609d6b4b21a88a8092a9c089574a300243dde38351f0d'
                '183df007211ded41f2f0854290b995be6c9d0367d9c00976745ba27740238b'
                '0dd49fee7c41e7ed1569bbab8ffbb00a2078e904ebeeec2f8e55e93d4baba1'
                '3db5dc670b1b16183a15d5067f1584db88b3dc55e3edd3c97c0f31fec99ea4'
                'ce96ddb8235b0352c9ce5110dad1a580072d955fe9203b6701364ddd85226b'
                '55bec84ac46e48cd324fd5d8d8ad264d1aa0b7dbad3ac04b87b2a6c2c8ef95'
                '3285cbe3b431e5def84552e112899459fcb64d2d84320c06faa1e8efa26eca'
                'cce2eff41f2d2364&x-goog-algorithm=GOOG4-RSA-SHA256&x-goog-cred'
                'ential=test%40developer.gserviceaccount.com%2F19000101%2Fasia%'
                '2Fstorage%2Fgoog4_request&x-goog-date=19000101T000555Z&x-goog-'
                'expires=0&x-goog-signedheaders=host')

    duration = timedelta(seconds=0)
    with SetBotoConfigForTest([
        ('Credentials', 'gs_host', 'storage.googleapis.com')]):
      signed_url = gslib.commands.signurl._GenSignedUrl(
          self.key,
          client_id=self.client_email,
          method='GET',
          gcs_path='test/test.txt',
          duration=duration,
          logger=self.logger,
          region='asia',
          content_type='')
    self.assertEquals(expected, signed_url)

  def testSignurlGetWithJSONKey(self):
    """Tests _GenSignedUrl with a GET method and the test JSON private key."""
    expected = ('https://storage.googleapis.com/test/test.txt?x-goog-signature='
                '2ed227f18d31cdf2b01da7cd4fcea45330fbfcc0dda1d327a8c27124a276ee'
                'e0de835e9cd4b0bee609d6b4b21a88a8092a9c089574a300243dde38351f0d'
                '183df007211ded41f2f0854290b995be6c9d0367d9c00976745ba27740238b'
                '0dd49fee7c41e7ed1569bbab8ffbb00a2078e904ebeeec2f8e55e93d4baba1'
                '3db5dc670b1b16183a15d5067f1584db88b3dc55e3edd3c97c0f31fec99ea4'
                'ce96ddb8235b0352c9ce5110dad1a580072d955fe9203b6701364ddd85226b'
                '55bec84ac46e48cd324fd5d8d8ad264d1aa0b7dbad3ac04b87b2a6c2c8ef95'
                '3285cbe3b431e5def84552e112899459fcb64d2d84320c06faa1e8efa26eca'
                'cce2eff41f2d2364&x-goog-algorithm=GOOG4-RSA-SHA256&x-goog-cred'
                'ential=test%40developer.gserviceaccount.com%2F19000101%2Fasia%'
                '2Fstorage%2Fgoog4_request&x-goog-date=19000101T000555Z&x-goog-'
                'expires=0&x-goog-signedheaders=host')

    json_contents = pkgutil.get_data('gslib', 'tests/test_data/test.json')
    key, client_email = gslib.commands.signurl._ReadJSONKeystore(
        json_contents)

    duration = timedelta(seconds=0)
    with SetBotoConfigForTest([
        ('Credentials', 'gs_host', 'storage.googleapis.com')]):
      signed_url = gslib.commands.signurl._GenSignedUrl(
          key,
          client_id=client_email,
          method='GET',
          gcs_path='test/test.txt',
          duration=duration,
          logger=self.logger,
          region='asia',
          content_type='')
    self.assertEquals(expected, signed_url)
