# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test signer_payloads_client library."""

from __future__ import print_function

import mock
import os
import shutil
import socket
import tempfile

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import gs
from chromite.lib import gs_unittest

from chromite.lib.paygen import gslock
from chromite.lib.paygen import signer_payloads_client


# pylint: disable=protected-access


class SignerPayloadsClientGoogleStorageTest(gs_unittest.AbstractGSContextTest):
  """Test suite for the class SignerPayloadsClientGoogleStorage."""

  orig_timeout = (
      signer_payloads_client.DELAY_CHECKING_FOR_SIGNER_RESULTS_SECONDS)

  def setUp(self):
    """Setup for tests, and store off some standard expected values."""
    self.hash_names = [
        '1.payload.hash',
        '2.payload.hash',
        '3.payload.hash']

    self.build_uri = ('gs://foo-bucket/foo-channel/foo-board/foo-version/'
                      'payloads/signing/foo-unique')

    # Some tests depend on this timeout. Make it smaller, then restore.
    signer_payloads_client.DELAY_CHECKING_FOR_SIGNER_RESULTS_SECONDS = 0.01

  def tearDown(self):
    """Teardown after tests, and restore values test might adjust."""
    # Some tests modify this timeout. Restore the original value.
    signer_payloads_client.DELAY_CHECKING_FOR_SIGNER_RESULTS_SECONDS = (
        self.orig_timeout)

  def createStandardClient(self):
    """Test helper method to create a client with standard arguments."""

    client = signer_payloads_client.SignerPayloadsClientGoogleStorage(
        'foo-channel',
        'foo-board',
        'foo-version',
        bucket='foo-bucket',
        unique='foo-unique',
        ctx=self.ctx)
    return client

  def testUris(self):
    """Test that the URIs on the client are correct."""

    client = self.createStandardClient()

    expected_build_uri = self.build_uri

    self.assertEquals(
        client.signing_base_dir,
        expected_build_uri)

    self.assertEquals(
        client.archive_uri,
        expected_build_uri + '/payload.hash.tar.bz2')

  def testCleanSignerFilesByKeyset(self):
    """Test the keyset specific cleanup works as expected."""

    hashes = ('hash-1', 'hash-2')
    keyset = 'foo-keys'

    lock_uri = ('gs://foo-bucket/tobesigned/45,foo-channel,foo-board,'
                'foo-version,payloads,signing,foo-unique,'
                'foo-keys.payload.signer.instructions.lock')

    signing_dir = ('gs://foo-bucket/foo-channel/foo-board/foo-version/'
                   'payloads/signing/foo-unique')

    expected_removals = (
        # Signing Request
        'gs://foo-bucket/tobesigned/45,foo-channel,foo-board,foo-version,'
        'payloads,signing,foo-unique,'
        'foo-keys.payload.signer.instructions',

        # Signing Instructions
        signing_dir + '/foo-keys.payload.signer.instructions',

        # Signed Results`
        signing_dir + '/1.payload.hash.foo-keys.signed.bin',
        signing_dir + '/1.payload.hash.foo-keys.signed.bin.md5',
        signing_dir + '/2.payload.hash.foo-keys.signed.bin',
        signing_dir + '/2.payload.hash.foo-keys.signed.bin.md5',
    )

    client = self.createStandardClient()

    # Fake lock failed then acquired.
    lock = self.PatchObject(gslock, 'Lock', autospec=True,
                            side_effect=[gslock.LockNotAcquired(),
                                         mock.MagicMock()])

    # Do the work.
    client._CleanSignerFilesByKeyset(hashes, keyset)

    # Assert locks created with expected lock_uri.
    lock.assert_called_with(lock_uri)

    # Verify all expected files were removed.
    for uri in expected_removals:
      self.gs_mock.assertCommandContains(['rm', uri])

  def testCleanSignerFiles(self):
    """Test that GS cleanup works as expected."""

    hashes = ('hash-1', 'hash-2')
    keysets = ('foo-keys-1', 'foo-keys-2')

    lock_uri1 = ('gs://foo-bucket/tobesigned/45,foo-channel,foo-board,'
                 'foo-version,payloads,signing,foo-unique,'
                 'foo-keys-1.payload.signer.instructions.lock')

    lock_uri2 = ('gs://foo-bucket/tobesigned/45,foo-channel,foo-board,'
                 'foo-version,payloads,signing,foo-unique,'
                 'foo-keys-2.payload.signer.instructions.lock')

    signing_dir = ('gs://foo-bucket/foo-channel/foo-board/foo-version/'
                   'payloads/signing/foo-unique')

    expected_removals = (
        # Signing Request
        'gs://foo-bucket/tobesigned/45,foo-channel,foo-board,foo-version,'
        'payloads,signing,foo-unique,'
        'foo-keys-1.payload.signer.instructions',

        'gs://foo-bucket/tobesigned/45,foo-channel,foo-board,foo-version,'
        'payloads,signing,foo-unique,'
        'foo-keys-2.payload.signer.instructions',

        # Signing Instructions
        signing_dir + '/foo-keys-1.payload.signer.instructions',
        signing_dir + '/foo-keys-2.payload.signer.instructions',

        # Signed Results
        signing_dir + '/1.payload.hash.foo-keys-1.signed.bin',
        signing_dir + '/1.payload.hash.foo-keys-1.signed.bin.md5',
        signing_dir + '/2.payload.hash.foo-keys-1.signed.bin',
        signing_dir + '/2.payload.hash.foo-keys-1.signed.bin.md5',
        signing_dir + '/1.payload.hash.foo-keys-2.signed.bin',
        signing_dir + '/1.payload.hash.foo-keys-2.signed.bin.md5',
        signing_dir + '/2.payload.hash.foo-keys-2.signed.bin',
        signing_dir + '/2.payload.hash.foo-keys-2.signed.bin.md5',
    )

    client = self.createStandardClient()

    # Fake lock failed then acquired.
    lock = self.PatchObject(gslock, 'Lock', autospec=True)

    # Do the work.
    client._CleanSignerFiles(hashes, keysets)

    # Check created with lock_uri1, lock_uri2.
    self.assertEqual(lock.call_args_list,
                     [mock.call(lock_uri1), mock.call(lock_uri2)])

    # Verify expected removals.
    for uri in expected_removals:
      self.gs_mock.assertCommandContains(['rm', uri])

    self.gs_mock.assertCommandContains(['rm', signing_dir])

  def testCreateInstructionsUri(self):
    """Test that the expected instructions URI is correct."""

    client = self.createStandardClient()

    signature_uri = client._CreateInstructionsURI('keyset_foo')

    expected_signature_uri = (
        self.build_uri +
        '/keyset_foo.payload.signer.instructions')

    self.assertEqual(signature_uri, expected_signature_uri)

  def testCreateHashNames(self):
    """Test that the expected hash names are generated."""

    client = self.createStandardClient()

    hash_names = client._CreateHashNames(3)

    expected_hash_names = self.hash_names

    self.assertEquals(hash_names, expected_hash_names)

  def testCreateSignatureURIs(self):
    """Test that the expected signature URIs are generated."""

    client = self.createStandardClient()

    signature_uris = client._CreateSignatureURIs(self.hash_names,
                                                 'keyset_foo')

    expected_signature_uris = [
        self.build_uri + '/1.payload.hash.keyset_foo.signed.bin',
        self.build_uri + '/2.payload.hash.keyset_foo.signed.bin',
        self.build_uri + '/3.payload.hash.keyset_foo.signed.bin',
    ]

    self.assertEquals(signature_uris, expected_signature_uris)

  def testCreateArchive(self):
    """Test that we can correctly archive up hash values for the signer."""

    client = self.createStandardClient()

    tmp_dir = None
    hashes = ['Hash 1', 'Hash 2', 'Hash 3']

    try:
      with tempfile.NamedTemporaryFile() as archive_file:
        client._CreateArchive(archive_file.name, hashes, self.hash_names)

        # Make sure the archive file created exists
        self.assertExists(archive_file.name)

        tmp_dir = tempfile.mkdtemp()

        cmd = ['tar', '-xjf', archive_file.name]
        cros_build_lib.RunCommand(
            cmd, redirect_stdout=True, redirect_stderr=True, cwd=tmp_dir)

        # Check that the expected (and only the expected) contents are present
        extracted_file_names = os.listdir(tmp_dir)
        self.assertEquals(len(extracted_file_names), len(self.hash_names))
        for name in self.hash_names:
          self.assertTrue(name in extracted_file_names)

        # Make sure each file has the expected contents
        for h, hash_name in zip(hashes, self.hash_names):
          with open(os.path.join(tmp_dir, hash_name), 'r') as f:
            self.assertEqual([h], f.readlines())

    finally:
      # Clean up at the end of the test
      if tmp_dir:
        shutil.rmtree(tmp_dir)

  def testCreateInstructions(self):
    """Test that we can correctly create signer instructions."""

    client = self.createStandardClient()

    instructions = client._CreateInstructions(self.hash_names, 'keyset_foo')

    expected_instructions = """
# Auto-generated instruction file for signing payload hashes.

[insns]
generate_metadata = false
keyset = keyset_foo
channel = foo

input_files = %s
output_names = @BASENAME@.@KEYSET@.signed

[general]
archive = metadata-disable.instructions
type = update_payload
board = foo-board

archive = payload.hash.tar.bz2

# We reuse version for version rev because we may not know the
# correct versionrev "R24-1.2.3"
version = foo-version
versionrev = foo-version
""" % ' '.join(['1.payload.hash',
                '2.payload.hash',
                '3.payload.hash'])

    self.assertEquals(instructions, expected_instructions)

  def testSignerRequestUri(self):
    """Test that we can create signer request URI."""

    client = self.createStandardClient()

    instructions_uri = client._CreateInstructionsURI('foo_keyset')
    signer_request_uri = client._SignerRequestUri(instructions_uri)

    expected = ('gs://foo-bucket/tobesigned/45,foo-channel,foo-board,'
                'foo-version,payloads,signing,foo-unique,'
                'foo_keyset.payload.signer.instructions')

    self.assertEquals(signer_request_uri, expected)

  def testWaitForSignaturesInstant(self):
    """Test that we can correctly wait for a list of URIs to be created."""
    uris = ['foo', 'bar', 'is']

    # All Urls exist.
    exists = self.PatchObject(self.ctx, 'Exists', returns=True)

    client = self.createStandardClient()

    self.assertTrue(client._WaitForSignatures(uris, timeout=0.02))

    # Make sure it really looked for every URL listed.
    self.assertEqual(exists.call_args_list,
                     [mock.call(u) for u in uris])

  def testWaitForSignaturesNever(self):
    """Test that we can correctly timeout waiting for a list of URIs."""
    uris = ['foo', 'bar', 'is']

    # Default mock GSContext behavior is nothing Exists.
    client = self.createStandardClient()
    self.assertFalse(client._WaitForSignatures(uris, timeout=0.02))

    # We don't care which URLs it checked, since it doesn't have to check
    # them all in this case.


class SignerPayloadsClientIntegrationTest(cros_test_lib.TestCase):
  """Test suite integration with live signer servers."""

  def setUp(self):
    # This is in the real production chromeos-releases, but the listed
    # build has never, and will never exist.
    self.client = signer_payloads_client.SignerPayloadsClientGoogleStorage(
        'test-channel',
        'crostools-client',
        'Rxx-Ryy')

  @cros_test_lib.NetworkTest()
  def testDownloadSignatures(self):
    """Test that we can correctly download a list of URIs."""
    uris = ['gs://chromeos-releases-test/sigining-test/foo',
            'gs://chromeos-releases-test/sigining-test/bar']

    downloads = self.client._DownloadSignatures(uris)
    self.assertEquals(downloads, ['FooSig\r\n\r', 'BarSig'])

  @cros_test_lib.NetworkTest()
  def testGetHashSignatures(self):
    """Integration test that talks to the real signer with test hashes."""
    ctx = gs.GSContext()

    unique_id = '%s.%d' % (socket.gethostname(), os.getpid())
    clean_uri = ('gs://chromeos-releases/test-channel/%s/'
                 'crostools-client/**') % unique_id

    # Cleanup before we start
    ctx.Remove(clean_uri, ignore_missing=True)

    try:
      hashes = ['0' * 32,
                '1' * 32,
                ('29834370e415b3124a926c903906f18b'
                 '3d52e955147f9e6accd67e9512185a63')]

      keysets = ['update_signer']

      expected_sigs_hex = (
          ('ba4c7a86b786c609bf6e4c5fb9c47525608678caa532bea8acc457aa6dd32b43'
           '5f094b331182f2e167682916990c40ff7b6b0128de3fa45ad0fd98041ec36d6f'
           '63b867bcf219804200616590a41a727c2685b48340efb4b480f1ef448fc7bc3f'
           'b1c4b53209e950ecc721b07a52a41d9c025fd25602340c93d5295211308caa29'
           'a03ed18516cf61411c508097d5b47620d643ed357b05213b2b9fa3a3f938d6c4'
           'f52b85c3f9774edc376902458344d1c1cd72bc932f033c076c76fee2400716fe'
           '652306871ba923021ce245e0c778ad9e0e50e87a169b2aea338c4dc8b5c0c716'
           'aabfb6133482e8438b084a09503db27ca546e910f8938f7805a8a76a3b0d0241',),

          ('2d909ca5b33a7fb6f2323ca0bf9de2e4f2266c73da4b6948a517dffa96783e08'
           'ca36411d380f6e8a20011f599d8d73576b2a141a57c0873d089726e24f62c7e0'
           '346ba5fbde68414b0f874b627fb1557a6e9658c8fac96c54f458161ea770982b'
           'fa9fe514120635e5ccb32e8219b9069cb0bf8063fba48d60d649c5af203cccef'
           'ca5dbc2191f81f0215edbdee4ec8c1553e69b83036aca3e840227d317ff6cf8b'
           '968c973f698db1ce59f6871303dcdbe839400c5df4d2e6e505d68890010a4459'
           '6ca9fee77f4db6ea3448d98018437c319fc8c5f4603ef94b04e3a4eafa206b73'
           '91a2640d43128310285bc0f1c7e5060d37c433d663b1c6f01110b9a43f2a74f4',),

          ('23791c99ab937f1ae5d4988afc9ceca39c290ac90e3da9f243f9a0b1c86c3c32'
           'ab7241d43dfc233da412bab989cf02f15a01fe9ea4b2dc7dc9182117547836d6'
           '9310af3aa005ee3a6deb9602bc676dcc103bf3f7831d64ab844b4785c5c8b4b1'
           '4467e6b5ab6bf34c12f7534e0d5140151c8f28e8276e703dd6332c2bab9e7f4a'
           '495215998ff56e476b81bd6b8d765e1f87da50c22cd52c9afa8c43a6528ab898'
           '6d7a273d9136d5aff5c4d95985d16eeec7380539ef963e0784a0de42b42890df'
           'c83702179f69f5c6eca4630807fbc4ab6241017e0942b15feada0b240e9729bf'
           '33bf456bd419da63302477e147963550a45c6cf60925ff48ad7b309fa158dcb2',))

      expected_sigs = [[sig[0].decode('hex')] for sig in expected_sigs_hex]

      all_signatures = self.client.GetHashSignatures(hashes, keysets)

      self.assertEquals(all_signatures, expected_sigs)
      self.assertRaises(gs.GSNoSuchKey, ctx.List, clean_uri)

    finally:
      # Cleanup when we are over
      ctx.Remove(clean_uri, ignore_missing=True)
