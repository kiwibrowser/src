# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for managing the toolchains in the chroot."""

from __future__ import print_function

import cStringIO

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import gs
from chromite.lib import portage_util
from chromite.lib import toolchain_list

if cros_build_lib.IsInsideChroot():
  # Only import portage after we've checked that we're inside the chroot.
  # Outside may not have portage, in which case the above may not happen.
  # We'll check in main() if the operation needs portage.

  # pylint: disable=F0401
  import portage


def GetHostTuple():
  """Returns compiler tuple for the host system."""
  # pylint: disable=E1101
  return portage.settings['CHOST']


# Tree interface functions. They help with retrieving data about the current
# state of the tree:
def GetAllTargets():
  """Get the complete list of targets.

  Returns:
    The list of cross targets for the current tree
  """
  targets = GetToolchainsForBoard('all')

  # Remove the host target as that is not a cross-target. Replace with 'host'.
  del targets[GetHostTuple()]
  return targets


def GetToolchainsForBoard(board, buildroot=constants.SOURCE_ROOT):
  """Get a dictionary mapping toolchain targets to their options for a board.

  Args:
    board: board name in question (e.g. 'daisy').
    buildroot: path to buildroot.

  Returns:
    The list of toolchain tuples for the given board
  """
  overlays = portage_util.FindOverlays(
      constants.BOTH_OVERLAYS, None if board in ('all', 'sdk') else board,
      buildroot=buildroot)
  toolchains = toolchain_list.ToolchainList(overlays=overlays)
  targets = toolchains.GetMergedToolchainSettings()
  if board == 'sdk':
    targets = FilterToolchains(targets, 'sdk', True)
  return targets


def GetToolchainTupleForBoard(board, buildroot=constants.SOURCE_ROOT):
  """Gets a tuple for the default and non-default toolchains for a board.

  Args:
    board: board name in question (e.g. 'daisy').
    buildroot: path to buildroot.

  Returns:
    The tuples of toolchain targets ordered default, non-default for the board.
  """
  toolchains = GetToolchainsForBoard(board, buildroot)
  return (FilterToolchains(toolchains, 'default', True).keys() +
          FilterToolchains(toolchains, 'default', False).keys())


def FilterToolchains(targets, key, value):
  """Filter out targets based on their attributes.

  Args:
    targets: dict of toolchains
    key: metadata to examine
    value: expected value for metadata

  Returns:
    dict where all targets whose metadata |key| does not match |value|
    have been deleted
  """
  return dict((k, v) for k, v in targets.iteritems() if v[key] == value)


def GetSdkURL(for_gsutil=False, suburl=''):
  """Construct a Google Storage URL for accessing SDK related archives

  Args:
    for_gsutil: Do you want a URL for passing to `gsutil`?
    suburl: A url fragment to tack onto the end

  Returns:
    The fully constructed URL
  """
  return gs.GetGsURL(constants.SDK_GS_BUCKET, for_gsutil=for_gsutil,
                     suburl=suburl)


def GetArchForTarget(target):
  """Returns the arch used by the given toolchain.

  Args:
    target: a toolchain.
  """
  info = cros_build_lib.RunCommand(['crossdev', '--show-target-cfg', target],
                                   capture_output=True, quiet=True).output
  return cros_build_lib.LoadKeyValueFile(cStringIO.StringIO(info)).get('arch')
