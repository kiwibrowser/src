# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Command to list patches applies to a repository."""

from __future__ import print_function

import functools
import json
import os
import parallel_emerge
import re
import shutil
import sys
import tempfile

from chromite.lib import cros_build_lib
from chromite.lib import osutils
from chromite.lib import portage_util


class PatchReporter(object):
  """Help discover patches being applied by ebuilds.

  The patches can be compared to a set of expected patches.  They can also be
  sorted into categories like 'needs_upstreaming', etc.  Use of this can help
  ensure that critical (e.g. security) patches are not inadvertently dropped,
  and help surface forgotten-about patches that are yet-to-be upstreamed.
  """

  PATCH_TYPES = ('upstreamed', 'needs_upstreaming', 'not_for_upstream',
                 'uncategorized')

  def __init__(self, config, overlay_dir, ebuild_cmd, equery_cmd, sudo=False):
    """Initialize.

    The 'config' dictionary should look like this:
    {
      "ignored_packages": ["chromeos-base/chromeos-chrome"],
      "upstreamed": [],
      "needs_upstreaming": [],
      "not_for_upstream": [],
      "uncategorized": [
        "net-misc/htpdate htpdate-1.0.4-checkagainstbuildtime.patch",
        "net-misc/htpdate htpdate-1.0.4-errorcheckhttpresp.patch"
      ]
    }
    """
    self.overlay_dir = os.path.realpath(overlay_dir)
    self.ebuild_cmd = ebuild_cmd
    self.equery_cmd = equery_cmd
    self._invoke_command = cros_build_lib.RunCommand
    if sudo:
      self._invoke_command = functools.partial(cros_build_lib.SudoRunCommand,
                                               strict=False)
    self.ignored_packages = config['ignored_packages']
    self.package_count = 0
    # The config format is stored as category: [ list of patches ]
    # for ease of maintenance. But it's actually more useful to us
    # in the code if kept as a map of patch:patch_type.
    self.patches = {}
    for cat in self.PATCH_TYPES:
      for patch in config[cat]:
        self.patches[patch] = cat

  def Ignored(self, package_name):
    """See if |package_name| should be ignored.

    Args:
      package_name: A package name (e.g. 'chromeos-base/chromeos-chrome')

    Returns:
       True if this package should be skipped in the analysis. False otherwise.
    """
    return package_name in self.ignored_packages

  def ObservePatches(self, deps_map):
    """Observe the patches being applied by ebuilds in |deps_map|.

    Args:
      deps_map: The packages to analyze.

    Returns:
       A list of patches being applied.
    """
    original = os.environ.get('PORT_LOGDIR', None)
    temp_space = None
    try:
      temp_space = tempfile.mkdtemp(prefix='check_patches')
      os.environ['PORT_LOGDIR'] = temp_space
      return self._ObservePatches(temp_space, deps_map)
    finally:
      if temp_space:
        shutil.rmtree(os.environ['PORT_LOGDIR'])
      if original:
        os.environ['PORT_LOGDIR'] = original
      else:
        os.environ.pop('PORT_LOGDIR')

  def _ObservePatches(self, temp_space, deps_map):
    for cpv in deps_map:
      split = portage_util.SplitCPV(cpv)
      if self.Ignored('%s/%s' % (split.category, split.package)):
        continue
      cmd = self.equery_cmd[:]
      cmd.extend(['which', cpv])
      ebuild_path = self._invoke_command(cmd, print_cmd=False,
                                         redirect_stdout=True).output.rstrip()
      # Some of these packages will be from other portdirs. Since we are
      # only interested in extracting the patches from one particular
      # overlay, we skip ebuilds not from that overlay.
      if self.overlay_dir != os.path.commonprefix([self.overlay_dir,
                                                   ebuild_path]):
        continue

      # By running 'ebuild blah.ebuild prepare', we get logs in PORT_LOGDIR
      # of what patches were applied. We clean first, to ensure we get a
      # complete log, and clean again afterwards to avoid leaving a mess.
      cmd = self.ebuild_cmd[:]
      cmd.extend([ebuild_path, 'clean', 'prepare', 'clean'])
      self._invoke_command(cmd, print_cmd=False, redirect_stdout=True)
      self.package_count += 1

    # Done with ebuild. Now just harvest the logs and we're finished.
    # This regex is tuned intentionally to ignore a few unhelpful cases.
    # E.g. elibtoolize repetitively applies a set of sed/portage related
    # patches. And media-libs/jpeg says it is applying
    # "various patches (bugfixes/updates)", which isn't very useful for us.
    # So, if you noticed these omissions, it was intentional, not a bug. :-)
    patch_regex = r'^ [*] Applying ([^ ]*) [.][.][.].*'
    output = cros_build_lib.RunCommand(
        ['egrep', '-r', patch_regex, temp_space], print_cmd=False,
        redirect_stdout=True).output
    lines = output.splitlines()
    patches = []
    patch_regex = re.compile(patch_regex)
    for line in lines:
      cat, pv, _, patchmsg = line.split(':')
      cat = os.path.basename(cat)
      split = portage_util.SplitCPV('%s/%s' % (cat, pv))
      patch_name = re.sub(patch_regex, r'\1', patchmsg)
      patches.append('%s/%s %s' % (cat, split.package, patch_name))

    return patches

  def ReportDiffs(self, observed_patches):
    """Prints a report on any differences to stdout.

    Returns:
      An int representing the total number of discrepancies found.
    """
    expected_patches = set(self.patches.keys())
    observed_patches = set(observed_patches)
    missing_patches = sorted(list(expected_patches - observed_patches))
    unexpected_patches = sorted(list(observed_patches - expected_patches))

    if missing_patches:
      print('Missing Patches:')
      for p in missing_patches:
        print('%s (%s)' % (p, self.patches[p]))

    if unexpected_patches:
      print('Unexpected Patches:')
      print('\n'.join(unexpected_patches))

    return len(missing_patches) + len(unexpected_patches)


def Usage():
  """Print usage."""
  print("""Usage:
cros_check_patches [--board=BOARD] [emerge args] package overlay-dir config.json

Given a package name (e.g. 'virtual/target-os') and an overlay directory
(e.g. /usr/local/portage/chromiumos), outputs a list of patches
applied by that overlay, in the course of building the specified
package and all its dependencies. Additional configuration options are
specified in the JSON-format config file named on the command line.

First run? Try this for a starter config:
{
  "ignored_packages": ["chromeos-base/chromeos-chrome"],
  "upstreamed": [],
  "needs_upstreaming": [],
  "not_for_upstream": [],
  "uncategorized": []
}
""")


def main(argv):
  if len(argv) < 4:
    Usage()
    sys.exit(1)

  # Avoid parsing most of argv because most of it is destined for
  # DepGraphGenerator/emerge rather than us. Extract what we need
  # without disturbing the rest.
  config_path = argv.pop()
  config = json.loads(osutils.ReadFile(config_path))
  overlay_dir = argv.pop()
  board = [x.split('=')[1] for x in argv if x.find('--board=') != -1]
  if board:
    ebuild_cmd = ['ebuild-%s' % board[0]]
    equery_cmd = ['equery-%s' % board[0]]
  else:
    ebuild_cmd = ['ebuild']
    equery_cmd = ['equery']

  use_sudo = not board

  # We want the toolchain to be quiet to avoid interfering with our output.
  depgraph_argv = ['--quiet', '--pretend', '--emptytree']

  # Defaults to rdeps, but allow command-line override.
  default_rootdeps_arg = ['--root-deps=rdeps']
  for arg in argv:
    if arg.startswith('--root-deps'):
      default_rootdeps_arg = []

  # Now, assemble the overall argv as the concatenation of the
  # default list + possible rootdeps-default + actual command line.
  depgraph_argv.extend(default_rootdeps_arg)
  depgraph_argv.extend(argv)

  deps = parallel_emerge.DepGraphGenerator()
  deps.Initialize(depgraph_argv)
  deps_tree, deps_info = deps.GenDependencyTree()
  deps_map = deps.GenDependencyGraph(deps_tree, deps_info)

  reporter = PatchReporter(config, overlay_dir, ebuild_cmd, equery_cmd,
                           sudo=use_sudo)
  observed = reporter.ObservePatches(deps_map)
  diff_count = reporter.ReportDiffs(observed)

  print('Packages analyzed: %d' % reporter.package_count)
  print('Patches observed: %d' % len(observed))
  print('Patches expected: %d' % len(reporter.patches.keys()))
  sys.exit(diff_count)
