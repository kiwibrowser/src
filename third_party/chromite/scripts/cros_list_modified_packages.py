# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Calculate what workon packages have changed since the last build.

A workon package is treated as changed if any of the below are true:
  1) The package is not installed.
  2) A file exists in the associated repository which has a newer modification
     time than the installed package.
  3) The source ebuild has a newer modification time than the installed package.

Some caveats:
  - We do not look at eclasses. This replicates the existing behavior of the
    commit queue, which also does not look at eclass changes.
  - We do not try to fallback to the non-workon package if the local tree is
    unmodified. This is probably a good thing, since developers who are
    "working on" a package want to compile it locally.
  - Portage only stores the time that a package finished building, so we
    aren't able to detect when users modify source code during builds.
"""

from __future__ import print_function

import errno
import multiprocessing
import os
try:
  import Queue
except ImportError:
  # Python-3 renamed to "queue".  We still use Queue to avoid collisions
  # with naming variables as "queue".  Maybe we'll transition at some point.
  # pylint: disable=F0401
  import queue as Queue

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import portage_util
from chromite.lib import sysroot_lib
from chromite.lib import workon_helper


class ModificationTimeMonitor(object):
  """Base class for monitoring last modification time of paths.

  This takes a list of (keys, path) pairs and finds the latest mtime of an
  object within each of the path's subtrees, populating a map from keys to
  mtimes. Note that a key may be associated with multiple paths, in which case
  the latest mtime among them will be returned.

  Members:
    _tasks: A list of (key, path) pairs to check.
    _result_queue: A queue populated with corresponding (key, mtime) pairs.
  """

  def __init__(self, key_path_pairs):
    self._tasks = list(key_path_pairs)
    self._result_queue = multiprocessing.Queue(len(self._tasks))

  def _EnqueueModificationTime(self, key, path):
    """Calculate the last modification time of |path| and enqueue it."""
    if os.path.isdir(path):
      self._result_queue.put((key, self._LastModificationTime(path)))

  def _LastModificationTime(self, path):
    """Returns the latest modification time for anything under |path|."""
    cmd = 'find . -name .git -prune -o -printf "%T@\n" | sort -nr | head -n1'
    ret = cros_build_lib.RunCommand(cmd, cwd=path, shell=True, print_cmd=False,
                                    capture_output=True)
    return float(ret.output) if ret.output else 0

  def GetModificationTimes(self):
    """Get the latest modification time for each of the queued keys."""
    parallel.RunTasksInProcessPool(self._EnqueueModificationTime, self._tasks)
    mtimes = {}
    try:
      while True:
        key, mtime = self._result_queue.get_nowait()
        mtimes[key] = max((mtimes.get(key, 0), mtime))
    except Queue.Empty:
      return mtimes


class WorkonPackageInfo(object):
  """Class for getting information about workon packages.

  Members:
    cp: The package name (e.g. chromeos-base/power_manager).
    mtime: The modification time of the installed package.
    projects: The project(s) associated with the package.
    full_srcpaths: The brick source path(s) associated with the package.
    src_ebuild_mtime: The modification time of the source ebuild.
  """

  def __init__(self, cp, mtime, projects, full_srcpaths, src_ebuild_mtime):
    self.cp = cp
    self.pkg_mtime = int(mtime)
    self.projects = projects
    self.full_srcpaths = full_srcpaths
    self.src_ebuild_mtime = src_ebuild_mtime


def ListWorkonPackages(sysroot, all_opt=False):
  """List the packages that are currently being worked on.

  Args:
    sysroot: sysroot_lib.Sysroot object.
    all_opt: Pass --all to cros_workon. For testing purposes.
  """
  helper = workon_helper.WorkonHelper(sysroot.path)
  return helper.ListAtoms(use_all=all_opt)


def ListWorkonPackagesInfo(sysroot):
  """Find the specified workon packages for the specified board.

  Args:
    sysroot: sysroot_lib.Sysroot object.

  Returns:
    A list of WorkonPackageInfo objects for unique packages being worked on.
  """
  # Import portage late so that this script can be imported outside the chroot.
  # pylint: disable=F0401
  import portage.const
  packages = ListWorkonPackages(sysroot)
  if not packages:
    return []
  results = {}

  if sysroot.path == '/':
    overlays = portage_util.FindOverlays(constants.BOTH_OVERLAYS, None)
  else:
    overlays = sysroot.GetStandardField('PORTDIR_OVERLAY').splitlines()

  vdb_path = os.path.join(sysroot.path, portage.const.VDB_PATH)

  for overlay in overlays:
    for filename, projects, srcpaths in portage_util.GetWorkonProjectMap(
        overlay, packages):
      # chromeos-base/power_manager/power_manager-9999
      # cp = chromeos-base/power_manager
      # cpv = chromeos-base/power_manager-9999
      category, pn, p = portage_util.SplitEbuildPath(filename)
      cp = '%s/%s' % (category, pn)
      cpv = '%s/%s' % (category, p)

      # Get the time the package finished building. TODO(build): Teach Portage
      # to store the time the package started building and use that here.
      pkg_mtime_file = os.path.join(vdb_path, cpv, 'BUILD_TIME')
      try:
        pkg_mtime = int(osutils.ReadFile(pkg_mtime_file))
      except EnvironmentError as ex:
        if ex.errno != errno.ENOENT:
          raise
        pkg_mtime = 0

      # Get the modificaton time of the ebuild in the overlay.
      src_ebuild_mtime = os.lstat(os.path.join(overlay, filename)).st_mtime

      # Write info into the results dictionary, overwriting any previous
      # values. This ensures that overlays override appropriately.
      results[cp] = WorkonPackageInfo(cp, pkg_mtime, projects, srcpaths,
                                      src_ebuild_mtime)

  return results.values()


def WorkonProjectsMonitor(projects):
  """Returns a monitor for project modification times."""
  # TODO(garnold) In order for the mtime monitor to be as accurate as
  # possible, this only needs to enqueue the checkout(s) relevant for the
  # task at hand, e.g. the specific ebuild we want to emerge. In general, the
  # CROS_WORKON_LOCALNAME variable in workon ebuilds defines the source path
  # uniquely and can be used for this purpose.
  project_path_pairs = []
  manifest = git.ManifestCheckout.Cached(constants.SOURCE_ROOT)
  for project in set(projects).intersection(manifest.checkouts_by_name):
    for checkout in manifest.FindCheckouts(project):
      project_path_pairs.append((project, checkout.GetPath(absolute=True)))

  return ModificationTimeMonitor(project_path_pairs)


def WorkonSrcpathsMonitor(srcpaths):
  """Returns a monitor for srcpath modification times."""
  return ModificationTimeMonitor(zip(srcpaths, srcpaths))


def ListModifiedWorkonPackages(sysroot):
  """List the workon packages that need to be rebuilt.

  Args:
    sysroot: sysroot_lib.Sysroot object.
  """
  packages = ListWorkonPackagesInfo(sysroot)
  if not packages:
    return

  # Get mtimes for all projects and source paths associated with our packages.
  all_projects = [p for info in packages for p in info.projects]
  project_mtimes = WorkonProjectsMonitor(all_projects).GetModificationTimes()
  all_srcpaths = [s for info in packages for s in info.full_srcpaths]
  srcpath_mtimes = WorkonSrcpathsMonitor(all_srcpaths).GetModificationTimes()

  for info in packages:
    mtime = int(max([project_mtimes.get(p, 0) for p in info.projects] +
                    [srcpath_mtimes.get(s, 0) for s in info.full_srcpaths] +
                    [info.src_ebuild_mtime]))
    if mtime >= info.pkg_mtime:
      yield info.cp


def _ParseArguments(argv):
  parser = commandline.ArgumentParser(description=__doc__)

  target = parser.add_mutually_exclusive_group(required=True)
  target.add_argument('--board', help='Board name')
  target.add_argument('--host', default=False, action='store_true',
                      help='Look at host packages instead of board packages')
  target.add_argument('--sysroot', help='Sysroot path.')

  flags = parser.parse_args(argv)
  flags.Freeze()
  return flags


def main(argv):
  logging.getLogger().setLevel(logging.INFO)
  flags = _ParseArguments(argv)
  sysroot = None
  if flags.board:
    sysroot = cros_build_lib.GetSysroot(flags.board)
  elif flags.host:
    sysroot = '/'
  else:
    sysroot = flags.sysroot

  modified = ListModifiedWorkonPackages(sysroot_lib.Sysroot(sysroot))
  print(' '.join(sorted(modified)))
