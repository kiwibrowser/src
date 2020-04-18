# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Simple log collection script for Mob* Monitor"""

from __future__ import print_function

import glob
import os
import tempfile
import shutil

from chromite.lib import cros_build_lib
from chromite.lib import osutils


TMPDIR = '/mnt/moblab/tmp'
TMPDIR_PREFIX = 'moblab_logs_'
LOG_DIRS = {
    'apache_errors': '/var/log/apache2/error_log',
    'devserver_logs': '/var/log/devserver',
    'dhcp_leases': '/var/lib/dhcp',
    'messages': '/var/log/messages',
    'mysql': '/var/log/mysql',
    'servod': '/var/log/servod.log',
    'scheduler': '/usr/local/autotest/logs/scheduler.latest'
}

def remove_old_tarballs():
  paths = glob.iglob(os.path.join(TMPDIR, '%s*.tgz' % TMPDIR_PREFIX))
  for path in paths:
    os.remove(path)


def collect_logs():
  remove_old_tarballs()
  osutils.SafeMakedirs(TMPDIR)
  tempdir = tempfile.mkdtemp(prefix=TMPDIR_PREFIX, dir=TMPDIR)
  os.chmod(tempdir, 0o777)

  try:
    for name, path in LOG_DIRS.iteritems():
      if not os.path.exists(path):
        continue
      if os.path.isdir(path):
        shutil.copytree(path, os.path.join(tempdir, name))
      else:
        shutil.copyfile(path, os.path.join(tempdir, name))

    cmd = ['mobmoncli', 'GetStatus']
    cros_build_lib.RunCommand(
        cmd,
        log_stdout_to_file=os.path.join(tempdir, 'mobmonitor_getstatus')
    )
  finally:
    tarball = '%s.tgz' % tempdir
    cros_build_lib.CreateTarball(tarball, tempdir)
    osutils.RmDir(tempdir, ignore_missing=True)
  return tarball
