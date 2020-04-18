# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""ChromeOS image pusher (from cbuildbot to signer).

This pushes files from the archive bucket to the signer bucket and marks
artifacts for signing (which a signing process will look for).
"""

from __future__ import print_function

import ConfigParser
import cStringIO
import getpass
import os
import re
import tempfile
import textwrap

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import signing


# This will split a fully qualified ChromeOS version string up.
# R34-5126.0.0 will break into "34" and "5126.0.0".
VERSION_REGEX = r'^R([0-9]+)-([^-]+)'

# The test signers will scan this dir looking for test work.
# Keep it in sync with the signer config files [gs_test_buckets].
TEST_SIGN_BUCKET_BASE = 'gs://chromeos-throw-away-bucket/signer-tests'

# Keysets that are only valid in the above test bucket.
TEST_KEYSET_PREFIX = 'test-keys'
TEST_KEYSETS = set((
    'mp',
    'premp',
    'nvidia-premp',
))

# Supported image types for signing.
_SUPPORTED_IMAGE_TYPES = (
    constants.IMAGE_TYPE_RECOVERY,
    constants.IMAGE_TYPE_FACTORY,
    constants.IMAGE_TYPE_FIRMWARE,
    constants.IMAGE_TYPE_NV_LP0_FIRMWARE,
    constants.IMAGE_TYPE_ACCESSORY_USBPD,
    constants.IMAGE_TYPE_ACCESSORY_RWSIG,
    constants.IMAGE_TYPE_BASE,
)


class PushError(Exception):
  """When an (unknown) error happened while trying to push artifacts."""


class MissingBoardInstructions(Exception):
  """Raised when a board lacks any signer instructions."""

  def __init__(self, board, image_type, input_insns):
    Exception.__init__(self, 'Board %s lacks insns for %s image: %s not found' %
                       (board, image_type, input_insns))


class InputInsns(object):
  """Object to hold settings for a signable board.

  Note: The format of the instruction file pushimage outputs (and the signer
  reads) is not exactly the same as the instruction file pushimage reads.
  """

  def __init__(self, board, image_type=None):
    """Initialization.

    Args:
      board: The board to look up details.
      image_type: The type of image we will be signing (see --sign-types).
    """
    self.board = board

    config = ConfigParser.ConfigParser()
    config.readfp(open(self.GetInsnFile('DEFAULT')))

    # What pushimage internally refers to as 'recovery', are the basic signing
    # instructions in practice, and other types are stacked on top.
    if image_type is None:
      image_type = constants.IMAGE_TYPE_RECOVERY
    self.image_type = image_type
    input_insns = self.GetInsnFile(constants.IMAGE_TYPE_RECOVERY)
    if not os.path.exists(input_insns):
      # This board doesn't have any signing instructions.
      raise MissingBoardInstructions(self.board, image_type, input_insns)
    config.readfp(open(input_insns))

    if image_type is not None:
      input_insns = self.GetInsnFile(image_type)
      if not os.path.exists(input_insns):
        # This type doesn't have any signing instructions.
        raise MissingBoardInstructions(self.board, image_type, input_insns)

      self.image_type = image_type
      config.readfp(open(input_insns))

    self.cfg = config

  def GetInsnFile(self, image_type):
    """Find the signer instruction files for this board/image type.

    Args:
      image_type: The type of instructions to load.  It can be a common file
        (like "DEFAULT"), or one of the --sign-types.

    Returns:
      Full path to the instruction file using |image_type| and |self.board|.
    """
    if image_type == image_type.upper():
      name = image_type
    elif image_type in (constants.IMAGE_TYPE_RECOVERY,
                        constants.IMAGE_TYPE_BASE):
      name = self.board
    else:
      name = '%s.%s' % (self.board, image_type)

    return os.path.join(signing.INPUT_INSN_DIR, '%s.instructions' % name)

  @staticmethod
  def SplitCfgField(val):
    """Split a string into multiple elements.

    This centralizes our convention for multiple elements in the input files
    being delimited by either a space or comma.

    Args:
      val: The string to split.

    Returns:
      The list of elements from having done split the string.
    """
    return val.replace(',', ' ').split()

  def GetChannels(self):
    """Return the list of channels to sign for this board.

    If the board-specific config doesn't specify a preference, we'll use the
    common settings.
    """
    return self.SplitCfgField(self.cfg.get('insns', 'channel'))

  def GetKeysets(self, insns_merge=None):
    """Return the list of keysets to sign for this board.

    Args:
      insns_merge: The additional section to look at over [insns].
    """
    # First load the default value from [insns.keyset] if available.
    sections = ['insns']
    # Then overlay the [insns.xxx.keyset] if requested.
    if insns_merge is not None:
      sections += [insns_merge]

    keyset = ''
    for section in sections:
      try:
        keyset = self.cfg.get(section, 'keyset')
      except (ConfigParser.NoSectionError, ConfigParser.NoOptionError):
        pass

    # We do not perturb the order (e.g. using sorted() or making a set())
    # because we want the behavior stable, and we want the input insns to
    # explicitly control the order (since it has an impact on naming).
    return self.SplitCfgField(keyset)

  def GetAltInsnSets(self):
    """Return the list of alternative insn sections."""
    # We do not perturb the order (e.g. using sorted() or making a set())
    # because we want the behavior stable, and we want the input insns to
    # explicitly control the order (since it has an impact on naming).
    ret = [x for x in self.cfg.sections() if x.startswith('insns.')]
    return ret if ret else [None]

  @staticmethod
  def CopyConfigParser(config):
    """Return a copy of a ConfigParser object.

    The python folks broke the ability to use something like deepcopy:
    https://bugs.python.org/issue16058
    """
    # Write the current config to a string io object.
    data = cStringIO.StringIO()
    config.write(data)
    data.seek(0)

    # Create a new ConfigParser from the serialized data.
    ret = ConfigParser.ConfigParser()
    ret.readfp(data)

    return ret

  def OutputInsns(self, output_file, sect_insns, sect_general,
                  insns_merge=None):
    """Generate the output instruction file for sending to the signer.

    The override order is (later has precedence):
      [insns]
      [insns_merge]  (should be named "insns.xxx")
      sect_insns

    Note: The format of the instruction file pushimage outputs (and the signer
    reads) is not exactly the same as the instruction file pushimage reads.

    Args:
      output_file: The file to write the new instruction file to.
      sect_insns: Items to set/override in the [insns] section.
      sect_general: Items to set/override in the [general] section.
      insns_merge: The alternative insns.xxx section to merge.
    """
    # Create a copy so we can clobber certain fields.
    config = self.CopyConfigParser(self.cfg)
    sect_insns = sect_insns.copy()

    # Merge in the alternative insns section if need be.
    if insns_merge is not None:
      for k, v in config.items(insns_merge):
        sect_insns.setdefault(k, v)

    # Clear channel entry in instructions file, ensuring we only get
    # one channel for the signer to look at.  Then provide all the
    # other details for this signing request to avoid any ambiguity
    # and to avoid relying on encoding data into filenames.
    for sect, fields in zip(('insns', 'general'), (sect_insns, sect_general)):
      if not config.has_section(sect):
        config.add_section(sect)
      for k, v in fields.iteritems():
        config.set(sect, k, v)

    # Now prune the alternative sections.
    for alt in self.GetAltInsnSets():
      config.remove_section(alt)

    output = cStringIO.StringIO()
    config.write(output)
    data = output.getvalue()
    osutils.WriteFile(output_file, data)
    logging.debug('generated insns file for %s:\n%s', self.image_type, data)


def MarkImageToBeSigned(ctx, tbs_base, insns_path, priority):
  """Mark an instructions file for signing.

  This will upload a file to the GS bucket flagging an image for signing by
  the signers.

  Args:
    ctx: A viable gs.GSContext.
    tbs_base: The full path to where the tobesigned directory lives.
    insns_path: The path (relative to |tbs_base|) of the file to sign.
    priority: Set the signing priority (lower == higher prio).

  Returns:
    The full path to the remote tobesigned file.
  """
  if priority < 0 or priority > 99:
    raise ValueError('priority must be [0, 99] inclusive')

  if insns_path.startswith(tbs_base):
    insns_path = insns_path[len(tbs_base):].lstrip('/')

  tbs_path = '%s/tobesigned/%02i,%s' % (tbs_base, priority,
                                        insns_path.replace('/', ','))

  # The caller will catch gs.GSContextException for us.
  ctx.Copy('-', tbs_path, input=cros_build_lib.MachineDetails())

  return tbs_path


def PushImage(src_path, board, versionrev=None, profile=None, priority=50,
              sign_types=None, dry_run=False, mock=False, force_keysets=(),
              force_channels=None):
  """Push the image from the archive bucket to the release bucket.

  Args:
    src_path: Where to copy the files from; can be a local path or gs:// URL.
      Should be a full path to the artifacts in either case.
    board: The board we're uploading artifacts for (e.g. $BOARD).
    versionrev: The full Chromium OS version string (e.g. R34-5126.0.0).
    profile: The board profile in use (e.g. "asan").
    priority: Set the signing priority (lower == higher prio).
    sign_types: If set, a set of types which we'll restrict ourselves to
      signing.  See the --sign-types option for more details.
    dry_run: Show what would be done, but do not upload anything.
    mock: Upload to a testing bucket rather than the real one.
    force_keysets: Set of keysets to use rather than what the inputs say.
    force_channels: Set of channels to use rather than what the inputs say.

  Returns:
    A dictionary that maps 'channel' -> ['gs://signer_instruction_uri1',
                                         'gs://signer_instruction_uri2',
                                         ...]
  """
  # Whether we hit an unknown error.  If so, we'll throw an error, but only
  # at the end (so that we still upload as many files as possible).
  # It's implemented using a list to deal with variable scopes in nested
  # functions below.
  unknown_error = [False]

  if versionrev is None:
    # Extract milestone/version from the directory name.
    versionrev = os.path.basename(src_path)

  # We only support the latest format here.  Older releases can use pushimage
  # from the respective branch which deals with legacy cruft.
  m = re.match(VERSION_REGEX, versionrev)
  if not m:
    raise ValueError('version %s does not match %s' %
                     (versionrev, VERSION_REGEX))
  milestone = m.group(1)
  version = m.group(2)

  # Normalize board to always use dashes not underscores.  This is mostly a
  # historical artifact at this point, but we can't really break it since the
  # value is used in URLs.
  boardpath = board.replace('_', '-')
  if profile is not None:
    boardpath += '-%s' % profile.replace('_', '-')

  ctx = gs.GSContext(dry_run=dry_run)

  try:
    input_insns = InputInsns(board)
  except MissingBoardInstructions as e:
    logging.warning('Missing base instruction file: %s', e)
    logging.warning('not uploading anything for signing')
    return

  if force_channels is None:
    channels = input_insns.GetChannels()
  else:
    # Filter out duplicates.
    channels = sorted(set(force_channels))

  # We want force_keysets as a set.
  force_keysets = set(force_keysets)

  if mock:
    logging.info('Upload mode: mock; signers will not process anything')
    tbs_base = gs_base = os.path.join(constants.TRASH_BUCKET, 'pushimage-tests',
                                      getpass.getuser())
  elif set(['%s-%s' % (TEST_KEYSET_PREFIX, x)
            for x in TEST_KEYSETS]) & force_keysets:
    logging.info('Upload mode: test; signers will process test keys')
    # We need the tbs_base to be in the place the signer will actually scan.
    tbs_base = TEST_SIGN_BUCKET_BASE
    gs_base = os.path.join(tbs_base, getpass.getuser())
  else:
    logging.info('Upload mode: normal; signers will process the images')
    tbs_base = gs_base = constants.RELEASE_BUCKET

  sect_general = {
      'config_board': board,
      'board': boardpath,
      'version': version,
      'versionrev': versionrev,
      'milestone': milestone,
  }
  sect_insns = {}

  if dry_run:
    logging.info('DRY RUN MODE ACTIVE: NOTHING WILL BE UPLOADED')
  logging.info('Signing for channels: %s', ' '.join(channels))

  instruction_urls = {}

  def _ImageNameBase(image_type=None):
    lmid = ('%s-' % image_type) if image_type else ''
    return 'ChromeOS-%s%s-%s' % (lmid, versionrev, boardpath)

  # These variables are defined outside the loop so that the nested functions
  # below can access them without 'cell-var-from-loop' linter warning.
  dst_path = ""
  files_to_sign = []
  for channel in channels:
    logging.debug('\n\n#### CHANNEL: %s ####\n', channel)
    sect_insns['channel'] = channel
    sub_path = '%s-channel/%s/%s' % (channel, boardpath, version)
    dst_path = '%s/%s' % (gs_base, sub_path)
    logging.info('Copying images to %s', dst_path)

    recovery_basename = _ImageNameBase(constants.IMAGE_TYPE_RECOVERY)
    factory_basename = _ImageNameBase(constants.IMAGE_TYPE_FACTORY)
    firmware_basename = _ImageNameBase(constants.IMAGE_TYPE_FIRMWARE)
    nv_lp0_firmware_basename = _ImageNameBase(
        constants.IMAGE_TYPE_NV_LP0_FIRMWARE)
    acc_usbpd_basename = _ImageNameBase(constants.IMAGE_TYPE_ACCESSORY_USBPD)
    acc_rwsig_basename = _ImageNameBase(constants.IMAGE_TYPE_ACCESSORY_RWSIG)
    test_basename = _ImageNameBase(constants.IMAGE_TYPE_TEST)
    base_basename = _ImageNameBase(constants.IMAGE_TYPE_BASE)
    hwqual_tarball = 'chromeos-hwqual-%s-%s.tar.bz2' % (board, versionrev)

    # The following build artifacts, if present, are always copied regardless of
    # requested signing types.
    files_to_copy_only = (
        # (<src>, <dst>, <suffix>),
        ('image.zip', _ImageNameBase(), 'zip'),
        (constants.TEST_IMAGE_TAR, test_basename, 'tar.xz'),
        ('debug.tgz', 'debug-%s' % boardpath, 'tgz'),
        (hwqual_tarball, '', ''),
        ('au-generator.zip', '', ''),
        ('stateful.tgz', '', ''),
    )

    # The following build artifacts, if present, are always copied.
    # If |sign_types| is None, all of them are marked for signing, otherwise
    # only the image types specified in |sign_types| are marked for signing.
    files_to_copy_and_maybe_sign = (
        # (<src>, <dst>, <suffix>, <signing type>),
        (constants.RECOVERY_IMAGE_TAR, recovery_basename, 'tar.xz',
         constants.IMAGE_TYPE_RECOVERY),

        ('factory_image.zip', factory_basename, 'zip',
         constants.IMAGE_TYPE_FACTORY),

        ('firmware_from_source.tar.bz2', firmware_basename, 'tar.bz2',
         constants.IMAGE_TYPE_FIRMWARE),

        ('firmware_from_source.tar.bz2', nv_lp0_firmware_basename, 'tar.bz2',
         constants.IMAGE_TYPE_NV_LP0_FIRMWARE),

        ('firmware_from_source.tar.bz2', acc_usbpd_basename, 'tar.bz2',
         constants.IMAGE_TYPE_ACCESSORY_USBPD),

        ('firmware_from_source.tar.bz2', acc_rwsig_basename, 'tar.bz2',
         constants.IMAGE_TYPE_ACCESSORY_RWSIG),
    )

    # The following build artifacts are copied and marked for signing, if
    # they are present *and* if the image type is specified via |sign_types|.
    files_to_maybe_copy_and_sign = (
        # (<src>, <dst>, <suffix>, <signing type>),
        (constants.BASE_IMAGE_TAR, base_basename, 'tar.xz',
         constants.IMAGE_TYPE_BASE),
    )

    def _CopyFileToGS(src, dst, suffix):
      """Returns |dst| file name if the copying was successful."""
      if not dst:
        dst = src
      elif suffix:
        dst = '%s.%s' % (dst, suffix)
      success = False
      try:
        ctx.Copy(os.path.join(src_path, src), os.path.join(dst_path, dst))
        success = True
      except gs.GSNoSuchKey:
        logging.warning('Skipping %s as it does not exist', src)
      except gs.GSContextException:
        unknown_error[0] = True
        logging.error('Skipping %s due to unknown GS error', src, exc_info=True)
      return dst if success else None

    for src, dst, suffix in files_to_copy_only:
      _CopyFileToGS(src, dst, suffix)

    # Clear the list of files to sign before adding new artifacts.
    files_to_sign = []

    def _AddToFilesToSign(image_type, dst, suffix):
      assert dst.endswith('.' + suffix), (
          'dst: %s, suffix: %s' % (dst, suffix))
      dst_base = dst[:-(len(suffix) + 1)]
      files_to_sign.append([image_type, dst_base, suffix])

    for src, dst, suffix, image_type in files_to_copy_and_maybe_sign:
      dst = _CopyFileToGS(src, dst, suffix)
      if dst and (not sign_types or image_type in sign_types):
        _AddToFilesToSign(image_type, dst, suffix)

    for src, dst, suffix, image_type in files_to_maybe_copy_and_sign:
      if sign_types and image_type in sign_types:
        dst = _CopyFileToGS(src, dst, suffix)
        if dst:
          _AddToFilesToSign(image_type, dst, suffix)

    logging.debug('Files to sign: %s', files_to_sign)
    # Now go through the subset for signing.
    for image_type, dst_name, suffix in files_to_sign:
      try:
        input_insns = InputInsns(board, image_type=image_type)
      except MissingBoardInstructions as e:
        logging.info('Nothing to sign: %s', e)
        continue

      dst_archive = '%s.%s' % (dst_name, suffix)
      sect_general['archive'] = dst_archive
      sect_general['type'] = image_type

      # In the default/automatic mode, only flag files for signing if the
      # archives were actually uploaded in a previous stage. This additional
      # check can be removed in future once |sign_types| becomes a required
      # argument.
      # TODO: Make |sign_types| a required argument.
      gs_artifact_path = os.path.join(dst_path, dst_archive)
      exists = False
      try:
        exists = ctx.Exists(gs_artifact_path)
      except gs.GSContextException:
        unknown_error[0] = True
        logging.error('Unknown error while checking %s', gs_artifact_path,
                      exc_info=True)
      if not exists:
        logging.info('%s does not exist.  Nothing to sign.',
                     gs_artifact_path)
        continue

      first_image = True
      for alt_insn_set in input_insns.GetAltInsnSets():
        # Figure out which keysets have been requested for this type.
        # We sort the forced set so tests/runtime behavior is stable.
        keysets = sorted(force_keysets)
        if not keysets:
          keysets = input_insns.GetKeysets(insns_merge=alt_insn_set)
          if not keysets:
            logging.warning('Skipping %s image signing due to no keysets',
                            image_type)

        for keyset in keysets:
          sect_insns['keyset'] = keyset

          # Generate the insn file for this artifact that the signer will use,
          # and flag it for signing.
          with tempfile.NamedTemporaryFile(
              bufsize=0, prefix='pushimage.insns.') as insns_path:
            input_insns.OutputInsns(insns_path.name, sect_insns, sect_general,
                                    insns_merge=alt_insn_set)

            gs_insns_path = '%s/%s' % (dst_path, dst_name)
            if not first_image:
              gs_insns_path += '-%s' % keyset
            first_image = False
            gs_insns_path += '.instructions'

            try:
              ctx.Copy(insns_path.name, gs_insns_path)
            except gs.GSContextException:
              unknown_error[0] = True
              logging.error('Unknown error while uploading insns %s',
                            gs_insns_path, exc_info=True)
              continue

            try:
              MarkImageToBeSigned(ctx, tbs_base, gs_insns_path, priority)
            except gs.GSContextException:
              unknown_error[0] = True
              logging.error('Unknown error while marking for signing %s',
                            gs_insns_path, exc_info=True)
              continue
            logging.info('Signing %s image with keyset %s at %s', image_type,
                         keyset, gs_insns_path)
            instruction_urls.setdefault(channel, []).append(gs_insns_path)

  if unknown_error[0]:
    raise PushError('hit some unknown error(s)', instruction_urls)

  return instruction_urls


def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)

  # The type of image_dir will strip off trailing slashes (makes later
  # processing simpler and the display prettier).
  parser.add_argument('image_dir', default=None, type='local_or_gs_path',
                      help='full path of source artifacts to upload')
  parser.add_argument('--board', default=None, required=True,
                      help='board to generate symbols for')
  parser.add_argument('--profile', default=None,
                      help='board profile in use (e.g. "asan")')
  parser.add_argument('--version', default=None,
                      help='version info (normally extracted from image_dir)')
  parser.add_argument('--channels', default=None, action='split_extend',
                      help='override list of channels to process')
  parser.add_argument('-n', '--dry-run', default=False, action='store_true',
                      help='show what would be done, but do not upload')
  parser.add_argument('-M', '--mock', default=False, action='store_true',
                      help='upload things to a testing bucket (dev testing)')
  parser.add_argument('--test-sign', default=[], action='append',
                      choices=TEST_KEYSETS,
                      help='mung signing behavior to sign w/ test keys')
  parser.add_argument('--priority', type=int, default=50,
                      help='set signing priority (lower == higher prio)')
  parser.add_argument('--sign-types', default=None, nargs='+',
                      choices=_SUPPORTED_IMAGE_TYPES,
                      help='only sign specified image types')
  parser.add_argument('--yes', action='store_true', default=False,
                      help='answer yes to all prompts')

  return parser


def main(argv):
  parser = GetParser()
  opts = parser.parse_args(argv)
  opts.Freeze()

  force_keysets = set(['%s-%s' % (TEST_KEYSET_PREFIX, x)
                       for x in opts.test_sign])

  # If we aren't using mock or test or dry run mode, then let's prompt the user
  # to make sure they actually want to do this.  It's rare that people want to
  # run this directly and hit the release bucket.
  if not (opts.mock or force_keysets or opts.dry_run) and not opts.yes:
    prolog = '\n'.join(textwrap.wrap(textwrap.dedent(
        'Uploading images for signing to the *release* bucket is not something '
        'you generally should be doing yourself.'), 80)).strip()
    if not cros_build_lib.BooleanPrompt(
        prompt='Are you sure you want to sign these images',
        default=False, prolog=prolog):
      cros_build_lib.Die('better safe than sorry')

  PushImage(opts.image_dir, opts.board, versionrev=opts.version,
            profile=opts.profile, priority=opts.priority,
            sign_types=opts.sign_types, dry_run=opts.dry_run, mock=opts.mock,
            force_keysets=force_keysets, force_channels=opts.channels)
