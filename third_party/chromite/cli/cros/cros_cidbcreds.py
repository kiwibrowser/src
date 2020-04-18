# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros cidbcreds: get the cidb credentials from gs."""

from __future__ import print_function

import os
import shutil

from chromite.cli import command
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import locking
from chromite.lib import path_util

PROD_REPLICA_CIDB_READONLY_BUCKET = (
    'gs://chromeos-cidb-creds/prod_replica_cidb_gen2_readonly/')

def GetCIDBCreds(cidb_dir):
  """Download CIDB creds from google storage to local cidb diretory."""
  ctx = gs.GSContext(init_boto=True)
  ctx.Copy(PROD_REPLICA_CIDB_READONLY_BUCKET + '*', cidb_dir)

def CheckAndGetCIDBCreds(force_update=False, folder=None):
  """Check if CIDB creds exist, download creds if necessary."""
  cache_dir = path_util.GetCacheDir()
  dir_name = folder if folder is not None else 'cidb_creds'
  cidb_dir = os.path.join(cache_dir, dir_name)
  cidb_dir_lock = cidb_dir + '.lock'

  with locking.FileLock(cidb_dir_lock).write_lock():
    if os.path.exists(cidb_dir):
      if force_update:
        shutil.rmtree(cidb_dir, ignore_errors=True)
        logging.debug('Force updating CIDB creds. Deleted %s.', cidb_dir)
      else:
        logging.debug('Using cached credentials %s', cidb_dir)
        return cidb_dir

    os.mkdir(cidb_dir)

    try:
      GetCIDBCreds(cidb_dir)
      return cidb_dir
    except Exception as e:
      if isinstance(e, gs.GSCommandError):
        logging.warning('Please check if the GS credentials is configured '
                        'correctly. Please note the permissions to fetch '
                        'these credentials are for Googlers only,')

      logging.error('Failed to get CIDB credentials. Deleting %s', cidb_dir)
      shutil.rmtree(cidb_dir, ignore_errors=True)
      raise

@command.CommandDecorator('cidbcreds')
class CidbCredsCommand(command.CliCommand):
  """cros cidbcreds: download the prod_replica_cidb_readonly credentials."""

  def __init__(self, options):
    super(CidbCredsCommand, self).__init__(options)

  @classmethod
  def AddParser(cls, parser):
    super(cls, CidbCredsCommand).AddParser(parser)
    parser.add_argument('--folder', action='store',
                        help='The folder name to store the credentials.')
    parser.add_argument('--force-update', action='store_true',
                        help='force updating the credentials.')
    return parser

  def Run(self):
    """Run cros cidbcreds."""
    self.options.Freeze()
    cidb_dir = CheckAndGetCIDBCreds(force_update=self.options.force_update,
                                    folder=self.options.folder)
    logging.notice('CIDB credentials at: %s', cidb_dir)
