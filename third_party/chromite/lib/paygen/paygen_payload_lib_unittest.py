# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test paygen_payload_lib library."""

from __future__ import print_function

import mox
import os
import shutil
import tempfile

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils

from chromite.lib.paygen import download_cache
from chromite.lib.paygen import gspaths
from chromite.lib.paygen import paygen_payload_lib
from chromite.lib.paygen import signer_payloads_client
from chromite.lib.paygen import urilib


# We access a lot of protected members during testing.
# pylint: disable=protected-access


class PaygenPayloadLibTest(cros_test_lib.MoxTempDirTestCase):
  """PaygenPayloadLib tests base class."""

  def setUp(self):
    self.old_image = gspaths.Image(
        channel='dev-channel',
        board='x86-alex',
        version='1620.0.0',
        key='mp-v3',
        uri=('gs://chromeos-releases-test/dev-channel/x86-alex/1620.0.0/'
             'chromeos_1620.0.0_x86-alex_recovery_dev-channel_mp-v3.bin'))

    self.old_base_image = gspaths.Image(
        channel='dev-channel',
        board='x86-alex',
        version='1620.0.0',
        key='mp-v3',
        image_type='base',
        uri=('gs://chromeos-releases-test/dev-channel/x86-alex/1620.0.0/'
             'chromeos_1620.0.0_x86-alex_base_dev-channel_mp-v3.bin'))

    self.new_image = gspaths.Image(
        channel='dev-channel',
        board='x86-alex',
        version='4171.0.0',
        key='mp-v3',
        uri=('gs://chromeos-releases-test/dev-channel/x86-alex/4171.0.0/'
             'chromeos_4171.0.0_x86-alex_recovery_dev-channel_mp-v3.bin'))

    self.old_test_image = gspaths.UnsignedImageArchive(
        channel='dev-channel',
        board='x86-alex',
        version='1620.0.0',
        uri=('gs://chromeos-releases-test/dev-channel/x86-alex/1620.0.0/'
             'chromeos_1620.0.0_x86-alex_recovery_dev-channel_test.bin'))

    self.new_test_image = gspaths.Image(
        channel='dev-channel',
        board='x86-alex',
        version='4171.0.0',
        uri=('gs://chromeos-releases-test/dev-channel/x86-alex/4171.0.0/'
             'chromeos_4171.0.0_x86-alex_recovery_dev-channel_test.bin'))

    self.full_payload = gspaths.Payload(tgt_image=self.old_base_image,
                                        src_image=None,
                                        uri='gs://full_old_foo/boo')

    self.delta_payload = gspaths.Payload(tgt_image=self.new_image,
                                         src_image=self.old_image,
                                         uri='gs://delta_new_old/boo')

    self.full_test_payload = gspaths.Payload(tgt_image=self.old_test_image,
                                             src_image=None,
                                             uri='gs://full_old_foo/boo-test')

    self.delta_test_payload = gspaths.Payload(tgt_image=self.new_test_image,
                                              src_image=self.old_test_image,
                                              uri='gs://delta_new_old/boo-test')

  @classmethod
  def setUpClass(cls):
    cls.cache_dir = tempfile.mkdtemp(prefix='crostools-unittest-cache')
    cls.cache = download_cache.DownloadCache(cls.cache_dir)

  @classmethod
  def tearDownClass(cls):
    cls.cache = None
    shutil.rmtree(cls.cache_dir)


class PaygenPayloadLibBasicTest(PaygenPayloadLibTest):
  """PaygenPayloadLib basic (and quick) testing."""

  def _GetStdGenerator(self, work_dir=None, payload=None, sign=True,
                       au_generator_uri_override=None):
    """Helper function to create a standardized PayloadGenerator."""
    if payload is None:
      payload = self.full_payload

    if work_dir is None:
      work_dir = self.tempdir

    if not au_generator_uri_override:
      au_generator_uri_override = gspaths.ChromeosReleases.GeneratorUri(
          payload.tgt_image.channel, payload.tgt_image.board, '7587.0.0')

    return paygen_payload_lib._PaygenPayload(
        payload=payload,
        cache=self.cache,
        work_dir=work_dir,
        sign=sign,
        verify=False,
        au_generator_uri_override=au_generator_uri_override)

  def testWorkingDirNames(self):
    """Make sure that some of the files we create have the expected names."""
    gen = self._GetStdGenerator(work_dir='/foo')

    self.assertEqual(gen.generator_dir, '/foo/au-generator')
    self.assertEqual(gen.src_image_file, '/foo/src_image.bin')
    self.assertEqual(gen.tgt_image_file, '/foo/tgt_image.bin')
    self.assertEqual(gen.payload_file, '/foo/delta.bin')
    self.assertEqual(gen.log_file, '/foo/delta.log')
    self.assertEqual(gen.metadata_size_file, '/foo/metadata_size.txt')

    # Siged image specific values.
    self.assertEqual(gen.signed_payload_file, '/foo/delta.bin.signed')
    self.assertEqual(gen.metadata_signature_file,
                     '/foo/delta.bin.signed.metadata-signature')

  def testUriManipulators(self):
    """Validate _MetadataUri."""
    gen = self._GetStdGenerator(work_dir='/foo')

    self.assertEqual(gen._MetadataUri('/foo/bar'),
                     '/foo/bar.metadata-signature')
    self.assertEqual(gen._MetadataUri('gs://foo/bar'),
                     'gs://foo/bar.metadata-signature')

    self.assertEqual(gen._LogsUri('/foo/bar'),
                     '/foo/bar.log')
    self.assertEqual(gen._LogsUri('gs://foo/bar'),
                     'gs://foo/bar.log')

    self.assertEqual(gen._JsonUri('/foo/bar'),
                     '/foo/bar.json')
    self.assertEqual(gen._JsonUri('gs://foo/bar'),
                     'gs://foo/bar.json')

  @cros_test_lib.NetworkTest()
  def testPrepareGenerator(self):
    """Validate that we can download an unzip a generator artifact."""
    gen = self._GetStdGenerator()
    gen._PrepareGenerator()

    # Ensure that the expected executables in the au-generator are available.
    expected = os.path.join(gen.generator_dir, 'cros_generate_update_payload')
    self.assertExists(expected)

    expected = os.path.join(gen.generator_dir, 'delta_generator')
    self.assertExists(expected)

  def testRunGeneratorCmd(self):
    """Test the specialized command to run programs from au-generate.zip."""
    expected_cmd = ['cmd', 'bar', 'jo nes']
    original_environ = os.environ.copy()
    gen = self._GetStdGenerator(work_dir='/foo')

    self.mox.StubOutWithMock(cros_build_lib, 'RunCommand')
    self.mox.StubOutWithMock(gen, '_StoreLog')

    mock_result = cros_build_lib.CommandResult()
    mock_result.output = 'foo output'

    # Set up the test replay script.
    cros_build_lib.RunCommand(
        expected_cmd,
        redirect_stdout=True,
        enter_chroot=True,
        combine_stdout_stderr=True,
        error_code_ok=True).AndReturn(mock_result)

    gen._StoreLog('Output of command: cmd bar jo nes')
    gen._StoreLog(mock_result.output)

    # Run the test verification.
    self.mox.ReplayAll()
    gen._RunGeneratorCmd(expected_cmd)

    # Demonstrate that the PATH was restored.
    self.assertEqual(os.environ, original_environ)

  def testBuildArg(self):
    """Make sure the function semantics is satisfied."""
    gen = self._GetStdGenerator(work_dir='/work')
    test_dict = {'foo': 'bar'}

    # Value present.
    self.assertEqual(gen._BuildArg('--foo', test_dict, 'foo'),
                     ['--foo', 'bar'])
    self.assertEqual(gen._BuildArg(None, test_dict, 'foo'),
                     ['bar'])

    # Value present, default has no impact.
    self.assertEqual(gen._BuildArg('--foo', test_dict, 'foo', default='baz'),
                     ['--foo', 'bar'])

    # Value missing, default kicking in.
    self.assertEqual(gen._BuildArg('--foo2', test_dict, 'foo2', default='baz'),
                     ['--foo2', 'baz'])

  def _DoPrepareImageTest(self, image_type):
    """Test _PrepareImage via mox."""
    download_uri = 'gs://bucket/foo/image.bin'
    image_file = '/work/image.bin'
    test_work_dir = tempfile.gettempdir()  # for testing purposes
    gen = self._GetStdGenerator(work_dir=test_work_dir)

    if image_type == 'Image':
      image_obj = gspaths.Image(
          channel='dev-channel',
          board='x86-alex',
          version='4171.0.0',
          key='mp-v3',
          uri=download_uri)
      test_extract_file = None
    elif image_type == 'UnsignedImageArchive':
      image_obj = gspaths.UnsignedImageArchive(
          channel='dev-channel',
          board='x86-alex',
          version='4171.0.0',
          image_type='test',
          uri=download_uri)
      test_extract_file = paygen_payload_lib._PaygenPayload.TEST_IMAGE_NAME
    else:
      raise ValueError('invalid image type descriptor (%s)' % image_type)

    # Stub out and record the expected function calls.
    self.mox.StubOutWithMock(download_cache.DownloadCache,
                             'GetFileCopy')
    if test_extract_file:
      download_file = mox.IsA(str)
    else:
      download_file = image_file
    self.cache.GetFileCopy(download_uri, download_file)

    if test_extract_file:
      self.mox.StubOutWithMock(cros_build_lib, 'RunCommand')
      cros_build_lib.RunCommand(['tar', '-xJf', download_file,
                                 test_extract_file], cwd=test_work_dir)
      self.mox.StubOutWithMock(shutil, 'move')
      shutil.move(os.path.join(test_work_dir, test_extract_file), image_file)

    # Run the test.
    self.mox.ReplayAll()
    gen._PrepareImage(image_obj, image_file)

  def testPrepareImageNormal(self):
    """Test preparing a normal image."""
    self._DoPrepareImageTest('Image')

  def testPrepareImageTest(self):
    """Test preparing a test image."""
    self._DoPrepareImageTest('UnsignedImageArchive')

  def testGenerateUnsignedPayloadFull(self):
    """Test _GenerateUnsignedPayload with full payload."""
    gen = self._GetStdGenerator(payload=self.full_payload, work_dir='/work')

    # Stub out the required functions.
    self.mox.StubOutWithMock(gen, '_RunGeneratorCmd')
    self.mox.StubOutWithMock(gen, '_CheckPartitionFiles')

    # Record the expected function calls.
    cmd = ['cros_generate_update_payload',
           '--output', gen.payload_file,
           '--image', gen.tgt_image_file,
           '--channel', 'dev-channel',
           '--board', 'x86-alex',
           '--version', '1620.0.0',
           '--kern_path', '/work/new_kernel.dat',
           '--root_path', '/work/new_rootfs.dat',
           '--key', 'mp-v3',
           '--build_channel', 'dev-channel',
           '--build_version', '1620.0.0']
    gen._RunGeneratorCmd(cmd)
    gen._CheckPartitionFiles()

    # Run the test.
    self.mox.ReplayAll()
    gen._GenerateUnsignedPayload()

  def testGenerateUnsignedPayloadDelta(self):
    """Test _GenerateUnsignedPayload with delta payload."""
    gen = self._GetStdGenerator(payload=self.delta_payload, work_dir='/work')

    # Stub out the required functions.
    self.mox.StubOutWithMock(gen, '_RunGeneratorCmd')
    self.mox.StubOutWithMock(gen, '_CheckPartitionFiles')

    # Record the expected function calls.
    cmd = ['cros_generate_update_payload',
           '--output', gen.payload_file,
           '--image', gen.tgt_image_file,
           '--channel', 'dev-channel',
           '--board', 'x86-alex',
           '--version', '4171.0.0',
           '--kern_path', '/work/new_kernel.dat',
           '--root_path', '/work/new_rootfs.dat',
           '--key', 'mp-v3',
           '--build_channel', 'dev-channel',
           '--build_version', '4171.0.0',
           '--src_image', gen.src_image_file,
           '--src_channel', 'dev-channel',
           '--src_board', 'x86-alex',
           '--src_version', '1620.0.0',
           '--src_kern_path', '/work/old_kernel.dat',
           '--src_root_path', '/work/old_rootfs.dat',
           '--src_key', 'mp-v3',
           '--src_build_channel', 'dev-channel',
           '--src_build_version', '1620.0.0']
    gen._RunGeneratorCmd(cmd)
    gen._CheckPartitionFiles()

    # Run the test.
    self.mox.ReplayAll()
    gen._GenerateUnsignedPayload()

  def testGenerateUnsignedTestPayloadFull(self):
    """Test _GenerateUnsignedPayload with full test payload."""
    gen = self._GetStdGenerator(payload=self.full_test_payload,
                                work_dir='/work')

    # Stub out the required functions.
    self.mox.StubOutWithMock(gen, '_RunGeneratorCmd')
    self.mox.StubOutWithMock(gen, '_CheckPartitionFiles')

    # Record the expected function calls.
    cmd = ['cros_generate_update_payload',
           '--output', gen.payload_file,
           '--image', gen.tgt_image_file,
           '--channel', 'dev-channel',
           '--board', 'x86-alex',
           '--version', '1620.0.0',
           '--kern_path', '/work/new_kernel.dat',
           '--root_path', '/work/new_rootfs.dat',
           '--key', 'test',
           '--build_channel', 'dev-channel',
           '--build_version', '1620.0.0']
    gen._RunGeneratorCmd(cmd)
    gen._CheckPartitionFiles()

    # Run the test.
    self.mox.ReplayAll()
    gen._GenerateUnsignedPayload()

  def testGenerateUnsignedTestPayloadDelta(self):
    """Test _GenerateUnsignedPayload with delta payload."""
    gen = self._GetStdGenerator(payload=self.delta_test_payload,
                                work_dir='/work')

    # Stub out the required functions.
    self.mox.StubOutWithMock(gen, '_RunGeneratorCmd')
    self.mox.StubOutWithMock(gen, '_CheckPartitionFiles')

    # Record the expected function calls.
    cmd = ['cros_generate_update_payload',
           '--output', gen.payload_file,
           '--image', gen.tgt_image_file,
           '--channel', 'dev-channel',
           '--board', 'x86-alex',
           '--version', '4171.0.0',
           '--kern_path', '/work/new_kernel.dat',
           '--root_path', '/work/new_rootfs.dat',
           '--key', 'test',
           '--build_channel', 'dev-channel',
           '--build_version', '4171.0.0',
           '--src_image', gen.src_image_file,
           '--src_channel', 'dev-channel',
           '--src_board', 'x86-alex',
           '--src_version', '1620.0.0',
           '--src_kern_path', '/work/old_kernel.dat',
           '--src_root_path', '/work/old_rootfs.dat',
           '--src_key', 'test',
           '--src_build_channel', 'dev-channel',
           '--src_build_version', '1620.0.0']
    gen._RunGeneratorCmd(cmd)
    gen._CheckPartitionFiles()

    # Run the test.
    self.mox.ReplayAll()
    gen._GenerateUnsignedPayload()

  def testGenerateHashes(self):
    """Test _GenerateHashes via mox."""
    gen = self._GetStdGenerator()

    # Stub out the required functions.
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_RunGeneratorCmd')

    # Record the expected function calls.
    cmd = ['brillo_update_payload', 'hash',
           '--unsigned_payload', gen.payload_file,
           '--payload_hash_file', mox.IsA(str),
           '--metadata_hash_file', mox.IsA(str),
           '--signature_size', '256']
    gen._RunGeneratorCmd(cmd)

    # Run the test.
    self.mox.ReplayAll()
    self.assertEqual(gen._GenerateHashes(), ('', ''))

  def testSignHashes(self):
    """Test _SignHashes via mox."""
    hashes = ('foo', 'bar')
    signatures = (('0' * 256,), ('1' * 256,))

    gen = self._GetStdGenerator(work_dir='/work')

    # Stub out the required functions.
    self.mox.StubOutWithMock(
        signer_payloads_client.SignerPayloadsClientGoogleStorage,
        'GetHashSignatures')

    gen.signer.GetHashSignatures(
        hashes,
        keysets=gen.PAYLOAD_SIGNATURE_KEYSETS).AndReturn(signatures)

    # Run the test.
    self.mox.ReplayAll()
    self.assertEqual(gen._SignHashes(hashes),
                     signatures)

  def testInsertPayloadSignatures(self):
    """Test inserting payload signatures."""
    gen = self._GetStdGenerator(payload=self.delta_payload)
    payload_signatures = ('0' * 256,)

    # Stub out the required functions.
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_RunGeneratorCmd')
    self.mox.StubOutWithMock(gen, '_ReadMetadataSizeFile')


    # Record the expected function calls.
    cmd = ['delta_generator',
           '-in_file=' + gen.payload_file,
           mox.IsA(str),
           '-out_file=' + gen.signed_payload_file,
           '-out_metadata_size_file=' + gen.metadata_size_file]
    gen._RunGeneratorCmd(cmd)
    gen._ReadMetadataSizeFile()

    # Run the test.
    self.mox.ReplayAll()
    gen._InsertPayloadSignatures(payload_signatures)

  def testStoreMetadataSignatures(self):
    """Test how we store metadata signatures."""
    gen = self._GetStdGenerator(payload=self.delta_payload)
    metadata_signatures = ('1' * 256,)
    encoded_metadata_signature = (
        'MTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMT'
        'ExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTEx'
        'MTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMT'
        'ExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTEx'
        'MTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMT'
        'ExMTExMTExMQ==')

    gen._StoreMetadataSignatures(metadata_signatures)

    with file(gen.metadata_signature_file, 'rb') as f:
      self.assertEqual(f.read(), encoded_metadata_signature)

  def testVerifyPayloadDelta(self):
    """Test _VerifyPayload with delta payload."""
    gen = self._GetStdGenerator(payload=self.delta_test_payload,
                                work_dir='/work')

    # Stub out the required functions.
    self.mox.StubOutWithMock(gen, '_RunGeneratorCmd')
    gen.metadata_size = 10

    # Record the expected function calls.
    cmd = ['check_update_payload',
           '--check',
           '--type', 'delta',
           '--disabled_tests', 'move-same-src-dst-block',
           '--dst_kern', '/work/new_kernel.dat',
           '--dst_root', '/work/new_rootfs.dat',
           '--meta-sig', gen.metadata_signature_file,
           '--metadata-size', "10",
           '--src_kern', '/work/old_kernel.dat',
           '--src_root', '/work/old_rootfs.dat',
           gen.signed_payload_file]

    gen._RunGeneratorCmd(cmd)

    # Run the test.
    self.mox.ReplayAll()
    gen._VerifyPayload()

  def testVerifyPayloadFull(self):
    """Test _VerifyPayload with Full payload."""
    gen = self._GetStdGenerator(payload=self.full_test_payload,
                                work_dir='/work')

    # Stub out the required functions.
    self.mox.StubOutWithMock(gen, '_RunGeneratorCmd')
    gen.metadata_size = 10

    # Record the expected function calls.
    cmd = ['check_update_payload',
           '--check',
           '--type', 'full',
           '--disabled_tests', 'move-same-src-dst-block',
           '--dst_kern', '/work/new_kernel.dat',
           '--dst_root', '/work/new_rootfs.dat',
           '--meta-sig', gen.metadata_signature_file,
           '--metadata-size', "10",
           gen.signed_payload_file]

    gen._RunGeneratorCmd(cmd)

    # Run the test.
    self.mox.ReplayAll()
    gen._VerifyPayload()

  def testPayloadJson(self):
    """Test how we store the payload description in json."""
    gen = self._GetStdGenerator(payload=self.delta_payload, sign=False)
    # Intentionally don't create signed file, to ensure it's never used.
    osutils.WriteFile(gen.payload_file, 'Fake payload contents.')
    gen.metadata_size = 10

    metadata_signatures = ()

    expected_json = (
        '{"md5_hex": "75218643432e5f621386d4ffcbedf9ba",'
        ' "metadata_signature": null,'
        ' "metadata_size": 10,'
        ' "sha1_hex": "FDwoNOUO+kNwrQJMSLnLDY7iZ/E=",'
        ' "sha256_hex": "gkm9207E7xbqpNRBFjEPO43nxyp/MNGQfyH3IYrq2kE=",'
        ' "version": 2}')
    gen._StorePayloadJson(metadata_signatures)

    # Validate the results.
    self.assertEqual(osutils.ReadFile(gen.description_file), expected_json)

  def testPayloadJsonSigned(self):
    """Test how we store the payload description in json."""
    gen = self._GetStdGenerator(payload=self.delta_payload, sign=True)
    # Intentionally don't create unsigned file, to ensure it's never used.
    osutils.WriteFile(gen.signed_payload_file, 'Fake signed payload contents.')
    gen.metadata_size = 10

    metadata_signatures = ('1',)

    expected_json = (
        '{"md5_hex": "ad8f67319ca16e691108ca703636b3ad",'
        ' "metadata_signature": "MQ==",'
        ' "metadata_size": 10,'
        ' "sha1_hex": "99zX3vZhTfwRJCi4zGK1A14AY3Y=",'
        ' "sha256_hex": "yZjWgvsNdzclJzJOleQrTjVFBQy810ZlUAU5+i0okME=",'
        ' "version": 2}')
    gen._StorePayloadJson(metadata_signatures)

    # Validate the results.
    self.assertEqual(osutils.ReadFile(gen.description_file), expected_json)

  def testSignPayload(self):
    """Test the overall payload signature process via mox."""
    payload_hash = 'payload_hash'
    metadata_hash = 'metadata_hash'
    payload_sigs = ('payload_sig',)
    metadata_sigs = ('metadata_sig',)

    gen = self._GetStdGenerator(payload=self.delta_payload, work_dir='/work')

    # Set up stubs.
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_GenerateHashes')
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_SignHashes')
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_InsertPayloadSignatures')
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_StoreMetadataSignatures')

    # Record expected calls.
    gen._GenerateHashes().AndReturn((payload_hash, metadata_hash))
    gen._SignHashes([payload_hash, metadata_hash]).AndReturn(
        (payload_sigs, metadata_sigs))
    gen._InsertPayloadSignatures(payload_sigs)
    gen._StoreMetadataSignatures(metadata_sigs)

    # Run the test.
    self.mox.ReplayAll()
    result_payload_sigs, result_metadata_sigs = gen._SignPayload()

    self.assertEqual(payload_sigs, result_payload_sigs)
    self.assertEqual(metadata_sigs, result_metadata_sigs)

  def testCreateSignedDelta(self):
    """Test the overall payload generation process via mox."""
    payload = self.delta_payload
    gen = self._GetStdGenerator(payload=payload, work_dir='/work')

    # Set up stubs.
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_PrepareGenerator')
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_PrepareImage')
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_GenerateUnsignedPayload')
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_SignPayload')
    self.mox.StubOutWithMock(paygen_payload_lib._PaygenPayload,
                             '_StorePayloadJson')

    # Record expected calls.
    gen._PrepareGenerator()
    gen._PrepareImage(payload.tgt_image, gen.tgt_image_file)
    gen._PrepareImage(payload.src_image, gen.src_image_file)
    gen._GenerateUnsignedPayload()
    gen._SignPayload().AndReturn((['payload_sigs'], ['metadata_sigs']))
    gen._StorePayloadJson(['metadata_sigs'])

    # Run the test.
    self.mox.ReplayAll()
    gen._Create()

  def testUploadResults(self):
    """Test the overall payload generation process via mox."""
    gen_sign = self._GetStdGenerator(work_dir='/work', sign=True)
    gen_nosign = self._GetStdGenerator(work_dir='/work', sign=False)

    # Set up stubs.
    self.mox.StubOutWithMock(urilib, 'Copy')
    self.mox.StubOutWithMock(urilib, 'ListFiles')

    # Record signed calls.
    urilib.Copy('/work/delta.bin.signed',
                'gs://full_old_foo/boo')
    urilib.Copy('/work/delta.bin.signed.metadata-signature',
                'gs://full_old_foo/boo.metadata-signature')
    urilib.Copy('/work/delta.log',
                'gs://full_old_foo/boo.log')
    urilib.Copy('/work/delta.json',
                'gs://full_old_foo/boo.json')

    # Record unsigned calls.
    urilib.Copy('/work/delta.bin',
                'gs://full_old_foo/boo')
    urilib.Copy('/work/delta.log',
                'gs://full_old_foo/boo.log')
    urilib.Copy('/work/delta.json',
                'gs://full_old_foo/boo.json')

    # Run the test.
    self.mox.ReplayAll()
    gen_sign._UploadResults()
    gen_nosign._UploadResults()

  def testDefaultPayloadUri(self):
    """Test paygen_payload_lib.DefaultPayloadUri."""

    # Test a Full Payload
    result = paygen_payload_lib.DefaultPayloadUri(self.full_payload,
                                                  random_str='abc123')
    self.assertEqual(
        result,
        'gs://chromeos-releases/dev-channel/x86-alex/1620.0.0/payloads/'
        'chromeos_1620.0.0_x86-alex_dev-channel_full_mp-v3.bin-abc123.signed')

    # Test a Delta Payload
    result = paygen_payload_lib.DefaultPayloadUri(self.delta_payload,
                                                  random_str='abc123')
    self.assertEqual(
        result,
        'gs://chromeos-releases/dev-channel/x86-alex/4171.0.0/payloads/'
        'chromeos_1620.0.0-4171.0.0_x86-alex_dev-channel_delta_mp-v3.bin-'
        'abc123.signed')

    # Test changing channel, board, and keys
    src_image = gspaths.Image(
        channel='dev-channel',
        board='x86-alex',
        version='3588.0.0',
        key='premp')
    tgt_image = gspaths.Image(
        channel='stable-channel',
        board='x86-alex-he',
        version='3590.0.0',
        key='mp-v3')
    payload = gspaths.Payload(src_image=src_image, tgt_image=tgt_image)

    result = paygen_payload_lib.DefaultPayloadUri(payload,
                                                  random_str='abc123')
    self.assertEqual(
        result,
        'gs://chromeos-releases/stable-channel/x86-alex-he/3590.0.0/payloads/'
        'chromeos_3588.0.0-3590.0.0_x86-alex-he_stable-channel_delta_mp-v3.bin-'
        'abc123.signed')

  def testFillInPayloadUri(self):
    """Test filling in the payload URI of a gspaths.Payload object."""
    # Assert that it doesn't change if already present.
    pre_uri = self.full_payload.uri
    paygen_payload_lib.FillInPayloadUri(self.full_payload,
                                        random_str='abc123')
    self.assertEqual(self.full_payload.uri,
                     pre_uri)

    # Test that it does change if not present.
    payload = gspaths.Payload(tgt_image=self.old_image)
    paygen_payload_lib.FillInPayloadUri(payload,
                                        random_str='abc123')
    self.assertEqual(
        payload.uri,
        'gs://chromeos-releases/dev-channel/x86-alex/1620.0.0/payloads/'
        'chromeos_1620.0.0_x86-alex_dev-channel_full_mp-v3.bin-abc123.signed')

  def testFindExistingPayloads(self):
    """Test finding already existing payloads."""
    self.mox.StubOutWithMock(urilib, 'ListFiles')

    # Set up the test replay script.
    urilib.ListFiles('gs://chromeos-releases/dev-channel/x86-alex/1620.0.0/'
                     'payloads/chromeos_1620.0.0_x86-alex_dev-channel_full_'
                     'mp-v3.bin-*.signed').AndReturn(['foo_result'])

    # Run the test verification.
    self.mox.ReplayAll()

    self.assertEqual(
        paygen_payload_lib.FindExistingPayloads(self.full_payload),
        ['foo_result'])

  def testFindCacheDir(self):
    """Test calculating the location of the cache directory."""
    result = paygen_payload_lib.FindCacheDir()

    # The correct result is based on the system cache directory, which changes.
    # Ensure it ends with the right directory name.
    self.assertEqual(os.path.basename(result), 'paygen_cache')


class PaygenPayloadLibEndToEndTest(PaygenPayloadLibTest):
  """PaygenPayloadLib end-to-end testing."""

  def _EndToEndIntegrationTest(self, tgt_image, src_image, sign):
    """Helper test function for validating end to end payload generation."""
    output_uri = os.path.join(self.tempdir, 'expected_payload_out')
    output_metadata_uri = output_uri + '.metadata-signature'
    output_metadata_json = output_uri + '.json'

    payload = gspaths.Payload(tgt_image=tgt_image,
                              src_image=src_image,
                              uri=output_uri)

    paygen_payload_lib.CreateAndUploadPayload(
        payload=payload,
        cache=self.cache,
        work_dir=self.tempdir,
        au_generator_uri=gspaths.ChromeosReleases.GeneratorUri(
            payload.tgt_image.channel, payload.tgt_image.board, '7587.0.0'),
        sign=sign)

    self.assertExists(output_uri)
    self.assertEqual(os.path.exists(output_metadata_uri), sign)
    self.assertExists(output_metadata_json)

  @cros_test_lib.NetworkTest()
  def testEndToEndIntegrationFull(self):
    """Integration test to generate a full payload for old_image."""
    self._EndToEndIntegrationTest(self.old_image, None, sign=True)

  @cros_test_lib.NetworkTest()
  def testEndToEndIntegrationDelta(self):
    """Integration test to generate a delta payload for N -> N."""
    self._EndToEndIntegrationTest(self.new_image, self.new_image, sign=False)
