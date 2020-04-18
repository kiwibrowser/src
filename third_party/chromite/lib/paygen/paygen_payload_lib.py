# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Hold the functions that do the real work generating payloads."""

from __future__ import print_function

import base64
import datetime
import json
import os
import shutil
import tempfile

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib.paygen import dryrun_lib
from chromite.lib.paygen import filelib
from chromite.lib.paygen import gspaths
from chromite.lib.paygen import signer_payloads_client
from chromite.lib.paygen import urilib
from chromite.lib.paygen import utils


DESCRIPTION_FILE_VERSION = 2


class Error(Exception):
  """Base class for payload generation errors."""


class UnexpectedSignerResultsError(Error):
  """This is raised when signer results don't match our expectations."""


class PayloadVerificationError(Error):
  """Raised when the generated payload fails to verify."""


class _PaygenPayload(object):
  """Class to manage the process of generating and signing a payload."""

  # What keys do we sign payloads with, and what size are they?
  PAYLOAD_SIGNATURE_KEYSETS = ('update_signer',)
  PAYLOAD_SIGNATURE_SIZES_BYTES = (2048 / 8,)  # aka 2048 bits in bytes.

  TEST_IMAGE_NAME = 'chromiumos_test_image.bin'
  RECOVERY_IMAGE_NAME = 'chromiumos_recovery_image.bin'
  BASE_IMAGE_NAME = 'chromiumos_base_image.bin'

  _KERNEL = 'kernel'
  _ROOTFS = 'rootfs'

  def __init__(self, payload, cache, work_dir, sign, verify,
               au_generator_uri_override, dry_run=False):
    """Init for _PaygenPayload.

    Args:
      payload: An instance of gspaths.Payload describing the payload to
               generate.
      cache: An instance of DownloadCache for retrieving files.
      work_dir: A working directory for output files. Can NOT be shared.
      sign: Boolean saying if the payload should be signed (normally, you do).
      verify: whether the payload should be verified after being generated
      au_generator_uri_override: URI to override standard au_generator.zip
          rules.
      dry_run: do not do any actual work
    """
    self.payload = payload
    self.cache = cache
    self.work_dir = work_dir
    self._verify = verify
    self._au_generator_uri_override = au_generator_uri_override
    self._drm = dryrun_lib.DryRunMgr(dry_run)

    self.generator_dir = os.path.join(work_dir, 'au-generator')
    self.src_image_file = os.path.join(work_dir, 'src_image.bin')
    self.tgt_image_file = os.path.join(work_dir, 'tgt_image.bin')

    self.payload_file = os.path.join(work_dir, 'delta.bin')
    self.log_file = os.path.join(work_dir, 'delta.log')
    self.description_file = os.path.join(work_dir, 'delta.json')
    self.metadata_size_file = os.path.join(work_dir, 'metadata_size.txt')
    self.metadata_size = 0

    self.signer = None

    # Names to pass to cros_generate_update_payload for extracting old/new
    # kernel/rootfs partitions.
    self.partition_names = (self._KERNEL, self._ROOTFS)
    self.tgt_partitions = {}
    self.src_partitions = {}
    for part_name in self.partition_names:
      self.tgt_partitions[part_name] = os.path.join(self.work_dir,
                                                    'new_' + part_name + '.dat')
      self.src_partitions[part_name] = os.path.join(self.work_dir,
                                                    'old_' + part_name + '.dat')

    if sign:
      self.signed_payload_file = self.payload_file + '.signed'
      self.metadata_signature_file = self._MetadataUri(self.signed_payload_file)

      self.signer = signer_payloads_client.SignerPayloadsClientGoogleStorage(
          payload.tgt_image.channel,
          payload.tgt_image.board,
          payload.tgt_image.version,
          payload.tgt_image.bucket)

  def _MetadataUri(self, uri):
    """Given a payload uri, find the uri for the metadata signature."""
    return uri + '.metadata-signature'

  def _LogsUri(self, uri):
    """Given a payload uri, find the uri for the logs."""
    return uri + '.log'

  def _JsonUri(self, uri):
    """Given a payload uri, find the uri for the json payload description."""
    return uri + '.json'

  def _PrepareGenerator(self):
    """Download, and extract au-generator.zip into self.generator_dir."""
    if self._au_generator_uri_override:
      generator_uri = self._au_generator_uri_override
    else:
      generator_uri = gspaths.ChromeosReleases.GeneratorUri(
          self.payload.tgt_image.channel,
          self.payload.tgt_image.board,
          self.payload.tgt_image.version,
          self.payload.tgt_image.bucket)

    logging.info('Preparing au-generator.zip from %s.', generator_uri)

    # Extract zipped delta generator files to the expected directory.
    tmp_zip = self.cache.GetFileInTempFile(generator_uri)
    cros_build_lib.RunCommand(
        ['unzip', '-o', '-d', self.generator_dir, tmp_zip.name],
        redirect_stdout=True, redirect_stderr=True)
    tmp_zip.close()

  def _RunGeneratorCmd(self, cmd):
    """Wrapper for RunCommand for programs in self.generator_dir.

    Adjusts the program name for the current self.au_generator directory, and
    sets up the special requirements needed for these 'out of chroot'
    programs. Will automatically log the command output if execution resulted
    in a nonzero exit code. Note that the command's stdout and stderr are
    combined into a single string. This also sets the TMPDIR variable
    accordingly in the spawned process' environment.

    Args:
      cmd: Program and argument list in a list. ['delta_generator', '--help']

    Returns:
      The output of the executed command.

    Raises:
      cros_build_lib.RunCommandError if the command exited with a nonzero code.
    """

    # Run the command.
    result = cros_build_lib.RunCommand(
        cmd,
        redirect_stdout=True,
        enter_chroot=True,
        combine_stdout_stderr=True,
        error_code_ok=True)

    # Dump error output and raise an exception if things went awry.
    if result.returncode:
      logging.error('Nonzero exit code (%d), dumping command output:\n%s',
                    result.returncode, result.output)
      raise cros_build_lib.RunCommandError(
          'Command failed: %s (cwd=%s)' % ' '.join(cmd), result)

    self._StoreLog('Output of command: ' + ' '.join(cmd))
    self._StoreLog(result.output)

  @staticmethod
  def _BuildArg(flag, dict_obj, key, default=None):
    """Returns a command-line argument iff its value is present in a dictionary.

    Args:
      flag: the flag name to use with the argument value, e.g. --foo; if None
            or an empty string, no flag will be used
      dict_obj: a dictionary mapping possible keys to values
      key: the key of interest; e.g. 'foo'
      default: a default value to use if key is not in dict_obj (optional)

    Returns:
      If dict_obj[key] contains a non-False value or default is non-False,
      returns a list containing the flag and value arguments (e.g. ['--foo',
      'bar']), unless flag is empty/None, in which case returns a list
      containing only the value argument (e.g.  ['bar']). Otherwise, returns an
      empty list.
    """
    arg_list = []
    val = dict_obj.get(key) or default
    if val:
      arg_list = [str(val)]
      if flag:
        arg_list.insert(0, flag)

    return arg_list

  def _PrepareImage(self, image, image_file):
    """Download an prepare an image for delta generation.

    Preparation includes downloading, extracting and converting the image into
    an on-disk format, as necessary.

    Args:
      image: an object representing the image we're processing, either
             UnsignedImageArchive or Image type from gspaths module.
      image_file: file into which the prepared image should be copied.
    """

    logging.info('Preparing image from %s as %s', image.uri, image_file)

    # Figure out what we're downloading and how to handle it.
    image_handling_by_type = {
        'signed': (None, True),
        'test': (self.TEST_IMAGE_NAME, False),
        'recovery': (self.RECOVERY_IMAGE_NAME, True),
        'base': (self.BASE_IMAGE_NAME, True),
    }
    if gspaths.IsImage(image):
      # No need to extract.
      extract_file = None
    elif gspaths.IsUnsignedImageArchive(image):
      extract_file, _ = image_handling_by_type[image.get('image_type',
                                                         'signed')]
    else:
      raise Error('Unknown image type %s' % type(image))

    # Are we donwloading an archive that contains the image?
    if extract_file:
      # Archive will be downloaded to a temporary location.
      with tempfile.NamedTemporaryFile(
          prefix='image-archive-', suffix='.tar.xz', dir=self.work_dir,
          delete=False) as temp_file:
        download_file = temp_file.name
    else:
      download_file = image_file

    # Download the image file or archive.
    self.cache.GetFileCopy(image.uri, download_file)

    # If we downloaded an archive, extract the image file from it.
    if extract_file:
      cmd = ['tar', '-xJf', download_file, extract_file]
      cros_build_lib.RunCommand(cmd, cwd=self.work_dir)

      # Rename it into the desired image name.
      shutil.move(os.path.join(self.work_dir, extract_file), image_file)

      # It's safe to delete the archive at this point.
      os.remove(download_file)

  def _CheckPartitionFiles(self):
    """Makes sure the extracted partition files exist."""
    for part_name in self.partition_names:
      if not os.path.isfile(self.tgt_partitions[part_name]):
        raise PayloadVerificationError('Failed to extract partition (%s)' %
                                       self.tgt_partitions[part_name])
    if self.payload.src_image:
      for part_name in self.partition_names:
        if not os.path.isfile(self.src_partitions[part_name]):
          raise PayloadVerificationError('Failed to extract partition (%s)' %
                                         self.src_partitions[part_name])

  def _GenerateUnsignedPayload(self):
    """Generate the unsigned delta into self.payload_file."""
    # Note that the command run here requires sudo access.

    logging.info('Generating unsigned payload as %s', self.payload_file)

    tgt_image = self.payload.tgt_image
    cmd = ['cros_generate_update_payload',
           '--output', path_util.ToChrootPath(self.payload_file),
           '--image', path_util.ToChrootPath(self.tgt_image_file),
           '--channel', tgt_image.channel,
           '--board', tgt_image.board,
           '--version', tgt_image.version,
           '--kern_path',
           path_util.ToChrootPath(self.tgt_partitions[self._KERNEL]),
           '--root_path',
           path_util.ToChrootPath(self.tgt_partitions[self._ROOTFS])]
    cmd += self._BuildArg('--key', tgt_image, 'key', default='test')
    cmd += self._BuildArg('--build_channel', tgt_image, 'image_channel',
                          default=tgt_image.channel)
    cmd += self._BuildArg('--build_version', tgt_image, 'image_version',
                          default=tgt_image.version)

    if self.payload.src_image:
      src_image = self.payload.src_image
      cmd += ['--src_image', path_util.ToChrootPath(self.src_image_file),
              '--src_channel', src_image.channel,
              '--src_board', src_image.board,
              '--src_version', src_image.version,
              '--src_kern_path',
              path_util.ToChrootPath(self.src_partitions[self._KERNEL]),
              '--src_root_path',
              path_util.ToChrootPath(self.src_partitions[self._ROOTFS])]
      cmd += self._BuildArg('--src_key', src_image, 'key', default='test')
      cmd += self._BuildArg('--src_build_channel', src_image, 'image_channel',
                            default=src_image.channel)
      cmd += self._BuildArg('--src_build_version', src_image, 'image_version',
                            default=src_image.version)

    self._RunGeneratorCmd(cmd)
    self._CheckPartitionFiles()

  def _GenerateHashes(self):
    """Generate a payload hash and a metadata hash.

    Works from an unsigned update payload.

    Returns:
      payload_hash as a string, metadata_hash as a string.
    """
    logging.info('Calculating hashes on %s.', self.payload_file)

    # How big will the signatures be.
    signature_sizes = [str(size) for size in self.PAYLOAD_SIGNATURE_SIZES_BYTES]

    with tempfile.NamedTemporaryFile(dir=self.work_dir) as \
         payload_hash_file, \
         tempfile.NamedTemporaryFile(dir=self.work_dir) as \
         metadata_hash_file:

      cmd = ['brillo_update_payload', 'hash',
             '--unsigned_payload', path_util.ToChrootPath(self.payload_file),
             '--payload_hash_file',
             path_util.ToChrootPath(payload_hash_file.name),
             '--metadata_hash_file',
             path_util.ToChrootPath(metadata_hash_file.name),
             '--signature_size', ':'.join(signature_sizes)]

      self._RunGeneratorCmd(cmd)
      return payload_hash_file.read(), metadata_hash_file.read()

  def _ReadMetadataSizeFile(self):
    """Discover the metadata size.

    The payload generator creates the file containing the metadata size. So we
    read it and make sure it is a proper value.
    """
    if not os.path.isfile(self.metadata_size_file):
      raise Error('Metadata size file %s does not exist' %
                  self.metadata_size_file)

    try:
      self.metadata_size = int(osutils.ReadFile(self.metadata_size_file)
                               .strip())
    except ValueError:
      raise Error('Invalid metadata size %s' % self.metadata_size)

  def _GenerateSignerResultsError(self, format_str, *args):
    """Helper for reporting errors with signer results."""
    msg = format_str % args
    logging.error(msg)
    raise UnexpectedSignerResultsError(msg)

  def _SignHashes(self, hashes):
    """Get the signer to sign the hashes with the update payload key via GS.

    May sign each hash with more than one key, based on how many keysets are
    required.

    Args:
      hashes: List of hashes to be signed.

    Returns:
      List of lists which contain each signed hash.
      [[hash_1_sig_1, hash_1_sig_2], [hash_2_sig_1, hash_2_sig_2]]
    """
    logging.info('Signing payload hashes with %s.',
                 ', '.join(self.PAYLOAD_SIGNATURE_KEYSETS))

    # Results look like:
    #  [[hash_1_sig_1, hash_1_sig_2], [hash_2_sig_1, hash_2_sig_2]]
    hashes_sigs = self.signer.GetHashSignatures(
        hashes,
        keysets=self.PAYLOAD_SIGNATURE_KEYSETS)

    if hashes_sigs is None:
      self._GenerateSignerResultsError('Signing of hashes failed')
    if len(hashes_sigs) != len(hashes):
      self._GenerateSignerResultsError(
          'Count of hashes signed (%d) != Count of hashes (%d).',
          len(hashes_sigs),
          len(hashes))

    # Make sure that the results we get back the expected number of signatures.
    for hash_sigs in hashes_sigs:
      # Make sure each hash has the right number of signatures.
      if len(hash_sigs) != len(self.PAYLOAD_SIGNATURE_SIZES_BYTES):
        self._GenerateSignerResultsError(
            'Signature count (%d) != Expected signature count (%d)',
            len(hash_sigs),
            len(self.PAYLOAD_SIGNATURE_SIZES_BYTES))

      # Make sure each hash signature is the expected size.
      for sig, sig_size in zip(hash_sigs, self.PAYLOAD_SIGNATURE_SIZES_BYTES):
        if len(sig) != sig_size:
          self._GenerateSignerResultsError(
              'Signature size (%d) != expected size(%d)',
              len(sig),
              sig_size)

    return hashes_sigs

  def _InsertPayloadSignatures(self, signatures):
    """Put payload signatures into the payload they sign.

    Args:
      signatures: List of signatures for the payload.
    """
    logging.info('Inserting payload signatures into %s.',
                 self.signed_payload_file)

    signature_files = [utils.CreateTempFileWithContents(s,
                                                        base_dir=self.work_dir)
                       for s in signatures]
    signature_file_names = [path_util.ToChrootPath(f.name)
                            for f in signature_files]

    cmd = ['delta_generator',
           '-in_file=' + path_util.ToChrootPath(self.payload_file),
           '-signature_file=' + ':'.join(signature_file_names),
           '-out_file=' + path_util.ToChrootPath(self.signed_payload_file),
           '-out_metadata_size_file=' +
           path_util.ToChrootPath(self.metadata_size_file)]

    self._RunGeneratorCmd(cmd)
    self._ReadMetadataSizeFile()

    for f in signature_files:
      f.close()

  def _StoreMetadataSignatures(self, signatures):
    """Store metadata signatures related to the payload.

    Our current format for saving metadata signatures only supports a single
    signature at this time.

    Args:
      signatures: A list of metadata signatures in binary string format.
    """
    if len(signatures) != 1:
      self._GenerateSignerResultsError(
          'Received %d metadata signatures, only a single signature supported.',
          len(signatures))

    logging.info('Saving metadata signatures in %s.',
                 self.metadata_signature_file)

    encoded_signature = base64.b64encode(signatures[0])

    with open(self.metadata_signature_file, 'w+') as f:
      f.write(encoded_signature)

  def _StorePayloadJson(self, metadata_signatures):
    """Generate the payload description json file.

    The payload description contains a dictionary with the following
    fields populated.

    {
      "version": 2,
      "sha1_hex": <payload sha1 hash as a hex encoded string>,
      "sha256_hex": <payload sha256 hash as a hex encoded string>,
      "md5_hex": <payload md5 hash as a hex encoded string>,
      "metadata_size": <integer of payload metadata covered by signature>,
      "metadata_signature": <metadata signature as base64 encoded string or nil>
    }

    Args:
      metadata_signatures: A list of signatures in binary string format.
    """
    # Decide if we use the signed or unsigned payload file.
    payload_file = self.payload_file
    if self.signer:
      payload_file = self.signed_payload_file

    # Locate everything we put in the json.
    sha1_hex, sha256_hex = filelib.ShaSums(payload_file)
    md5_hex = filelib.MD5Sum(payload_file)

    metadata_signature = None
    if metadata_signatures:
      if len(metadata_signatures) != 1:
        self._GenerateSignerResultsError(
            'Received %d metadata signatures, only one supported.',
            len(metadata_signatures))
      metadata_signature = base64.b64encode(metadata_signatures[0])

    # Bundle it up in a map matching the Json format.
    # Increment DESCRIPTION_FILE_VERSION, if changing this map.
    payload_map = {
        'version': DESCRIPTION_FILE_VERSION,
        'sha1_hex': sha1_hex,
        'sha256_hex': sha256_hex,
        'md5_hex': md5_hex,
        'metadata_size': self.metadata_size,
        'metadata_signature': metadata_signature,
    }

    # Convert to Json.
    payload_json = json.dumps(payload_map, sort_keys=True)

    # Write out the results.
    osutils.WriteFile(self.description_file, payload_json)

  def _StoreLog(self, log):
    """Store any log related to the payload.

    Write out the log to a known file name. Mostly in its own function
    to simplify unittest mocks.

    Args:
      log: The delta logs as a single string.
    """
    osutils.WriteFile(self.log_file, log, mode='a')

  def _SignPayload(self):
    """Wrap all the steps for signing an existing payload.

    Returns:
      List of payload signatures, List of metadata signatures.
    """
    # Create hashes to sign.
    payload_hash, metadata_hash = self._GenerateHashes()

    # Sign them.
    # pylint: disable=unpacking-non-sequence
    payload_signatures, metadata_signatures = self._SignHashes(
        [payload_hash, metadata_hash])
    # pylint: enable=unpacking-non-sequence

    # Insert payload signature(s).
    self._InsertPayloadSignatures(payload_signatures)

    # Store Metadata signature(s).
    self._StoreMetadataSignatures(metadata_signatures)

    return (payload_signatures, metadata_signatures)

  def _Create(self):
    """Create a given payload, if it doesn't already exist."""

    logging.info('Generating %s payload %s',
                 'delta' if self.payload.src_image else 'full', self.payload)

    # Fetch and extract the delta generator.
    self._PrepareGenerator()

    # Fetch and prepare the tgt image.
    self._PrepareImage(self.payload.tgt_image, self.tgt_image_file)

    # Fetch and prepare the src image.
    if self.payload.src_image:
      self._PrepareImage(self.payload.src_image, self.src_image_file)

    # Generate the unsigned payload.
    self._GenerateUnsignedPayload()

    # Sign the payload, if needed.
    metadata_signatures = None
    if self.signer:
      _, metadata_signatures = self._SignPayload()

    # Store hash and signatures json.
    self._StorePayloadJson(metadata_signatures)

  def _VerifyPayload(self):
    """Checks the integrity of the generated payload.

    Raises:
      PayloadVerificationError when the payload fails to verify.
    """
    if self.signer:
      payload_file_name = self.signed_payload_file
      metadata_sig_file_name = self.metadata_signature_file
    else:
      payload_file_name = self.payload_file
      metadata_sig_file_name = None

    is_delta = bool(self.payload.src_image)

    logging.info('Applying %s payload and verifying result',
                 'delta' if is_delta else 'full')

    # This command checks both the payload integrity and applies the payload
    # to source and target partitions.
    cmd = ['check_update_payload', '--check',
           '--type', 'delta' if is_delta else 'full',
           '--disabled_tests', 'move-same-src-dst-block',
           '--dst_kern',
           path_util.ToChrootPath(self.tgt_partitions[self._KERNEL]),
           '--dst_root',
           path_util.ToChrootPath(self.tgt_partitions[self._ROOTFS])]
    if metadata_sig_file_name:
      cmd += ['--meta-sig', path_util.ToChrootPath(metadata_sig_file_name)]

    cmd += ['--metadata-size', str(self.metadata_size)]

    if is_delta:
      cmd += ['--src_kern',
              path_util.ToChrootPath(self.src_partitions[self._KERNEL]),
              '--src_root',
              path_util.ToChrootPath(self.src_partitions[self._ROOTFS])]
    cmd += [path_util.ToChrootPath(payload_file_name)]

    self._RunGeneratorCmd(cmd)

  def _UploadResults(self):
    """Copy the payload generation results to the specified destination."""

    logging.info('Uploading payload to %s.', self.payload.uri)

    # Deliver the payload to the final location.
    if self.signer:
      urilib.Copy(self.signed_payload_file, self.payload.uri)
      urilib.Copy(self.metadata_signature_file,
                  self._MetadataUri(self.payload.uri))
    else:
      urilib.Copy(self.payload_file, self.payload.uri)

    # Upload payload related artifacts.
    urilib.Copy(self.log_file, self._LogsUri(self.payload.uri))
    urilib.Copy(self.description_file, self._JsonUri(self.payload.uri))

  def Run(self):
    """Create, verify and upload the results."""
    self._drm(self._Create)
    if self._verify:
      self._drm(self._VerifyPayload)
    self._drm(self._UploadResults)


def DefaultPayloadUri(payload, random_str=None):
  """Compute the default output URI for a payload.

  For a glob that matches all potential URIs for this
  payload, pass in a random_str of '*'.

  Args:
    payload: gspaths.Payload instance.
    random_str: A hook to force a specific random_str. None means generate it.

  Returns:
    Default URI for the payload.
  """
  src_version = None
  if payload.src_image:
    src_version = payload.src_image['version']

  if gspaths.IsImage(payload.tgt_image):
    # Signed payload.
    return gspaths.ChromeosReleases.PayloadUri(
        channel=payload.tgt_image.channel,
        board=payload.tgt_image.board,
        version=payload.tgt_image.version,
        random_str=random_str,
        key=payload.tgt_image.key,
        image_channel=payload.tgt_image.image_channel,
        image_version=payload.tgt_image.image_version,
        src_version=src_version,
        bucket=payload.tgt_image.bucket)
  elif gspaths.IsUnsignedImageArchive(payload.tgt_image):
    # Unsigned test payload.
    return gspaths.ChromeosReleases.PayloadUri(
        channel=payload.tgt_image.channel,
        board=payload.tgt_image.board,
        version=payload.tgt_image.version,
        random_str=random_str,
        src_version=src_version,
        bucket=payload.tgt_image.bucket)
  else:
    raise Error('Unknown image type %s' % type(payload.tgt_image))


def FillInPayloadUri(payload, random_str=None):
  """Fill in default output URI for a payload if missing.

  Args:
    payload: gspaths.Payload instance.
    random_str: A hook to force a specific random_str. None means generate it.
  """
  if not payload.uri:
    payload.uri = DefaultPayloadUri(payload, random_str)


def _FilterNonPayloadUris(payload_uris):
  """Filters out non-payloads from a list of GS URIs.

  This essentially filters out known auxiliary artifacts whose names resemble /
  derive from a respective payload name, such as files with .log and
  .metadata-signature extensions.

  Args:
    payload_uris: a list of GS URIs (potentially) corresopnding to payloads

  Returns:
    A filtered list of URIs.
  """
  return [uri for uri in payload_uris
          if not (uri.endswith('.log') or uri.endswith('.metadata-signature'))]


def FindExistingPayloads(payload):
  """Look to see if any matching payloads already exist.

  Since payload names contain a random component, there can be multiple
  names for a given payload. This function lists all existing payloads
  that match the default URI for the given payload.

  Args:
    payload: gspaths.Payload instance.

  Returns:
    List of URIs for existing payloads that match the default payload pattern.
  """
  search_uri = DefaultPayloadUri(payload, random_str='*')
  return _FilterNonPayloadUris(urilib.ListFiles(search_uri))


def FindCacheDir():
  """Helper for deciding what cache directory to use.

  Returns:
    Returns a directory suitable for use with a DownloadCache.
  """
  # Discover which directory to use for caching
  return os.path.join(path_util.GetCacheDir(), 'paygen_cache')


def CreateAndUploadPayload(payload, cache, work_dir, sign=True, verify=True,
                           dry_run=False, au_generator_uri=None):
  """Helper to create a PaygenPayloadLib instance and use it.

  Args:
    payload: An instance of utils.Payload describing the payload to generate.
    cache: An instance of DownloadCache for retrieving files.
    work_dir: A working directory that can hold scratch files. Will be cleaned
              up when done, and won't interfere with other users. None for /tmp.
    sign: Boolean saying if the payload should be signed (normally, you do).
    verify: whether the payload should be verified (default: True)
    dry_run: don't perform actual work
    au_generator_uri: URI to override standard au_generator.zip rules.
  """
  # We need to create a temp directory inside the chroot so be able to access
  # from both inside and outside the chroot.
  temp_dir = path_util.FromChrootPath(cros_build_lib.RunCommand(
      ['mktemp', '-d'],
      capture_output=True,
      enter_chroot=True).output.strip())

  with osutils.TempDir(prefix='paygen_payload.', base_dir=temp_dir) as gen_dir:
    logging.info('* Starting payload generation')
    start_time = datetime.datetime.now()

    _PaygenPayload(payload, cache, gen_dir, sign, verify, au_generator_uri,
                   dry_run=dry_run).Run()

    end_time = datetime.datetime.now()
    logging.info('* Finished payload generation in %s', end_time - start_time)
