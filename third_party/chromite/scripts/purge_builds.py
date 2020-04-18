# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Purge old build artifacts to free up space.

Looks at build artifacts in gs://chromeos-releases and
gs://chromeos-image-archive. If they are old enough (and not protected for
some reason), they are moved to the matching backup bucket
gs://chromeos-releases-backup, or gs://chromeos-image-archive-backup.

Files in the backup buckets have a limited lifespan (currently 30 days), and are
deleted from there permanently after that time.

See go/cros-gs-cleanup-design for an overview.
"""

from __future__ import print_function

import datetime
import multiprocessing

from chromite.lib import commandline
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import parallel
from chromite.lib import purge_lib

# Roughly 6 months.
BUILD_CLEANUP = datetime.timedelta(weeks=6*4)


def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(
      description=__doc__,
      default_log_level='debug')

  parser.add_argument(
      '--dry-run', action='store_true', default=False,
      help='This is a dryrun, no files will be moved.')

  parser.add_argument(
      '--chromeos-releases', action='store_true', default=False,
      help='Cleanup files from gs://chromeos-releases.')

  parser.add_argument(
      '--chromeos-image-archive', action='store_true', default=False,
      help='Cleanup files from gs://chromeos-image-archive.')

  return parser


def Examine(ctx, dryrun, expired_cutoff, candidates):
  """Given a list of candidates to move, move them to the backup buckets.

  Args:
    ctx: GS context.
    dryrun: Flag to turn on/off bucket updates.
    expired_cutoff: datetime.datetime of cutoff for expiring candidates.
    candidates: Iterable of gs.GSListResult objects.
  """
  # Scale our processes with CPUs, but overload since we mostly block on
  # external calls.
  processes = multiprocessing.cpu_count() * 5
  with parallel.BackgroundTaskRunner(purge_lib.ExpandAndExpire,
                                     ctx, dryrun, expired_cutoff,
                                     processes=processes) as expire_queue:
    for candidate in candidates:
      expire_queue.put((candidate,))


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)
  options.Freeze()

  ctx = gs.GSContext()
  ctx.GetDefaultGSUtilBin()  # To force caching of gsutil pre-forking.

  now = datetime.datetime.now()
  expired_cutoff = now - BUILD_CLEANUP
  logging.info('Cutoff: %s', expired_cutoff)

  if options.chromeos_image_archive:
    archiveExcludes = purge_lib.LocateChromeosImageArchiveProtectedPrefixes(ctx)
    logging.info('Excluding:%s', '\n  '.join(archiveExcludes))

    archiveCandidates = purge_lib.ProduceFilteredCandidates(
        ctx, 'gs://chromeos-image-archive/', archiveExcludes, 2)
    Examine(ctx, options.dry_run, expired_cutoff, archiveCandidates)

  if options.chromeos_releases:
    remote_branches = purge_lib.ListRemoteBranches()
    protected_branches = purge_lib.ProtectedBranchVersions(remote_branches)
    logging.info('Protected branch versions: %s',
                 '\n  '.join(protected_branches))

    releasesExcludes = purge_lib.LocateChromeosReleasesProtectedPrefixes(
        ctx, protected_branches)
    logging.info('Excluding:%s', '\n  '.join(releasesExcludes))

    releasesCandidates = purge_lib.ProduceFilteredCandidates(
        ctx, 'gs://chromeos-releases/', releasesExcludes, 3)
    Examine(ctx, options.dry_run, expired_cutoff, releasesCandidates)

  return
