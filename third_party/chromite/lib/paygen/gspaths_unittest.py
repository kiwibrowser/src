# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test gspaths library."""

from __future__ import print_function

from chromite.lib import cros_test_lib
from chromite.lib.paygen import gspaths


class GsPathsDataTest(cros_test_lib.TestCase):
  """Tests for structs defined in GsPaths."""

  def testBuild(self):
    default_input = {
        'channel': 'foo-channel',
        'board': 'board-name',
        'version': '1.2.3',
    }
    default_expected = {
        'bucket': None,
        'channel': 'foo-channel',
        'board': 'board-name',
        'version': '1.2.3',
        'uri': None,
    }
    expected_str = ("Build definition (board='board-name',"
                    " version='1.2.3', channel='foo-channel')")

    build = gspaths.Build(default_input)
    self.assertEqual(build, default_expected)

    self.assertEqual(expected_str, str(build))


class GsPathsChromeosReleasesTest(cros_test_lib.TestCase):
  """Tests for gspaths.ChromeosReleases."""
  # Standard Chrome OS releases names.
  _CHROMEOS_RELEASES_BUCKET = 'chromeos-releases'
  _AU_GENERATOR_FILE_NAME = 'au-generator.zip'

  # Google Storage path, image and payload name base templates.
  _GS_BUILD_PATH_TEMPLATE = 'gs://%(bucket)s/%(channel)s/%(board)s/%(version)s'
  _IMAGE_NAME_TEMPLATE = (
      'chromeos_%(image_version)s_%(board)s_%(signed_image_type)s_'
      '%(image_channel)s_%(key)s.bin')
  _UNSIGNED_IMAGE_ARCHIVE_NAME_TEMPLATE = (
      'ChromeOS-%(unsigned_image_type)s-%(milestone)s-%(image_version)s-'
      '%(board)s.tar.xz')
  _FULL_PAYLOAD_NAME_TEMPLATE = (
      'chromeos_%(image_version)s_%(board)s_%(image_channel)s_full_%(key)s.bin-'
      '%(random_str)s.signed')
  _DELTA_PAYLOAD_NAME_TEMPLATE = (
      'chromeos_%(src_version)s-%(image_version)s_%(board)s_%(image_channel)s_'
      'delta_%(key)s.bin-%(random_str)s.signed')
  _UNSIGNED_FULL_PAYLOAD_NAME_TEMPLATE = (
      'chromeos_%(image_version)s_%(board)s_%(image_channel)s_full_'
      '%(unsigned_image_type)s.bin-%(random_str)s')
  _UNSIGNED_DELTA_PAYLOAD_NAME_TEMPLATE = (
      'chromeos_%(src_version)s-%(image_version)s_%(board)s_%(image_channel)s_'
      'delta_%(unsigned_image_type)s.bin-%(random_str)s')

  # Compound templates.
  _GS_IMAGE_PATH_TEMPLATE = '/'.join(
      (_GS_BUILD_PATH_TEMPLATE, _IMAGE_NAME_TEMPLATE))
  _GS_UNSIGNED_IMAGE_ARCHIVE_PATH_TEMPLATE = '/'.join(
      (_GS_BUILD_PATH_TEMPLATE, _UNSIGNED_IMAGE_ARCHIVE_NAME_TEMPLATE))
  _GS_PAYLOADS_PATH_TEMPLATE = '/'.join((_GS_BUILD_PATH_TEMPLATE, 'payloads'))
  _GS_PAYLOADS_SIGNING_PATH_TEMPLATE = '/'.join((_GS_BUILD_PATH_TEMPLATE,
                                                 'payloads', 'signing'))
  _GS_FULL_PAYLOAD_PATH_TEMPLATE = '/'.join(
      (_GS_PAYLOADS_PATH_TEMPLATE, _FULL_PAYLOAD_NAME_TEMPLATE))
  _GS_DELTA_PAYLOAD_PATH_TEMPLATE = '/'.join(
      (_GS_PAYLOADS_PATH_TEMPLATE, _DELTA_PAYLOAD_NAME_TEMPLATE))

  def setUp(self):
    # Shared attributes (signed + unsigned images).
    self.bucket = 'crt'
    self.channel = 'foo-channel'
    self.board = 'board-name'
    self.version = '1.2.3'

    # Signed image attributes.
    self.key = 'mp-v3'
    self.signed_image_type = 'base'

    # Unsigned (test) image attributes.
    self.milestone = 'R12'
    self.unsigned_image_type = 'test'

    # Attributes used for payload testing.
    self.src_version = '1.1.1'
    self.random_str = '1234567890'

    # Dictionaries for populating templates.
    self.image_attrs = dict(
        bucket=self.bucket,
        channel=self.channel,
        image_channel=self.channel,
        board=self.board,
        version=self.version,
        image_version=self.version,
        key=self.key,
        signed_image_type=self.signed_image_type)
    self.unsigned_image_archive_attrs = dict(
        bucket=self.bucket,
        channel=self.channel,
        image_channel=self.channel,
        board=self.board,
        version=self.version,
        image_version=self.version,
        milestone=self.milestone,
        unsigned_image_type=self.unsigned_image_type)
    self.all_attrs = dict(self.image_attrs,
                          src_version=self.src_version,
                          random_str=self.random_str,
                          **self.unsigned_image_archive_attrs)

  def _Populate(self, template, **kwargs):
    """Populates a template string with override attributes.

    This will use the default test attributes to populate a given string
    template. It will further override default field values with the values
    provided by the optional named arguments.

    Args:
      template: a string with named substitution fields
      kwargs: named attributes to override the defaults
    """
    attrs = dict(self.all_attrs, **kwargs)
    return template % attrs

  def _PopulateGsPath(self, base_path, suffix=None, **kwargs):
    """Populates a Google Storage path template w/ optional suffix.

    Args:
      base_path: a path string template with named substitution fields
      suffix: a path suffix to append to the given base path
      kwargs: named attributes to override the defaults
    """
    template = base_path
    if suffix:
      template += '/' + suffix

    return self._Populate(template, **kwargs)

  def testBuildUri(self):
    self.assertEquals(
        gspaths.ChromeosReleases.BuildUri(self.channel,
                                          self.board,
                                          self.version,
                                          bucket=self.bucket),
        self._PopulateGsPath(self._GS_BUILD_PATH_TEMPLATE))

    self.assertEquals(
        gspaths.ChromeosReleases.BuildUri(self.channel,
                                          self.board,
                                          self.version),
        self._PopulateGsPath(self._GS_BUILD_PATH_TEMPLATE,
                             bucket=self._CHROMEOS_RELEASES_BUCKET))

  def testGeneratorUri(self):
    self.assertEquals(
        gspaths.ChromeosReleases.GeneratorUri(self.channel,
                                              self.board,
                                              self.version,
                                              bucket=self.bucket),
        self._PopulateGsPath(self._GS_BUILD_PATH_TEMPLATE,
                             suffix=self._AU_GENERATOR_FILE_NAME))

    self.assertEquals(
        gspaths.ChromeosReleases.GeneratorUri(self.channel,
                                              self.board,
                                              self.version),
        self._PopulateGsPath(self._GS_BUILD_PATH_TEMPLATE,
                             suffix=self._AU_GENERATOR_FILE_NAME,
                             bucket=self._CHROMEOS_RELEASES_BUCKET))

  def testBuildPayloadsUri(self):
    self.assertEquals(
        gspaths.ChromeosReleases.BuildPayloadsUri(self.channel,
                                                  self.board,
                                                  self.version,
                                                  bucket=self.bucket),
        self._PopulateGsPath(self._GS_PAYLOADS_PATH_TEMPLATE))

    self.assertEquals(
        gspaths.ChromeosReleases.BuildPayloadsUri(self.channel,
                                                  self.board,
                                                  self.version),
        self._PopulateGsPath(self._GS_PAYLOADS_PATH_TEMPLATE,
                             bucket=self._CHROMEOS_RELEASES_BUCKET))

  def testBuildPayloadsSigningUri(self):
    self.assertEquals(
        gspaths.ChromeosReleases.BuildPayloadsSigningUri(self.channel,
                                                         self.board,
                                                         self.version,
                                                         bucket=self.bucket),
        self._PopulateGsPath(self._GS_PAYLOADS_SIGNING_PATH_TEMPLATE))

    self.assertEquals(
        gspaths.ChromeosReleases.BuildPayloadsSigningUri(self.channel,
                                                         self.board,
                                                         self.version),
        self._PopulateGsPath(self._GS_PAYLOADS_SIGNING_PATH_TEMPLATE,
                             bucket=self._CHROMEOS_RELEASES_BUCKET))

    self.assertEquals(
        gspaths.ChromeosReleases.BuildPayloadsFlagUri(
            self.channel,
            self.board,
            self.version,
            gspaths.ChromeosReleases.LOCK,
            bucket=self.bucket),
        self._PopulateGsPath(self._GS_PAYLOADS_PATH_TEMPLATE,
                             suffix='LOCK_flag'))

    # Default bucket
    self.assertEquals(
        gspaths.ChromeosReleases.BuildPayloadsFlagUri(
            self.channel,
            self.board,
            self.version,
            gspaths.ChromeosReleases.LOCK),
        self._PopulateGsPath(self._GS_PAYLOADS_PATH_TEMPLATE,
                             suffix='LOCK_flag',
                             bucket=self._CHROMEOS_RELEASES_BUCKET))

  def testImageName(self):
    self.assertEquals(
        gspaths.ChromeosReleases.ImageName(self.channel,
                                           self.board,
                                           self.version,
                                           self.key,
                                           self.signed_image_type),
        self._Populate(self._IMAGE_NAME_TEMPLATE))

  def testUnsignedImageArchiveName(self):
    self.assertEquals(
        gspaths.ChromeosReleases.UnsignedImageArchiveName(
            self.board,
            self.version,
            self.milestone,
            self.unsigned_image_type),
        self._Populate(self._UNSIGNED_IMAGE_ARCHIVE_NAME_TEMPLATE))

  def testImageUri(self):
    self.assertEquals(
        gspaths.ChromeosReleases.ImageUri(self.channel,
                                          self.board,
                                          self.version,
                                          self.key,
                                          self.signed_image_type,
                                          bucket=self.bucket),
        self._Populate(self._GS_IMAGE_PATH_TEMPLATE))

  def testUnsignedImageUri(self):
    self.assertEquals(
        gspaths.ChromeosReleases.UnsignedImageUri(
            self.channel, self.board, self.version, self.milestone,
            self.unsigned_image_type, bucket=self.bucket),
        self._Populate(self._GS_UNSIGNED_IMAGE_ARCHIVE_PATH_TEMPLATE))

  @staticmethod
  def _IncrementVersion(version, inc_amount=1):
    version_part = version.rpartition('.')
    return '.'.join((version_part[0], str(int(version_part[2]) + inc_amount)))

  def testParseImageUri(self):
    npo_version = self._IncrementVersion(self.version)
    npo_channel = 'nplusone-channel'

    basic_dict = dict(self.image_attrs)
    npo_dict = dict(self.image_attrs,
                    bucket=self._CHROMEOS_RELEASES_BUCKET,
                    image_version=npo_version,
                    image_channel=npo_channel)
    basic_dict['uri'] = uri_basic = self._GS_IMAGE_PATH_TEMPLATE % basic_dict
    npo_dict['uri'] = uri_npo = self._GS_IMAGE_PATH_TEMPLATE % npo_dict

    expected_basic = gspaths.Image(bucket=self.bucket,
                                   channel=self.channel,
                                   board=self.board,
                                   version=self.version,
                                   image_type=self.signed_image_type,
                                   key=self.key,
                                   uri=uri_basic)
    expected_basic_str = gspaths.ChromeosReleases.ImageName(
        expected_basic.channel, expected_basic.board, expected_basic.version,
        expected_basic.key, expected_basic.image_type)

    expected_npo = gspaths.Image(channel=self.channel,
                                 board=self.board,
                                 version=self.version,
                                 key=self.key,
                                 image_type=self.signed_image_type,
                                 image_channel=npo_channel,
                                 image_version=npo_version,
                                 uri=uri_npo)

    expected_npo_str = gspaths.ChromeosReleases.ImageName(
        expected_npo.image_channel, expected_npo.board,
        expected_npo.image_version, expected_npo.key, expected_npo.image_type)

    basic_image = gspaths.ChromeosReleases.ParseImageUri(uri_basic)
    self.assertEqual(basic_image, expected_basic)
    self.assertEqual(str(basic_image), expected_basic_str)

    npo_image = gspaths.ChromeosReleases.ParseImageUri(uri_npo)
    self.assertEqual(npo_image, expected_npo)
    self.assertEqual(str(npo_image), expected_npo_str)

    signer_output = ('gs://chromeos-releases/dev-channel/link/4537.7.0/'
                     'chromeos_4537.7.1_link_recovery_nplusone-channel_'
                     'mp-v4.bin.1.payload.hash.update_signer.signed.bin')

    bad_image = gspaths.ChromeosReleases.ParseImageUri(signer_output)
    self.assertEqual(bad_image, None)

  def testParseUnsignedImageUri(self):
    attr_dict = dict(self.unsigned_image_archive_attrs)
    attr_dict['uri'] = uri = (
        self._GS_UNSIGNED_IMAGE_ARCHIVE_PATH_TEMPLATE % attr_dict)

    expected = gspaths.UnsignedImageArchive(bucket=self.bucket,
                                            channel=self.channel,
                                            board=self.board,
                                            version=self.version,
                                            milestone=self.milestone,
                                            image_type=self.unsigned_image_type,
                                            uri=uri)
    expected_str = gspaths.ChromeosReleases.UnsignedImageArchiveName(
        expected.board, expected.version, expected.milestone,
        expected.image_type)

    image = gspaths.ChromeosReleases.ParseUnsignedImageUri(uri)
    self.assertEqual(image, expected)
    self.assertEqual(str(image), expected_str)

  def testPayloadNamePreset(self):
    full = gspaths.ChromeosReleases.PayloadName(channel=self.channel,
                                                board=self.board,
                                                version=self.version,
                                                key=self.key,
                                                random_str=self.random_str)

    delta = gspaths.ChromeosReleases.PayloadName(channel=self.channel,
                                                 board=self.board,
                                                 version=self.version,
                                                 key=self.key,
                                                 src_version=self.src_version,
                                                 random_str=self.random_str)

    full_unsigned = gspaths.ChromeosReleases.PayloadName(
        channel=self.channel,
        board=self.board,
        version=self.version,
        random_str=self.random_str,
        unsigned_image_type=self.unsigned_image_type)

    delta_unsigned = gspaths.ChromeosReleases.PayloadName(
        channel=self.channel,
        board=self.board,
        version=self.version,
        src_version=self.src_version,
        random_str=self.random_str,
        unsigned_image_type=self.unsigned_image_type)

    self.assertEqual(full, self._Populate(self._FULL_PAYLOAD_NAME_TEMPLATE))
    self.assertEqual(delta, self._Populate(self._DELTA_PAYLOAD_NAME_TEMPLATE))
    self.assertEqual(full_unsigned,
                     self._Populate(self._UNSIGNED_FULL_PAYLOAD_NAME_TEMPLATE))
    self.assertEqual(delta_unsigned,
                     self._Populate(self._UNSIGNED_DELTA_PAYLOAD_NAME_TEMPLATE))

  def testPayloadNameRandom(self):
    full = gspaths.ChromeosReleases.PayloadName(channel=self.channel,
                                                board=self.board,
                                                version=self.version,
                                                key=self.key)

    delta = gspaths.ChromeosReleases.PayloadName(channel=self.channel,
                                                 board=self.board,
                                                 version=self.version,
                                                 key=self.key,
                                                 src_version=self.src_version)

    # Isolate the actual random string, transplant it in the reference template.
    full_random_str = full.split('-')[-1].partition('.')[0]
    self.assertEqual(
        full,
        self._Populate(self._FULL_PAYLOAD_NAME_TEMPLATE,
                       random_str=full_random_str))
    delta_random_str = delta.split('-')[-1].partition('.')[0]
    self.assertEqual(
        delta,
        self._Populate(self._DELTA_PAYLOAD_NAME_TEMPLATE,
                       random_str=delta_random_str))

  def testPayloadUri(self):
    test_random_channel = 'test_random_channel'
    test_max_version = '4.5.6'
    test_min_version = '0.12.1.0'

    min_full = gspaths.ChromeosReleases.PayloadUri(
        channel=self.channel,
        board=self.board,
        version=self.version,
        random_str=self.random_str,
        key=self.key)

    self.assertEqual(
        min_full,
        self._Populate(self._GS_FULL_PAYLOAD_PATH_TEMPLATE,
                       bucket=self._CHROMEOS_RELEASES_BUCKET))

    max_full = gspaths.ChromeosReleases.PayloadUri(
        channel=self.channel,
        board=self.board,
        version=self.version,
        random_str=self.random_str,
        key=self.key,
        image_channel=test_random_channel,
        image_version=test_max_version,
        bucket=self.bucket)

    self.assertEqual(
        max_full,
        self._Populate(self._GS_FULL_PAYLOAD_PATH_TEMPLATE,
                       image_channel=test_random_channel,
                       image_version=test_max_version))

    min_delta = gspaths.ChromeosReleases.PayloadUri(
        channel=self.channel,
        board=self.board,
        version=self.version,
        random_str=self.random_str,
        key=self.key,
        src_version=test_min_version)

    self.assertEqual(
        min_delta,
        self._Populate(self._GS_DELTA_PAYLOAD_PATH_TEMPLATE,
                       bucket=self._CHROMEOS_RELEASES_BUCKET,
                       src_version=test_min_version))

    max_delta = gspaths.ChromeosReleases.PayloadUri(
        channel=self.channel,
        board=self.board,
        version=self.version,
        random_str=self.random_str,
        key=self.key,
        image_channel=test_random_channel,
        image_version=test_max_version,
        src_version=test_min_version,
        bucket=self.bucket)

    self.assertEqual(
        max_delta,
        self._Populate(self._GS_DELTA_PAYLOAD_PATH_TEMPLATE,
                       src_version=test_min_version,
                       image_version=test_max_version,
                       image_channel=test_random_channel))

  def testParsePayloadUri(self):
    """Test gsutils.ChromeosReleases.ParsePayloadUri()."""

    image_version = '1.2.4'

    full_uri = self._Populate(self._GS_FULL_PAYLOAD_PATH_TEMPLATE,
                              bucket='chromeos-releases')

    delta_uri = self._Populate(self._GS_DELTA_PAYLOAD_PATH_TEMPLATE,
                               bucket='chromeos-releases')

    max_full_uri = self._Populate(self._GS_FULL_PAYLOAD_PATH_TEMPLATE,
                                  image_channel='image-channel',
                                  image_version=image_version)

    max_delta_uri = self._Populate(self._GS_DELTA_PAYLOAD_PATH_TEMPLATE,
                                   image_channel='image-channel',
                                   image_version=image_version)

    self.assertDictEqual(
        gspaths.ChromeosReleases.ParsePayloadUri(full_uri),
        {
            'tgt_image': gspaths.Image(board=self.board,
                                       channel=self.channel,
                                       version=self.version,
                                       key=self.key),
            'src_image': None,
            'uri': full_uri,
            'exists': False
        })

    self.assertDictEqual(
        gspaths.ChromeosReleases.ParsePayloadUri(delta_uri),
        {
            'src_image': gspaths.Image(board=self.board,
                                       channel=self.channel,
                                       version=self.src_version),
            'tgt_image': gspaths.Image(board=self.board,
                                       channel=self.channel,
                                       version=self.version,
                                       key=self.key),
            'uri': delta_uri,
            'exists': False
        })

    self.assertDictEqual(
        gspaths.ChromeosReleases.ParsePayloadUri(max_full_uri),
        {
            'tgt_image': gspaths.Image(bucket=self.bucket,
                                       board=self.board,
                                       channel=self.channel,
                                       version=self.version,
                                       key=self.key,
                                       image_version=image_version,
                                       image_channel='image-channel'),
            'src_image': None,
            'uri': max_full_uri,
            'exists': False
        })

    self.assertDictEqual(
        gspaths.ChromeosReleases.ParsePayloadUri(max_delta_uri),
        {
            'src_image': gspaths.Image(bucket=self.bucket,
                                       board=self.board,
                                       channel=self.channel,
                                       version=self.src_version),
            'tgt_image': gspaths.Image(bucket=self.bucket,
                                       board=self.board,
                                       channel=self.channel,
                                       version=self.version,
                                       key=self.key,
                                       image_version=image_version,
                                       image_channel='image-channel'),
            'uri': max_delta_uri,
            'exists': False
        })


class GsPathsTest(cros_test_lib.TestCase):
  """Test general gspaths utilities."""

  def testVersionKey(self):
    """Test VersionKey, especially for new-style versus old-style."""

    values = ['1.2.3', '1.2.2', '2.0.0', '1.1.4',
              '1.2.3.4', '1.2.3.3', '1.2.4.4', '1.2.4.5', '1.3.3.4',
              '0.1.2.3', '0.14.45.32']

    sorted_values = sorted(values, key=gspaths.VersionKey)
    reverse_sorted_values = sorted(reversed(values), key=gspaths.VersionKey)

    expected_values = ['0.1.2.3', '0.14.45.32',
                       '1.2.3.3', '1.2.3.4', '1.2.4.4', '1.2.4.5', '1.3.3.4',
                       '1.1.4', '1.2.2', '1.2.3', '2.0.0']

    self.assertEquals(sorted_values, expected_values)
    self.assertEquals(reverse_sorted_values, expected_values)

  def testVersionGreater(self):
    """Test VersionGreater, especially for new-style versus old-style."""

    self.assertTrue(gspaths.VersionGreater('1.2.3', '1.2.2'))
    self.assertTrue(gspaths.VersionGreater('1.2.3', '1.1.4'))
    self.assertTrue(gspaths.VersionGreater('2.0.0', '1.2.3'))

    self.assertFalse(gspaths.VersionGreater('1.2.3', '1.2.3'))

    self.assertFalse(gspaths.VersionGreater('1.2.2', '1.2.3'))
    self.assertFalse(gspaths.VersionGreater('1.1.4', '1.2.3'))
    self.assertFalse(gspaths.VersionGreater('1.2.3', '2.0.0'))

    self.assertTrue(gspaths.VersionGreater('1.2.3.4', '1.2.3.3'))
    self.assertTrue(gspaths.VersionGreater('1.2.4.4', '1.2.3.4'))
    self.assertTrue(gspaths.VersionGreater('1.3.3.4', '1.2.4.5'))
    self.assertTrue(gspaths.VersionGreater('2.0.0.0', '1.2.3.4'))

    self.assertFalse(gspaths.VersionGreater('1.2.3.4', '1.2.3.4'))

    self.assertFalse(gspaths.VersionGreater('1.2.3.3', '1.2.3.4'))
    self.assertFalse(gspaths.VersionGreater('1.2.3.4', '1.2.4.4'))
    self.assertFalse(gspaths.VersionGreater('1.2.4.5', '1.3.3.4'))
    self.assertFalse(gspaths.VersionGreater('1.2.3.4', '2.0.0.0'))

    self.assertTrue(gspaths.VersionGreater('1.2.3', '1.2.3.4'))
    self.assertTrue(gspaths.VersionGreater('1.2.3', '0.1.2.3'))

    self.assertFalse(gspaths.VersionGreater('1.2.3.4', '1.2.3'))
    self.assertFalse(gspaths.VersionGreater('0.1.2.3', '1.2.3'))

  def testIsImage(self):
    a = float(3.14)
    self.assertFalse(gspaths.IsImage(a))
    b = gspaths.Image()
    self.assertTrue(gspaths.IsImage(b))

  def testIsUnsignedImageArchive(self):
    a = float(3.14)
    self.assertFalse(gspaths.IsUnsignedImageArchive(a))
    b = gspaths.UnsignedImageArchive()
    self.assertTrue(gspaths.IsUnsignedImageArchive(b))


class ImageTest(cros_test_lib.TestCase):
  """Test Image class implementation."""

  def testImage_DefaultImageType(self):
    default_image = gspaths.Image(bucket='bucket',
                                  board='board',
                                  channel='channel',
                                  version='version')
    self.assertEquals('recovery', default_image.image_type)

  def testImage_CustomImageType(self):
    custom_image_type = 'base'
    custom_image = gspaths.Image(bucket='bucket',
                                 board='board',
                                 channel='channel',
                                 version='version',
                                 image_type=custom_image_type)
    self.assertEquals(custom_image_type, custom_image.image_type)
