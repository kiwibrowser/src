# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module to upload your homebrew ChromeOS image as a GCE image.

Use this script if you have a GCE targeted CrOS image (currently,
lakitu_mobbuild) that you built locally, and want to upload it to a GCE project
to be used to launch a builder instance. This script will take care of
converting the image to a GCE friendly format and creating a GCE image from it.
"""

from __future__ import print_function

import os
import sys

from chromite.cbuildbot import commands
from chromite.compute import gcloud
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils


_DEFAULT_TMP_GS_PATH = 'gs://chromeos-throw-away-bucket/gce_tmp/'


class UploadGceImageRuntimError(RuntimeError):
  """RuntimeError raised explicitly from this module."""


def _GetParser():
  """Create a parser for this script."""
  parser = commandline.ArgumentParser(description=__doc__)

  parser.add_argument('source_image', type='path',
                      help='Path to the image to upload.')
  parser.add_argument('target_name',
                      help='Name of the final image created in the project.')
  parser.add_argument('--project', default='chromeos-bot',
                      help='The GCE project to target: (default: chromeos-bot)')
  parser.add_argument('--temp-gcs-path', type='gs_path',
                      default=_DEFAULT_TMP_GS_PATH,
                      help='GCS bucket used as temporary storage '
                           '(default: %s).' % _DEFAULT_TMP_GS_PATH)
  parser.add_argument('--dry-run', action='store_true',
                      help='Skip actually uploading stuff')
  return parser


def main(argv):
  parser = _GetParser()
  opts = parser.parse_args(argv)
  opts.Freeze()

  if opts.dry_run:
    logging.getLogger().setLevel(logging.DEBUG)

  if not os.path.isfile(opts.source_image):
    raise UploadGceImageRuntimError('%s is not a valid file.')

  source_dir, source_image_name = os.path.split(opts.source_image)
  with osutils.TempDir() as tempdir:
    logging.info('Generating tarball from %s', opts.source_image)
    tarball_name = commands.BuildGceTarball(tempdir, source_dir,
                                            source_image_name)
    # We must generate a uuid when uploading the tarball because repeated
    # uploads are likely to be named similarly. We'll just use tempdir to keep
    # files separate.
    temp_tarball_dir = os.path.join(opts.temp_gcs_path,
                                    os.path.basename(tempdir))
    gs_context = gs.GSContext(init_boto=True, retries=5, acl='private',
                              dry_run=opts.dry_run)
    gc_context = gcloud.GCContext(opts.project, dry_run=opts.dry_run)
    try:
      logging.info('Uploading tarball %s to %s',
                   tarball_name, temp_tarball_dir)
      gs_context.CopyInto(os.path.join(tempdir, tarball_name), temp_tarball_dir)
      logging.info('Creating image %s', opts.target_name)
      gc_context.CreateImage(opts.target_name,
                             source_uri=os.path.join(temp_tarball_dir,
                                                     tarball_name))
    except:
      logging.error('Oops! Something went wonky.')
      logging.error('Trying to clean up temporary artifacts...')
      try:
        with cros_build_lib.OutputCapturer() as output_capturer:
          gc_context.ListImages()
        if opts.target_name in ''.join(output_capturer.GetStdoutLines()):
          logging.info('Removing image %s', opts.target_name)
          gc_context.DeleteImage(opts.target_name, quiet=True)
      except gcloud.GCContextException:
        # Gobble up this error so external error is visible.
        logging.error('Failed to clean up image %s', opts.target_name)

      raise
    finally:
      logging.info('Removing GS tempdir %s', temp_tarball_dir)
      gs_context.Remove(temp_tarball_dir, ignore_missing=True)

  logging.info('All done!')


if __name__ == '__main__':
  main(sys.argv)
