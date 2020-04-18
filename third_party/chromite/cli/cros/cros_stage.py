# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Stage a custom image on a Moblab device or in Google Storage."""

from __future__ import print_function

import os
import re
import shutil

from chromite.cbuildbot import commands
from chromite.cli import command
from chromite.cli import flash
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import dev_server_wrapper
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import remote_access


MOBLAB_STATIC_DIR = '/mnt/moblab/static'
MOBLAB_TMP_DIR = os.path.join(MOBLAB_STATIC_DIR, 'tmp')
BOARD_BUILD_DIR = 'usr/local/build'
DEVSERVER_STAGE_URL = ('http://%(moblab)s:8080/stage?local_path=%(staged_dir)s'
                       '&artifacts=full_payload,stateful,test_suites,'
                       'control_files,autotest_packages,'
                       'autotest_server_package')
CUSTOM_BUILD_NAME = '%(board)s-custom/%(build)s'


class CustomImageStagingException(Exception):
  """Thrown when there is an error staging an custom image."""


def GSURLRegexHelper(gsurl):
  """Helper to do regex matching on a Google Storage URL

  Args:
    gsurl: Google Storage URL to match.

  Returns:
    Regex Match Object with groups(board, type, & build_name) or None if there
    was no match.
  """
  return re.match(r'gs://.*/(trybot-)?(?P<board>[\w-]+)-(?P<type>\w+)/'
                  r'(?P<build_name>R\d+-[\d.ab-]+)', gsurl)


@command.CommandDecorator('stage')
class StageCommand(command.CliCommand):
  """Remotely stages an image onto a MobLab device or into Google Storage.

  The image to be staged may be a local custom image built in the chroot or an
  official image in Google Storage. The test binaries will always come from the
  local build root regardless of the image source.

  This script generates/copies the update payloads and test binaries required.
  It then stages them on the Moblab's devserver or copies them into the
  specified Google Storage Bucket.

  The image name to then use for testing is outputted at the end of this
  script.
  """

  EPILOG = """
To stage a local image path onto a moblab device:
  cros stage /path/to/board/build/chromiumos-test-image.bin <moblab>

To stage an official image with custom test binaries onto a moblab device:
  cros stage <gs_image_dir> <moblab>

To stage a local image path into a Google Storage Bucket:
  cros stage /path/to/board/build/chromiumos-test-image.bin <gs_base_path>
    --boto_file=<boto_file_path>

NOTES:
* The autotest bits used to test this image will be the latest in your
  build sysroot! I.E. if you emerge new autotest changes after producing the
  image you wish to stage, there is a chance that the changes will not match.
* The custom image will only stay on the Moblab device for 24 hours at which
  point it will be wiped.
"""

  @classmethod
  def AddParser(cls, parser):
    """Add parser arguments."""
    super(StageCommand, cls).AddParser(parser)
    parser.add_argument(
        'image', nargs='?', default='latest', help='Path to image we want to '
        'stage. If a local path, it should be in the format of '
        '/.../.../board/build/<image>.bin . If a Google Storage path it should'
        'be in the format of '
        'gs://<bucket-name>/<board>-<builder type>/<build name>')
    parser.add_argument(
        'remote', help='MobLab device that has password-less SSH set up via '
        'the chroot already. Or Google Storage Bucket in the form of '
        'gs://<bucket-name>/')
    parser.add_argument(
        '--board', dest='board', default=None,
        help='The board name, defaults to value extracted from image path.')
    parser.add_argument(
        '--staged_image_name', dest='staged_image_name', default=None,
        help='Name for the staged image. Default: <board>-custom/<build>')
    parser.add_argument(
        '--boto_file', dest='boto_file', default=None,
        help='Path to boto file to use when uploading to Google Storage. If '
        'none the default chroot boto file is used.')

  def __init__(self, options):
    """Initializes cros stage."""
    super(StageCommand, self).__init__(options)
    self.board = self.options.board
    self.staged_image_name = self.options.staged_image_name
    # Determine if we are staging a local custom image or an official image.
    if self.options.image.startswith('gs://'):
      self._remote_image = True
      if not self.staged_image_name:
        self.staged_image_name = self._GenerateImageNameFromGSUrl(
            self.options.image)
    else:
      self._remote_image = False
      if not self.staged_image_name:
        self.staged_image_name = self._GenerateImageNameFromLocalPath(
            self.options.image)
    if not self.board:
      raise CustomImageStagingException('Please specify the "board" argument')
    self.stage_directory = os.path.join(MOBLAB_TMP_DIR, self.staged_image_name)

    # Determine if the staging destination is a Moblab or Google Storage.
    if self.options.remote.startswith('gs://'):
      self._remote_is_moblab = False
    else:
      self._remote_is_moblab = True

  def _GenerateImageNameFromLocalPath(self, image):
    """Generate the name as which |image| will be staged onto Moblab.

    If the board name has not been specified, set the board name based on
    the image path.

    Args:
      image: Path to image we want to stage. It should be in the format of
             /.../.../board/build/<image>.bin

    Returns:
      Name the image will be staged as.

    Raises:
      CustomImageStagingException: If the image name supplied is not valid.
    """
    realpath = osutils.ExpandPath(image)
    if not realpath.endswith('.bin'):
      raise CustomImageStagingException(
          'Image path: %s does not end in .bin !' % realpath)
    build_name = os.path.basename(os.path.dirname(realpath))
    # Custom builds are name with the suffix of '-a1' but the build itself
    # is missing this suffix in its filesystem. Therefore lets rename the build
    # name to match the name inside the build.
    if build_name.endswith('-a1'):
      build_name = build_name[:-len('-a1')]

    if not self.board:
      self.board = os.path.basename(os.path.dirname(os.path.dirname(realpath)))
    return CUSTOM_BUILD_NAME % dict(board=self.board, build=build_name)

  def _GenerateImageNameFromGSUrl(self, image):
    """Generate the name as which |image| will be staged onto Moblab.

    If the board name has not been specified, set the board name based on
    the image path.

    Args:
      image: GS Url to the image we want to stage. It should be in the format
             gs://<bucket-name>/<board>-<builder type>/<build name>

    Returns:
      Name the image will be staged as.

    Raises:
      CustomImageStagingException: If the image name supplied is not valid.
    """
    match = GSURLRegexHelper(image)
    if not match:
      raise CustomImageStagingException(
          'Image URL: %s is improperly defined!' % image)
    if not self.board:
      self.board = match.group('board')
    return CUSTOM_BUILD_NAME % dict(board=self.board,
                                    build=match.group('build_name'))

  def _DownloadPayloads(self, tempdir):
    """Download from GS the update payloads we require.

    Args:
      tempdir: Temporary Directory to store the downloaded payloads.
    """
    gs_context = gs.GSContext(boto_file=self.options.boto_file)
    gs_context.Copy(os.path.join(self.options.image, 'stateful.tgz'), tempdir)
    gs_context.Copy(os.path.join(self.options.image, '*_full*'), tempdir)

  def _GeneratePayloads(self, tempdir):
    """Generate the update payloads we require.

    Args:
      tempdir: Temporary Directory to store the generated payloads.
    """
    dev_server_wrapper.GetUpdatePayloadsFromLocalPath(
        self.options.image, tempdir, static_dir=flash.DEVSERVER_STATIC_DIR)
    rootfs_payload = os.path.join(tempdir, dev_server_wrapper.ROOTFS_FILENAME)
    # Devservers will look for a file named *_full_*.
    shutil.move(rootfs_payload, os.path.join(tempdir, 'update_full_dev.bin'))

  def _GenerateTestBits(self, tempdir):
    """Generate and transfer to the Moblab the test bits we require.

    Args:
      tempdir: Temporary Directory to store the generated test artifacts.
    """
    build_root = cros_build_lib.GetSysroot(board=self.board)
    cwd = os.path.join(build_root, BOARD_BUILD_DIR)
    commands.BuildAutotestTarballsForHWTest(build_root, cwd, tempdir)

  def _StageOnMoblab(self, tempdir):
    """Stage the generated payloads and test bits on a moblab device.

    Args:
      tempdir: Temporary Directory that contains the generated payloads and
               test bits.
    """
    with remote_access.ChromiumOSDeviceHandler(self.options.remote) as device:
      device.RunCommand(['mkdir', '-p', self.stage_directory])
      for f in os.listdir(tempdir):
        device.CopyToDevice(os.path.join(tempdir, f), self.stage_directory,
                            mode='rsync')
      device.RunCommand(['chown', '-R', 'moblab:moblab',
                         MOBLAB_TMP_DIR])
      # Delete this image from the Devserver in case it was previously staged.
      device.RunCommand(['rm', '-rf', os.path.join(MOBLAB_STATIC_DIR,
                                                   self.staged_image_name)])
      stage_url = DEVSERVER_STAGE_URL % dict(moblab=self.options.remote,
                                             staged_dir=self.stage_directory)
      # Stage the image from the moblab, as port 8080 might not be reachable
      # from the developer's system.
      res = device.RunCommand(['curl', '--fail',
                               cros_build_lib.ShellQuote(stage_url)],
                              error_code_ok=True)
      if res.returncode == 0:
        logging.info('\n\nStaging Completed!')
        logging.info('Image is staged on Moblab as %s',
                     self.staged_image_name)
      else:
        logging.info('Staging failed. Error Message: %s', res.error)

      device.RunCommand(['rm', '-rf', self.stage_directory])

  def _StageOnGS(self, tempdir):
    """Stage the generated payloads and test bits into a Google Storage bucket.

    Args:
      tempdir: Temporary Directory that contains the generated payloads and
               test bits.
    """
    gs_context = gs.GSContext(boto_file=self.options.boto_file)
    for f in os.listdir(tempdir):
      gs_context.CopyInto(os.path.join(tempdir, f), os.path.join(
          self.options.remote, self.staged_image_name))
    logging.info('\n\nStaging Completed!')
    logging.info('Image is staged in Google Storage as %s',
                 self.staged_image_name)

  def Run(self):
    """Perform the cros stage command."""
    logging.info('Attempting to stage: %s as Image: %s at Location: %s',
                 self.options.image, self.staged_image_name,
                 self.options.remote)
    osutils.SafeMakedirsNonRoot(flash.DEVSERVER_STATIC_DIR)

    with osutils.TempDir() as tempdir:
      if self._remote_image:
        self._DownloadPayloads(tempdir)
      else:
        self._GeneratePayloads(tempdir)
      self._GenerateTestBits(tempdir)
      if self._remote_is_moblab:
        self._StageOnMoblab(tempdir)
      else:
        self._StageOnGS(tempdir)
