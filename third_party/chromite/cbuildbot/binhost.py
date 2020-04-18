# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility functions for calculating compatible binhosts."""

from __future__ import print_function

import collections
import json
import os
import tempfile

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import parallel


# A unique identifier for looking up CompatIds by board/useflags.
_BoardKey = collections.namedtuple('_BoardKey', ['board', 'useflags'])


def BoardKey(board, useflags):
  """Create a new _BoardKey object.

  Args:
    board: The board associated with this config.
    useflags: A sequence of extra useflags associated with this config.
  """
  return _BoardKey(board, tuple(useflags))


def GetBoardKey(config, board=None):
  """Get the BoardKey associated with a given config.

  Args:
    config: A config_lib.BuildConfig object.
    board: Board to use. Defaults to the first board in the config.
      Optional if len(config.boards) == 1.
  """
  if board is None:
    assert len(config.boards) == 1
    board = config.boards[0]
  else:
    assert board in config.boards
  return BoardKey(board, config.useflags)


def GetAllImportantBoardKeys(site_config):
  """Get a list of all board keys used in a top-level config.

  Args:
    site_config: A config_lib.SiteConfig instance.
  """
  boards = set()
  for config in site_config.values():
    if config.important:
      for board in config.boards:
        boards.add(GetBoardKey(config, board))
  return boards


def GetChromePrebuiltConfigs(site_config):
  """Get a mapping of the boards used in the Chrome PFQ.

  Args:
    site_config: A config_lib.SiteConfig instance.

  Returns:
    A dict mapping BoardKey objects to configs.
  """
  boards = {}
  master_chromium_pfq = site_config['master-chromium-pfq']
  for config in site_config.GetSlavesForMaster(master_chromium_pfq):
    if config.prebuilts:
      for board in config.boards:
        boards[GetBoardKey(config, board)] = config
  return boards


# A tuple of dicts describing our Chrome PFQs.
# by_compat_id: A dict mapping CompatIds to sets of BoardKey objects.
# by_arch_useflags: A dict mapping (arch, useflags) tuples to sets of
#     BoardKey objects.
_PrebuiltMapping = collections.namedtuple(
    '_PrebuiltMapping', ['by_compat_id', 'by_arch_useflags'])


class PrebuiltMapping(_PrebuiltMapping):
  """A tuple of dicts describing our Chrome PFQs.

  Members:
    by_compat_id: A dict mapping CompatIds to sets of BoardKey objects.
    by_arch_useflags: A dict mapping (arch, useflags) tuples to sets of
      BoardKey objects.
  """

  # The location in a ChromeOS checkout where we should store our JSON dump.
  INTERNAL_MAP_LOCATION = ('%s/src/private-overlays/chromeos-partner-overlay/'
                           'chromeos/binhost/%s.json')

  # The location in an external Chromium OS checkout where we should store our
  # JSON dump.
  EXTERNAL_MAP_LOCATION = ('%s/src/third_party/chromiumos-overlay/chromeos/'
                           'binhost/%s.json')

  @classmethod
  def GetFilename(cls, buildroot, suffix, internal=True):
    """Get the filename where we should store our JSON dump.

    Args:
      buildroot: The root of the source tree.
      suffix: The base filename used for the dump (e.g. "chrome").
      internal: If true, use the internal binhost location. Otherwise, use the
        public one.
    """
    if internal:
      return cls.INTERNAL_MAP_LOCATION % (buildroot, suffix)

    return cls.EXTERNAL_MAP_LOCATION % (buildroot, suffix)

  @classmethod
  def Get(cls, keys, compat_ids):
    """Get a mapping of the Chrome PFQ configs.

    Args:
      keys: A list of the BoardKey objects that are considered part of the
        Chrome PFQ.
      compat_ids: A dict mapping BoardKey objects to CompatId objects.

    Returns:
      A PrebuiltMapping object.
    """
    configs = cls(by_compat_id=collections.defaultdict(set),
                  by_arch_useflags=collections.defaultdict(set))
    for key in keys:
      compat_id = compat_ids[key]
      configs.by_compat_id[compat_id].add(key)
      partial_compat_id = (compat_id.arch, compat_id.useflags)
      configs.by_arch_useflags[partial_compat_id].add(key)
    return configs

  def Dump(self, filename, internal=True):
    """Save a mapping of the Chrome PFQ configs to disk (JSON format).

    Args:
      filename: A location to write the Chrome PFQ configs.
      internal: Whether the dump should include internal configurations.
    """
    output = []
    for compat_id, keys in self.by_compat_id.items():
      for key in keys:
        # Filter internal prebuilts out of external dumps.
        if not internal and 'chrome_internal' in key.useflags:
          continue

        output.append({'key': key.__dict__, 'compat_id': compat_id.__dict__})

    with open(filename, 'w') as f:
      json.dump(output, f, sort_keys=True, indent=2)

  @classmethod
  def Load(cls, filename):
    """Load a mapping of the Chrome PFQ configs from disk (JSON format).

    Args:
      filename: A location to read the Chrome PFQ configs from.
    """
    with open(filename) as f:
      output = json.load(f)

    compat_ids = {}
    for d in output:
      key = BoardKey(**d['key'])
      compat_ids[key] = CompatId(**d['compat_id'])

    return cls.Get(compat_ids.keys(), compat_ids)

  def GetPrebuilts(self, compat_id):
    """Get the matching BoardKey objects associated with |compat_id|.

    Args:
      compat_id: The CompatId to use to look up prebuilts.
    """
    if compat_id in self.by_compat_id:
      return self.by_compat_id[compat_id]

    partial_compat_id = (compat_id.arch, compat_id.useflags)
    if partial_compat_id in self.by_arch_useflags:
      return self.by_arch_useflags[partial_compat_id]

    return set()


def GetChromeUseFlags(board, extra_useflags):
  """Get a list of the use flags turned on for Chrome on a given board.

  This function requires that the board has been set up first (e.g. using
  GenConfigsForBoard)

  Args:
    board: The board to use.
    extra_useflags: A sequence of use flags to enable or disable.

  Returns:
    A tuple of the use flags that are enabled for Chrome on the given board.
    Use flags that are disabled are not listed.
  """
  assert cros_build_lib.IsInsideChroot()
  assert os.path.exists('/build/%s' % board), 'Board %s not set up' % board
  extra_env = {'USE': ' '.join(extra_useflags)}
  cmd = ['equery-%s' % board, 'uses', constants.CHROME_CP]
  chrome_useflags = cros_build_lib.RunCommand(
      cmd, capture_output=True, print_cmd=False,
      extra_env=extra_env).output.rstrip().split()
  return tuple(x[1:] for x in chrome_useflags if x.startswith('+'))


def GenConfigsForBoard(board, regen, error_code_ok):
  """Set up the configs for the specified board.

  This must be run from within the chroot. It sets up the board but does not
  fully initialize it (it skips the initialization of the toolchain and the
  board packages)

  Args:
    board: Board to set up.
    regen: Whether to regen configs if the board already exists.
    error_code_ok: Whether errors are acceptable. We set this to True in some
      tests for configs that are not on the waterfall.
  """
  assert cros_build_lib.IsInsideChroot()
  if regen or not os.path.exists('/build/%s' % board):
    cmd = ['%s/src/scripts/setup_board' % constants.CHROOT_SOURCE_ROOT,
           '--board=%s' % board, '--regen_configs', '--skip_toolchain_update',
           '--skip_chroot_upgrade', '--skip_board_pkg_init', '--quiet']
    cros_build_lib.RunCommand(cmd, error_code_ok=error_code_ok)


_CompatId = collections.namedtuple('_CompatId', ['arch', 'useflags', 'cflags'])


def CompatId(arch, useflags, cflags):
  """Create a new _CompatId object.

  Args:
    arch: The architecture of this builder.
    useflags: The full list of use flags for Chrome.
    cflags: The full list of CFLAGS.
  """
  return _CompatId(arch, tuple(useflags), tuple(cflags))


def CalculateCompatId(board, extra_useflags):
  """Calculate the CompatId for board with the specified extra useflags.

  This function requires that the board has been set up first (e.g. using
  GenConfigsForBoard)

  Args:
    board: The board to use.
    extra_useflags: A sequence of use flags to enable or disable.

  Returns:
    A CompatId object for the board with the specified extra_useflags.
  """
  assert cros_build_lib.IsInsideChroot()
  useflags = GetChromeUseFlags(board, extra_useflags)
  cmd = ['portageq-%s' % board, 'envvar', 'ARCH', 'CFLAGS']
  arch_cflags = cros_build_lib.RunCommand(
      cmd, print_cmd=False, capture_output=True).output.rstrip()
  arch, cflags = arch_cflags.split('\n', 1)
  cflags_split = cflags.split()
  return CompatId(arch, useflags, cflags_split)


class CompatIdFetcher(object):
  """Class for calculating CompatIds in parallel."""

  def __init__(self, caching=False):
    """Create a new CompatIdFetcher object.

    Args:
      caching: Whether to cache setup from run to run. See
        PrebuiltCompatibilityTest.CACHING for details.
    """
    self.compat_ids = None
    if caching:
      # This import occurs here rather than at the top of the file because we
      # don't want to force developers to install joblib. The caching argument
      # is only set to True if PrebuiltCompatibilityTest.CACHING is hand-edited
      # (for testing purposes).
      # pylint: disable=import-error
      from joblib import Memory
      memory = Memory(cachedir=tempfile.gettempdir(), verbose=0)
      self.FetchCompatIds = memory.cache(self.FetchCompatIds)

  def _FetchCompatId(self, board, extra_useflags):
    self.compat_ids[(board, extra_useflags)] = (
        CalculateCompatId(board, extra_useflags))

  def FetchCompatIds(self, board_keys):
    """Generate a dict mapping BoardKeys to their associated CompatId.

    Args:
      board_keys: A list of BoardKey objects to fetch.
    """
    # pylint: disable=method-hidden
    logging.info('Fetching CompatId objects...')
    with parallel.Manager() as manager:
      self.compat_ids = manager.dict()
      parallel.RunTasksInProcessPool(self._FetchCompatId, board_keys)
      return dict(self.compat_ids)
