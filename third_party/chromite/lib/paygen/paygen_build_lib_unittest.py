# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for paygen_build_lib."""

from __future__ import print_function

import json
import os
import mock
import tarfile

from chromite.lib import config_lib_unittest
from chromite.lib import cros_test_lib
from chromite.lib import parallel

from chromite.lib.paygen import gslock
from chromite.lib.paygen import gslib
from chromite.lib.paygen import gspaths
from chromite.lib.paygen import urilib
from chromite.lib.paygen import paygen_build_lib
from chromite.lib.paygen import paygen_payload_lib


# We access a lot of protected members during testing.
# pylint: disable=protected-access


class BasePaygenBuildLibTest(cros_test_lib.MockTestCase):
  """Base class for testing PaygenBuildLib class."""

  def setUp(self):
    self.maxDiff = None

    # Clear json cache.
    paygen_build_lib.PaygenBuild._cachedPaygenJson = None

    # Mock out fetching of paygen.json from GS.
    self.mockGetJson = self.PatchObject(
        paygen_build_lib, '_GetJson',
        side_effect=lambda _: self.getParseTestPaygenJson())

    # Mock a few more to ensure there is no accidental GS interaction.
    self.mockUriList = self.PatchObject(urilib, 'ListFiles')

  def getParseTestPaygenJson(self):
    """Fetch raw parsed json from our test copy of paygen.json."""
    # TODO: Add caching, so we don't keep loading/parsing the same file.
    paygen_json_file = os.path.join(os.path.dirname(__file__),
                                    'testdata', 'paygen.json')
    with open(paygen_json_file, 'r') as fp:
      return json.load(fp)


class PaygenJsonTests(BasePaygenBuildLibTest):
  """Test cases that require mocking paygen.json fetching."""

  def testGetPaygenJsonCaching(self):
    result = paygen_build_lib.PaygenBuild.GetPaygenJson()
    self.assertEqual(len(result), 1360)
    self.mockGetJson.assert_called_once()

    # Validate caching, by proving we don't refetch.
    self.mockGetJson.reset_mock()
    result = paygen_build_lib.PaygenBuild.GetPaygenJson()
    self.assertEqual(len(result), 1360)
    self.mockGetJson.assert_not_called()

  def testGetPaygenJsonBoard(self):
    result = paygen_build_lib.PaygenBuild.GetPaygenJson('unknown')
    self.assertEqual(len(result), 0)

    result = paygen_build_lib.PaygenBuild.GetPaygenJson('auron-yuna')
    self.assertEqual(len(result), 13)

  def testGetPaygenJsonBoardChannel(self):
    result = paygen_build_lib.PaygenBuild.GetPaygenJson(
        'auron-yuna', 'unknown')
    self.assertEqual(len(result), 0)

    result = paygen_build_lib.PaygenBuild.GetPaygenJson(
        'auron-yuna', 'canary')
    self.assertEqual(len(result), 1)

    result = paygen_build_lib.PaygenBuild.GetPaygenJson(
        'auron-yuna', 'dev-channel')
    self.assertEqual(len(result), 1)

    result = paygen_build_lib.PaygenBuild.GetPaygenJson(
        'auron-yuna', 'beta-channel')
    self.assertEqual(len(result), 1)

    result = paygen_build_lib.PaygenBuild.GetPaygenJson(
        'auron-yuna', 'stable-channel')
    self.assertEqual(len(result), 10)

    # Prove that we get the same results in both channel namespaces.
    self.assertEqual(
        paygen_build_lib.PaygenBuild.GetPaygenJson(
            'auron-yuna', 'stable'),
        paygen_build_lib.PaygenBuild.GetPaygenJson(
            'auron-yuna', 'stable-channel'))

    # Look up data for a board with no payloads.
    result = paygen_build_lib.PaygenBuild.GetPaygenJson(
        'arkham', 'canary')
    self.assertEqual(len(result), 0)

  def testValidateBoardConfig(self):
    """Test ValidateBoardConfig."""
    # Test a known board works.
    paygen_build_lib.ValidateBoardConfig('x86-mario')

    # Test a known variant board works.
    paygen_build_lib.ValidateBoardConfig('auron-yuna')

    # Test an unknown board doesn't.
    with self.assertRaises(paygen_build_lib.BoardNotConfigured):
      paygen_build_lib.ValidateBoardConfig('unknown')


class BasePaygenBuildLibTestWithBuilds(BasePaygenBuildLibTest,
                                       cros_test_lib.TempDirTestCase):
  """Test PaygenBuildLib class."""

  def setUp(self):
    self.maxDiff = None

    self.prev_build = gspaths.Build(bucket='crt',
                                    channel='foo-channel',
                                    board='foo-board',
                                    version='1.0.0')

    self.prev_image = gspaths.Image(key='mp', **self.prev_build)
    self.prev_premp_image = gspaths.Image(key='premp', **self.prev_build)
    self.prev_test_image = gspaths.UnsignedImageArchive(
        image_type='test', **self.prev_build)

    self.target_build = gspaths.Build(bucket='crt',
                                      channel='foo-channel',
                                      board='foo-board',
                                      version='1.2.3')

    # Create an additional 'special' image like NPO that isn't NPO,
    # and keyed with a weird key. It should match none of the filters.
    self.special_image = gspaths.Image(bucket='crt',
                                       channel='foo-channel',
                                       board='foo-board',
                                       version='1.2.3',
                                       key='foo-key',
                                       image_channel='special-channel')

    self.basic_image = gspaths.Image(key='mp-v2', **self.target_build)
    self.premp_image = gspaths.Image(key='premp', **self.target_build)
    self.test_image = gspaths.UnsignedImageArchive(
        image_type='test', **self.target_build)

    self.mp_full_payload = gspaths.Payload(tgt_image=self.basic_image)
    self.test_full_payload = gspaths.Payload(tgt_image=self.test_image)
    self.mp_delta_payload = gspaths.Payload(tgt_image=self.basic_image,
                                            src_image=self.prev_image)
    self.test_delta_payload = gspaths.Payload(tgt_image=self.test_image,
                                              src_image=self.prev_test_image)

    self.full_payload_test = paygen_build_lib.PayloadTest(
        self.test_full_payload,
        self.target_build.channel,
        self.target_build.version)
    self.delta_payload_test = paygen_build_lib.PayloadTest(
        self.test_delta_payload)

  def _GetPaygenBuildInstance(self,
                              dry_run=False,
                              skip_delta_payloads=False):
    """Helper method to create a standard Paygen instance."""
    return paygen_build_lib.PaygenBuild(self.target_build, self.tempdir,
                                        config_lib_unittest.MockSiteConfig(),
                                        dry_run=dry_run,
                                        skip_delta_payloads=skip_delta_payloads)

  def testGetFlagURI(self):
    """Validate the helper method to create flag URIs for our current build."""
    paygen = self._GetPaygenBuildInstance()

    self.assertEqual(
        paygen._GetFlagURI(gspaths.ChromeosReleases.LOCK),
        'gs://crt/foo-channel/foo-board/1.2.3/payloads/LOCK_flag')

  def testFilterHelpers(self):
    """Test _FilterForMp helper method."""

    # All of the filter helpers should handle empty list.
    self.assertEqual(paygen_build_lib._FilterForMp([]), [])
    self.assertEqual(paygen_build_lib._FilterForPremp([]), [])
    self.assertEqual(paygen_build_lib._FilterForBasic([]), [])

    # prev_image lets us test with an 'mp' key, instead of an 'mp-v2' key.
    images = [self.basic_image, self.premp_image,
              self.special_image, self.prev_image]

    self.assertEqual(paygen_build_lib._FilterForMp(images),
                     [self.basic_image, self.prev_image])

    self.assertEqual(paygen_build_lib._FilterForPremp(images),
                     [self.premp_image])

    self.assertEqual(paygen_build_lib._FilterForBasic(images),
                     [self.basic_image, self.premp_image, self.prev_image])

  def testMapToArchive(self):
    """Test the _MapToArchive method."""
    # TODO

  def testValidateExpectedBuildImages(self):
    """Test a function that validates expected images are found on a build."""
    paygen = self._GetPaygenBuildInstance()

    # Test with basic mp image only.
    paygen._ValidateExpectedBuildImages(self.target_build, (self.basic_image,))

    # Test with basic mp and mp npo images.
    paygen._ValidateExpectedBuildImages(self.target_build, (self.basic_image,))
    # Test with basic mp and premp images.
    paygen._ValidateExpectedBuildImages(self.target_build, (self.basic_image,
                                                            self.premp_image))

    # Test with basic mp and premp images.
    paygen._ValidateExpectedBuildImages(self.target_build, (self.basic_image,
                                                            self.premp_image))

    # Test with 4 different images.
    paygen._ValidateExpectedBuildImages(self.target_build, (self.basic_image,
                                                            self.premp_image))

    # No images isn't valid.
    with self.assertRaises(paygen_build_lib.ImageMissing):
      paygen._ValidateExpectedBuildImages(self.target_build, [])

    # More than one of the same type of image should trigger BuildCorrupt
    with self.assertRaises(paygen_build_lib.BuildCorrupt):
      paygen._ValidateExpectedBuildImages(self.target_build, (self.basic_image,
                                                              self.basic_image))

    # Unexpected images should trigger BuildCorrupt
    with self.assertRaises(paygen_build_lib.BuildCorrupt):
      paygen._ValidateExpectedBuildImages(self.target_build,
                                          (self.basic_image,
                                           self.special_image))


class TestPaygenBuildLibTestGSSearch(BasePaygenBuildLibTestWithBuilds):
  """Test discovery of images."""

  def testDiscoverImages(self):
    """Test _DiscoverSignedImages."""
    paygen = self._GetPaygenBuildInstance()

    uri_base = 'gs://crt/foo-channel/foo-board/1.2.3'

    uri_basic = os.path.join(
        uri_base, 'chromeos_1.2.3_foo-board_recovery_foo-channel_mp-v3.bin')
    uri_premp = os.path.join(
        uri_base, 'chromeos_1.2.3_foo-board_recovery_foo-channel_premp.bin')

    self.mockUriList.return_value = [uri_basic, uri_premp]

    # Run the test.
    result = paygen._DiscoverSignedImages(self.target_build)

    # See if we got the results we expect.
    base_image_params = {'channel': 'foo-channel',
                         'board': 'foo-board',
                         'version': '1.2.3',
                         'bucket': 'crt'}
    expected_basic = gspaths.Image(key='mp-v3', uri=uri_basic,
                                   **base_image_params)
    expected_premp = gspaths.Image(key='premp', uri=uri_premp,
                                   **base_image_params)
    expected_result = [expected_basic, expected_premp]

    self.assertEqual(result, expected_result)

  def testDiscoverTestImages(self):
    """Test _DiscoverTestImages (success)."""
    paygen = self._GetPaygenBuildInstance()

    uri_base = 'gs://crt/foo-channel/foo-board/1.2.3'

    uri_test_archive = os.path.join(
        uri_base, 'ChromeOS-test-R12-1.2.3-foo-board.tar.xz')
    self.mockUriList.return_value = [uri_test_archive]

    # Run the test.
    result = paygen._DiscoverTestImage(self.target_build)

    expected_test_archive = gspaths.UnsignedImageArchive(
        channel='foo-channel',
        board='foo-board',
        version='1.2.3',
        bucket='crt',
        uri=uri_test_archive,
        milestone='R12',
        image_type='test')

    self.assertEqual(result, expected_test_archive)

  def testDiscoverTestImagesMultipleResults(self):
    """Test _DiscoverTestImages (fails due to multiple results)."""
    paygen = self._GetPaygenBuildInstance()
    uri_base = 'gs://crt/foo-channel/foo-board/1.2.3'

    uri_test_archive1 = os.path.join(
        uri_base, 'ChromeOS-test-R12-1.2.3-foo-board.tar.xz')
    uri_test_archive2 = os.path.join(
        uri_base, 'ChromeOS-test-R13-1.2.3-foo-board.tar.xz')
    self.mockUriList.return_value = [uri_test_archive1, uri_test_archive2]

    # Run the test.
    with self.assertRaises(paygen_build_lib.BuildCorrupt):
      paygen._DiscoverTestImage(self.target_build)

  def testDiscoverRequiredDeltasBuildToBuild(self):
    """Test _DiscoverRequiredDeltasBuildToBuild"""
    paygen = self._GetPaygenBuildInstance()

    # Test the empty case.
    results = paygen._DiscoverRequiredDeltasBuildToBuild([], [])
    self.assertItemsEqual(results, [])

    # Fully populated prev and current.
    results = paygen._DiscoverRequiredDeltasBuildToBuild(
        [self.prev_test_image],
        [self.test_image])
    self.assertItemsEqual(results, [
        gspaths.Payload(src_image=self.prev_test_image,
                        tgt_image=self.test_image),
    ])

    # Mismatch MP, PreMP
    results = paygen._DiscoverRequiredDeltasBuildToBuild(
        [self.prev_premp_image],
        [self.basic_image])
    self.assertItemsEqual(results, [])

    # It's totally legal for a build to be signed for both PreMP and MP at the
    # same time. If that happens we generate:
    # MP -> MP, PreMP -> PreMP, test -> test.
    results = paygen._DiscoverRequiredDeltasBuildToBuild(
        [self.prev_image, self.prev_premp_image, self.prev_test_image],
        [self.basic_image, self.premp_image, self.test_image])
    self.assertItemsEqual(results, [
        gspaths.Payload(src_image=self.prev_image,
                        tgt_image=self.basic_image),
        gspaths.Payload(src_image=self.prev_premp_image,
                        tgt_image=self.premp_image),
        gspaths.Payload(src_image=self.prev_test_image,
                        tgt_image=self.test_image),
    ])


class MockImageDiscoveryHelper(BasePaygenBuildLibTest):
  """Tests DiscoverRequiredPayloads using a fixed paygen.json from testdata."""

  def setUp(self):
    # We want to use a dict as dict key, but can't.
    # Use a list of key, value tuples.
    self.signedResults = []
    self.testResults = []

    self.PatchObject(
        paygen_build_lib.PaygenBuild, '_DiscoverSignedImages',
        side_effect=self._DiscoverSignedImages)
    self.PatchObject(
        paygen_build_lib.PaygenBuild, '_DiscoverTestImage',
        side_effect=self._DiscoverTestImage)

  def _DiscoverSignedImages(self, build):
    for b, images in self.signedResults:
      if build == b:
        return images
    raise paygen_build_lib.ImageMissing(build)

  def _DiscoverTestImage(self, build):
    for b, images in self.testResults:
      if build == b:
        return images
    raise paygen_build_lib.ImageMissing()

  def addSignedImage(self, build, key='mp'):
    images = []
    for i in xrange(len(self.signedResults)):
      if build == self.signedResults[i][0]:
        images = self.signedResults[i][1]
        self.signedResults.pop(i)
        break

    image = gspaths.Image(key=key, image_version=build.version,
                          image_type='recovery', **build)
    images.append(image)
    self.signedResults.append((build, images))
    return image

  def addTestImage(self, build):
    for i in xrange(len(self.testResults)):
      if build == self.testResults[i][0]:
        self.testResults.pop(i)
        break

    image = gspaths.UnsignedImageArchive(**build)
    self.testResults.append((build, image))
    return image


class TestPaygenBuildLibDiscoverRequiredPayloads(MockImageDiscoveryHelper,
                                                 cros_test_lib.TempDirTestCase):
  """Test deciding what payloads to generate."""

  def _GetPaygenBuildInstance(self, build, skip_delta_payloads=False):
    """Helper method to create a standard Paygen instance."""
    return paygen_build_lib.PaygenBuild(
        build, self.tempdir,
        config_lib_unittest.MockSiteConfig(),
        skip_delta_payloads=skip_delta_payloads)

  def testImagesNotReady(self):
    """See that we do the right thing if there are no images for the build."""
    target_build = gspaths.Build(bucket='crt',
                                 channel='canary-channel',
                                 board='auron-yuna',
                                 version='9999.0.0')

    paygen = self._GetPaygenBuildInstance(target_build)

    with self.assertRaises(paygen_build_lib.BuildNotReady):
      paygen._DiscoverRequiredPayloads()

    # Re-run with a test image, but no signed images.
    self.addTestImage(target_build)
    with self.assertRaises(paygen_build_lib.BuildNotReady):
      paygen._DiscoverRequiredPayloads()

  def testCanaryEverything(self):
    """Handle the canary payloads and tests."""
    # Make our random strings deterministic for testing.
    self.PatchObject(gspaths, '_RandomString', return_value='<random>')

    target_build = gspaths.Build(bucket='crt',
                                 channel='canary-channel',
                                 board='auron-yuna',
                                 version='9999.0.0')
    prev_build = gspaths.Build(bucket='crt',
                               channel='canary-channel',
                               board='auron-yuna',
                               version='9756.0.0')

    # Create our images.
    premp_image = self.addSignedImage(target_build, key='premp')
    mp_image = self.addSignedImage(target_build)
    test_image = self.addTestImage(target_build)
    prev_premp_image = self.addSignedImage(prev_build, key='premp')
    prev_mp_image = self.addSignedImage(prev_build)
    prev_test_image = self.addTestImage(prev_build)

    # Run the test.
    paygen = self._GetPaygenBuildInstance(target_build)
    payloads, tests = paygen._DiscoverRequiredPayloads()

    # pylint: disable=line-too-long
    # Define the expected payloads, including URLs.
    mp_full = gspaths.Payload(
        tgt_image=mp_image,
        uri='gs://crt/canary-channel/auron-yuna/9999.0.0/payloads/chromeos_9999.0.0_auron-yuna_canary-channel_full_mp.bin-<random>.signed')
    premp_full = gspaths.Payload(
        tgt_image=premp_image,
        uri='gs://crt/canary-channel/auron-yuna/9999.0.0/payloads/chromeos_9999.0.0_auron-yuna_canary-channel_full_premp.bin-<random>.signed')
    test_full = gspaths.Payload(
        tgt_image=test_image,
        uri='gs://crt/canary-channel/auron-yuna/9999.0.0/payloads/chromeos_9999.0.0_auron-yuna_canary-channel_full_test.bin-<random>')
    n2n_delta = gspaths.Payload(
        tgt_image=test_image,
        src_image=test_image,
        uri='gs://crt/canary-channel/auron-yuna/9999.0.0/payloads/chromeos_9999.0.0-9999.0.0_auron-yuna_canary-channel_delta_test.bin-<random>')
    mp_delta = gspaths.Payload(
        tgt_image=mp_image,
        src_image=prev_mp_image,
        uri='gs://crt/canary-channel/auron-yuna/9999.0.0/payloads/chromeos_9756.0.0-9999.0.0_auron-yuna_canary-channel_delta_mp.bin-<random>.signed')
    premp_delta = gspaths.Payload(
        tgt_image=premp_image,
        src_image=prev_premp_image,
        uri='gs://crt/canary-channel/auron-yuna/9999.0.0/payloads/chromeos_9756.0.0-9999.0.0_auron-yuna_canary-channel_delta_premp.bin-<random>.signed')
    test_delta = gspaths.Payload(
        tgt_image=test_image,
        src_image=prev_test_image,
        uri='gs://crt/canary-channel/auron-yuna/9999.0.0/payloads/chromeos_9756.0.0-9999.0.0_auron-yuna_canary-channel_delta_test.bin-<random>')

    # Verify the results.
    self.assertItemsEqual(
        payloads,
        [
            mp_full,
            premp_full,
            test_full,
            n2n_delta,
            mp_delta,
            premp_delta,
            test_delta,
        ])

    self.assertItemsEqual(
        tests,
        [
            paygen_build_lib.PayloadTest(test_full,
                                         'canary-channel', '9999.0.0'),
            paygen_build_lib.PayloadTest(n2n_delta),
            paygen_build_lib.PayloadTest(test_delta),
        ])

  def testCanaryPrempMismatch(self):
    """Handle the canary payloads and testss."""
    target_build = gspaths.Build(bucket='crt',
                                 channel='canary-channel',
                                 board='auron-yuna',
                                 version='9999.0.0')
    prev_build = gspaths.Build(bucket='crt',
                               channel='canary-channel',
                               board='auron-yuna',
                               version='9756.0.0')

    # Create our images.
    mp_image = self.addSignedImage(target_build)
    test_image = self.addTestImage(target_build)
    _prev_premp_image = self.addSignedImage(prev_build, key='premp')
    prev_test_image = self.addTestImage(prev_build)

    # Run the test.
    paygen = self._GetPaygenBuildInstance(target_build)
    payloads, tests = paygen._DiscoverRequiredPayloads()

    # Define the expected payloads. Test delta from prev, but no signed ones.
    mp_full = gspaths.Payload(tgt_image=mp_image, uri=mock.ANY)
    test_full = gspaths.Payload(tgt_image=test_image, uri=mock.ANY)
    n2n_delta = gspaths.Payload(tgt_image=test_image, src_image=test_image,
                                uri=mock.ANY)
    test_delta = gspaths.Payload(tgt_image=test_image,
                                 src_image=prev_test_image,
                                 uri=mock.ANY)

    # Verify the results.
    self.assertItemsEqual(
        payloads,
        [
            mp_full,
            test_full,
            n2n_delta,
            test_delta,
        ])

    self.assertItemsEqual(
        tests,
        [
            paygen_build_lib.PayloadTest(test_full,
                                         'canary-channel', '9999.0.0'),
            paygen_build_lib.PayloadTest(n2n_delta),
            paygen_build_lib.PayloadTest(test_delta),
        ])

  def testCanarySkipDeltas(self):
    """Handle the canary payloads and testss."""
    target_build = gspaths.Build(bucket='crt',
                                 channel='canary-channel',
                                 board='auron-yuna',
                                 version='9999.0.0')
    prev_build = gspaths.Build(bucket='crt',
                               channel='canary-channel',
                               board='auron-yuna',
                               version='9756.0.0')

    # Create our images.
    mp_image = self.addSignedImage(target_build)
    test_image = self.addTestImage(target_build)
    _prev_premp_image = self.addSignedImage(prev_build, key='premp')
    _prev_test_image = self.addTestImage(prev_build)

    # Run the test.
    paygen = self._GetPaygenBuildInstance(
        target_build, skip_delta_payloads=True)
    payloads, tests = paygen._DiscoverRequiredPayloads()

    # Define the expected payloads. Test delta from prev, but no signed ones.
    mp_full = gspaths.Payload(tgt_image=mp_image, uri=mock.ANY)
    test_full = gspaths.Payload(tgt_image=test_image, uri=mock.ANY)

    # Verify the results.
    self.assertItemsEqual(
        payloads,
        [
            mp_full,
            test_full,
        ])

    self.assertItemsEqual(
        tests,
        [
            paygen_build_lib.PayloadTest(test_full,
                                         'canary-channel', '9999.0.0'),
        ])

  def testStable(self):
    """Handle the canary payloads and testss."""
    target_build = gspaths.Build(bucket='crt',
                                 channel='stable-channel',
                                 board='auron-yuna',
                                 version='9999.0.0')
    build_8530 = gspaths.Build(bucket='crt',
                               channel='stable-channel',
                               board='auron-yuna',
                               version='8530.96.0')
    build_8743 = gspaths.Build(bucket='crt',
                               channel='stable-channel',
                               board='auron-yuna',
                               version='8743.85.0')
    build_8872 = gspaths.Build(bucket='crt',
                               channel='stable-channel',
                               board='auron-yuna',
                               version='8872.76.0')
    build_9000 = gspaths.Build(bucket='crt',
                               channel='stable-channel',
                               board='auron-yuna',
                               version='9000.91.0')
    build_9202 = gspaths.Build(bucket='crt',
                               channel='stable-channel',
                               board='auron-yuna',
                               version='9202.64.0')
    build_9334 = gspaths.Build(bucket='crt',
                               channel='stable-channel',
                               board='auron-yuna',
                               version='9334.72.0')
    build_9460 = gspaths.Build(bucket='crt',
                               channel='stable-channel',
                               board='auron-yuna',
                               version='9460.60.0')
    build_9460_67 = gspaths.Build(bucket='crt',
                                  channel='stable-channel',
                                  board='auron-yuna',
                                  version='9460.67.0')

    # Create our images, ignore FSI 6457.83.0, 7390.68.0
    mp_image = self.addSignedImage(target_build)
    test_image = self.addTestImage(target_build)

    image_8530 = self.addSignedImage(build_8530)
    test_image_8530 = self.addTestImage(build_8530)

    image_8743 = self.addSignedImage(build_8743)
    test_image_8743 = self.addTestImage(build_8743)

    image_8872 = self.addSignedImage(build_8872)
    test_image_8872 = self.addTestImage(build_8872)

    image_9000 = self.addSignedImage(build_9000)
    test_image_9000 = self.addTestImage(build_9000)

    image_9202 = self.addSignedImage(build_9202)
    test_image_9202 = self.addTestImage(build_9202)

    image_9334 = self.addSignedImage(build_9334)
    test_image_9334 = self.addTestImage(build_9334)

    image_9460 = self.addSignedImage(build_9460)
    test_image_9460 = self.addTestImage(build_9460)

    image_9460_67 = self.addSignedImage(build_9460_67)
    test_image_9460_67 = self.addTestImage(build_9460_67)

    # Run the test.
    paygen = self._GetPaygenBuildInstance(target_build)
    payloads, tests = paygen._DiscoverRequiredPayloads()

    # Define the expected payloads. Test delta from prev, but no signed ones.
    mp_full = gspaths.Payload(tgt_image=mp_image, uri=mock.ANY)
    test_full = gspaths.Payload(tgt_image=test_image, uri=mock.ANY)
    n2n_delta = gspaths.Payload(tgt_image=test_image, src_image=test_image,
                                uri=mock.ANY)

    mp_delta_8530 = gspaths.Payload(
        tgt_image=mp_image, src_image=image_8530, uri=mock.ANY)
    test_delta_8530 = gspaths.Payload(
        tgt_image=test_image, src_image=test_image_8530, uri=mock.ANY)
    mp_delta_8743 = gspaths.Payload(
        tgt_image=mp_image, src_image=image_8743, uri=mock.ANY)
    test_delta_8743 = gspaths.Payload(
        tgt_image=test_image, src_image=test_image_8743, uri=mock.ANY)
    mp_delta_8872 = gspaths.Payload(
        tgt_image=mp_image, src_image=image_8872, uri=mock.ANY)
    test_delta_8872 = gspaths.Payload(
        tgt_image=test_image, src_image=test_image_8872, uri=mock.ANY)
    mp_delta_9000 = gspaths.Payload(
        tgt_image=mp_image, src_image=image_9000, uri=mock.ANY)
    test_delta_9000 = gspaths.Payload(
        tgt_image=test_image, src_image=test_image_9000, uri=mock.ANY)
    mp_delta_9202 = gspaths.Payload(
        tgt_image=mp_image, src_image=image_9202, uri=mock.ANY)
    test_delta_9202 = gspaths.Payload(
        tgt_image=test_image, src_image=test_image_9202, uri=mock.ANY)
    mp_delta_9334 = gspaths.Payload(
        tgt_image=mp_image, src_image=image_9334, uri=mock.ANY)
    test_delta_9334 = gspaths.Payload(
        tgt_image=test_image, src_image=test_image_9334, uri=mock.ANY)
    mp_delta_9460 = gspaths.Payload(
        tgt_image=mp_image, src_image=image_9460, uri=mock.ANY)
    test_delta_9460 = gspaths.Payload(
        tgt_image=test_image, src_image=test_image_9460, uri=mock.ANY)
    mp_delta_9460_67 = gspaths.Payload(
        tgt_image=mp_image, src_image=image_9460_67, uri=mock.ANY)
    test_delta_9460_67 = gspaths.Payload(
        tgt_image=test_image, src_image=test_image_9460_67, uri=mock.ANY)

    # Verify the results.
    self.assertItemsEqual(
        payloads,
        [
            mp_full,
            test_full,
            n2n_delta,
            mp_delta_8530, test_delta_8530,
            mp_delta_8743, test_delta_8743,
            mp_delta_8872, test_delta_8872,
            mp_delta_9000, test_delta_9000,
            mp_delta_9202, test_delta_9202,
            mp_delta_9334, test_delta_9334,
            mp_delta_9460, test_delta_9460,
            mp_delta_9460_67, test_delta_9460_67,
        ])

    self.assertItemsEqual(
        tests,
        [
            paygen_build_lib.PayloadTest(
                test_full, 'stable-channel', '9999.0.0'),
            paygen_build_lib.PayloadTest(
                test_full, 'stable-channel', '8530.96.0'),
            paygen_build_lib.PayloadTest(n2n_delta),
            paygen_build_lib.PayloadTest(test_delta_8530),
            paygen_build_lib.PayloadTest(test_delta_8743),
            paygen_build_lib.PayloadTest(test_delta_8872),
            paygen_build_lib.PayloadTest(test_delta_9000),
            paygen_build_lib.PayloadTest(test_delta_9202),
            paygen_build_lib.PayloadTest(test_delta_9334),
            paygen_build_lib.PayloadTest(test_delta_9460),
            # test_image_9460_67 had test turned off in json.
        ])


class TestPayloadGeneration(BasePaygenBuildLibTestWithBuilds):
  """Test GeneratePayloads method."""

  def testGeneratePayloads(self):
    """Test paygen_build_lib._GeneratePayloads, no dry_run."""
    poolMock = self.PatchObject(parallel, 'RunTasksInProcessPool')

    paygen = self._GetPaygenBuildInstance()
    paygen._GeneratePayloads((self.mp_full_payload,
                              self.mp_delta_payload,
                              self.test_delta_payload))

    self.assertEqual(
        poolMock.call_args_list,
        [mock.call(paygen_build_lib._GenerateSinglePayload,
                   [(self.mp_full_payload, self.tempdir, True, False),
                    (self.mp_delta_payload, self.tempdir, True, False),
                    (self.test_delta_payload, self.tempdir, False, False)])])

  def testGeneratePayloadsDryrun(self):
    """Ensure we correctly pass along the dryrun flag."""
    poolMock = self.PatchObject(parallel, 'RunTasksInProcessPool')

    paygen = self._GetPaygenBuildInstance(dry_run=True)
    paygen._GeneratePayloads((self.mp_full_payload,
                              self.mp_delta_payload,
                              self.test_delta_payload))

    self.assertEqual(
        poolMock.call_args_list,
        [mock.call(paygen_build_lib._GenerateSinglePayload,
                   [(self.mp_full_payload, self.tempdir, True, True),
                    (self.mp_delta_payload, self.tempdir, True, True),
                    (self.test_delta_payload, self.tempdir, False, True)])])

  def testGeneratePayloadInProcess(self):
    """Make sure the _GenerateSinglePayload calls into paygen_payload_lib."""
    createMock = self.PatchObject(
        paygen_payload_lib, 'CreateAndUploadPayload')

    paygen_build_lib._GenerateSinglePayload(
        self.test_delta_payload, self.tempdir, False, False)

    self.assertEqual(
        createMock.call_args_list,
        [mock.call(self.test_delta_payload,
                   mock.ANY,
                   work_dir=self.tempdir,
                   sign=False,
                   dry_run=False)])

  def testCleanupBuild(self):
    """Test PaygenBuild._CleanupBuild."""
    removeMock = self.PatchObject(gslib, 'Remove')

    paygen = self._GetPaygenBuildInstance()

    paygen._CleanupBuild()

    self.assertEqual(
        removeMock.call_args_list,
        [mock.call('gs://crt/foo-channel/foo-board/1.2.3/payloads/signing',
                   recurse=True, ignore_no_match=True)])


class TestCreatePayloads(BasePaygenBuildLibTestWithBuilds):
  """Test CreatePayloads."""
  def setUp(self):
    self.mockCreate = self.PatchObject(gslib, 'CreateWithContents')
    self.mockExists = self.PatchObject(gslib, 'Exists')
    self.mockRemove = self.PatchObject(gslib, 'Remove')
    self.mockLock = self.PatchObject(gslock, 'Lock')

    self.mockDiscover = self.PatchObject(
        paygen_build_lib.PaygenBuild, '_DiscoverRequiredPayloads')
    self.mockGenerate = self.PatchObject(
        paygen_build_lib.PaygenBuild, '_GeneratePayloads')
    self.mockArchive = self.PatchObject(
        paygen_build_lib.PaygenBuild, '_MapToArchive')
    self.mockAutotest = self.PatchObject(
        paygen_build_lib.PaygenBuild, '_AutotestPayloads')
    self.mockCleanup = self.PatchObject(
        paygen_build_lib.PaygenBuild, '_CleanupBuild')
    self.mockExisting = self.PatchObject(
        paygen_payload_lib, 'FindExistingPayloads')

  def testCreatePayloadsLockedBuild(self):
    self.mockLock.side_effect = gslock.LockNotAcquired

    paygen = self._GetPaygenBuildInstance()

    with self.assertRaises(paygen_build_lib.BuildLocked):
      paygen.CreatePayloads()

  def testCreatePayloadsBuildNotReady(self):
    """Test paygen_build_lib._GeneratePayloads if not all images are there."""
    self.mockExists.return_value = False
    self.mockDiscover.side_effect = paygen_build_lib.BuildNotReady

    paygen = self._GetPaygenBuildInstance()

    with self.assertRaises(paygen_build_lib.BuildNotReady):
      paygen.CreatePayloads()

    self.assertEqual(self.mockCleanup.call_args_list, [mock.call()])

  def testCreatePayloadsCreateFailed(self):
    """Test paygen_build_lib._GeneratePayloads if payload generation failed."""
    mockException = Exception

    self.mockGenerate.side_effect = mockException

    paygen = self._GetPaygenBuildInstance()

    with self.assertRaises(mockException):
      paygen.CreatePayloads()

    self.assertEqual(self.mockCleanup.call_args_list, [mock.call()])

  def testCreatePayloadsSuccess(self):
    """Test paygen_build_lib._GeneratePayloads success."""
    self.mockArchive.return_value = (
        'archive_board', 'archive_build', 'archive_build_uri')
    self.mockAutotest.return_value = 'suite_name'
    self.mockExisting.return_value = None

    payloads = [
        self.mp_full_payload,
        self.test_full_payload,
        self.test_delta_payload,
    ]
    payload_tests = [
        self.full_payload_test,
        self.delta_payload_test,
    ]

    self.mockExists.return_value = False
    self.mockDiscover.return_value = (payloads, payload_tests)

    paygen = self._GetPaygenBuildInstance()

    testdata = paygen.CreatePayloads()

    self.assertEqual(
        testdata,
        ('suite_name', 'archive_board', 'archive_build')
    )

    self.assertEqual(self.mockGenerate.call_args_list, [
        mock.call(payloads),
    ])

    self.assertEqual(self.mockArchive.call_args_list, [
        mock.call('foo-board', '1.2.3'),
    ])

    self.assertEqual(self.mockAutotest.call_args_list, [
        mock.call(payload_tests),
    ])

    self.assertEqual(self.mockCleanup.call_args_list, [mock.call()])

  def testCreatePayloadsSuccessAllExist(self):
    """Test paygen_build_lib._GeneratePayloads success."""
    self.mockArchive.return_value = (
        'archive_board', 'archive_build', 'archive_build_uri')
    self.mockAutotest.return_value = 'suite_name'
    self.mockExisting.return_value = ['existing_url']

    payloads = [
        self.mp_full_payload,
        self.test_full_payload,
        self.test_delta_payload,
    ]
    payload_tests = [
        self.full_payload_test,
        self.delta_payload_test,
    ]

    self.mockExists.return_value = False
    self.mockDiscover.return_value = (payloads, payload_tests)

    paygen = self._GetPaygenBuildInstance()

    testdata = paygen.CreatePayloads()

    self.assertEqual(
        testdata,
        ('suite_name', 'archive_board', 'archive_build')
    )

    # Note... no payloads were generated.
    self.assertEqual(self.mockGenerate.call_args_list, [])

    self.assertEqual(self.mockArchive.call_args_list, [
        mock.call('foo-board', '1.2.3'),
    ])

    self.assertEqual(self.mockAutotest.call_args_list, [
        mock.call(payload_tests),
    ])

    self.assertEqual(self.mockCleanup.call_args_list, [mock.call()])


class TestAutotestPayloadsPayloads(BasePaygenBuildLibTestWithBuilds):
  """Test autotest tarball generation."""
  def setUp(self):
    # For autotest, we have to look up URLs for old builds. Mock out lookup.
    self.mockFind = self.PatchObject(
        paygen_build_lib.PaygenBuild, '_FindFullTestPayloads',
        side_effect=lambda channel, version: ['%s_%s_uri' % (channel, version)])

    self.mockExists = self.PatchObject(
        urilib, 'Exists',
        side_effect=lambda uri: uri and uri.endswith('stateful.tgz'))

    self.mockCopy = self.PatchObject(gslib, 'Copy')

    # Our images have to exist, and have URIs for autotest.
    self.test_image.uri = 'test_image_uri'
    self.prev_test_image.uri = 'prev_test_image_uri'

  def testAutotestPayloadsSuccess(self):
    payload_tests = [
        self.full_payload_test,
        self.delta_payload_test,
    ]

    paygen = self._GetPaygenBuildInstance()

    # These are normally set during CreatePayloads.
    paygen._archive_board = 'archive_board'
    paygen._archive_build = 'archive_board/R62-9778.0.0'
    paygen._archive_build_uri = 'archive_uri'

    paygen._AutotestPayloads(payload_tests)

    tarball_path = os.path.join(
        self.tempdir, 'autotests/paygen_au_foo_control.tar.bz2')

    # Verify that we uploaded the results correctly.
    self.assertEqual(self.mockCopy.call_args_list, [
        mock.call(tarball_path, 'archive_uri/paygen_au_foo_control.tar.bz2',
                  acl='public-read'),
    ])

    delta_ctrl = 'autotest/au_control_files/control.paygen_au_foo_delta_1.0.0'
    full_ctrl = 'autotest/au_control_files/control.paygen_au_foo_full_1.2.3'

    # Verify tarfile contents.
    with tarfile.open(tarball_path) as t:
      self.assertItemsEqual(
          t.getnames(),
          ['autotest/au_control_files', delta_ctrl, full_ctrl])

      delta_fp = t.extractfile(delta_ctrl)
      delta_contents = delta_fp.read()
      delta_fp.close()

      full_fp = t.extractfile(full_ctrl)
      full_contents = full_fp.read()
      full_fp.close()

    # We only checking the beginning to avoid the very long doc string.
    self.assertTrue(delta_contents.startswith("""name = 'paygen_au_foo'
update_type = 'delta'
source_release = '1.0.0'
target_release = '1.2.3'
target_payload_uri = 'None'
SUITE = 'paygen_au_foo'
source_payload_uri = 'foo-channel_1.0.0_uri'
source_archive_uri = 'gs://chromeos-releases/foo-channel/foo-board/1.0.0'
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.bin import sysinfo
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import constants
from autotest_lib.server import host_attributes

AUTHOR = "Chromium OS"
NAME = "autoupdate_EndToEndTest_paygen_au_foo_delta_1.0.0"
TIME = "MEDIUM"
TEST_CATEGORY = "Functional"
TEST_CLASS = "platform"
TEST_TYPE = "server"
JOB_RETRIES = 1
BUG_TEMPLATE = {
    'cc': ['chromeos-installer-alerts@google.com'],
    'components': ['Internals>Installer'],
}

# Skip provision special task for AU tests.
DEPENDENCIES = "skip_provision"

# Disable server-side packaging support for this test.
# This control file is used as the template for paygen_au_canary suite, which
# creates the control files during paygen. Therefore, autotest server package
# does not have these test control files for paygen_au_canary suite.
REQUIRE_SSP = False

DOC ="""))

    # We only checking the beginning to avoid the very long doc string.
    self.assertTrue(full_contents.startswith("""name = 'paygen_au_foo'
update_type = 'full'
source_release = '1.2.3'
target_release = '1.2.3'
target_payload_uri = 'None'
SUITE = 'paygen_au_foo'
source_payload_uri = 'foo-channel_1.2.3_uri'
source_archive_uri = 'gs://chromeos-releases/foo-channel/foo-board/1.2.3'
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.bin import sysinfo
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import constants
from autotest_lib.server import host_attributes

AUTHOR = "Chromium OS"
NAME = "autoupdate_EndToEndTest_paygen_au_foo_full_1.2.3"
TIME = "MEDIUM"
TEST_CATEGORY = "Functional"
TEST_CLASS = "platform"
TEST_TYPE = "server"
JOB_RETRIES = 1
BUG_TEMPLATE = {
    'cc': ['chromeos-installer-alerts@google.com'],
    'components': ['Internals>Installer'],
}

# Skip provision special task for AU tests.
DEPENDENCIES = "skip_provision"

# Disable server-side packaging support for this test.
# This control file is used as the template for paygen_au_canary suite, which
# creates the control files during paygen. Therefore, autotest server package
# does not have these test control files for paygen_au_canary suite.
REQUIRE_SSP = False

DOC ="""))
