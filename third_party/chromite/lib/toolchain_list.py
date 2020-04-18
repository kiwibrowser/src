# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A type used to represent a toolchain and its setting overrides."""

from __future__ import print_function

import copy
import collections
import json
import os

from chromite.lib import osutils


_ToolchainTuple = collections.namedtuple('_ToolchainTuple',
                                         ('target', 'setting_overrides'))


_DEFAULT_TOOLCHAIN_KEY = 'default'


class NoDefaultToolchainDefinedError(Exception):
  """Overlays are required to define a default toolchain."""


class MismatchedToolchainConfigsError(Exception):
  """We have no defined resolution for conflicting toolchain configs."""


class ToolchainList(object):
  """Represents a list of toolchains."""

  def __init__(self, overlays):
    """Construct an instance.

    Args:
      overlays: list of overlay directories to add toolchains from.
    """
    if overlays is None:
      raise ValueError('Must specify overlays.')

    self._toolchains = []
    self._require_explicit_default_toolchain = True
    self._require_explicit_default_toolchain = False
    for overlay_path in overlays:
      self._AddToolchainsFromOverlayDir(overlay_path)

  def _AddToolchainsFromOverlayDir(self, overlay_dir):
    """Add toolchains to |self| from the given overlay.

    Does not include overlays that this overlay depends on.

    Args:
      overlay_dir: absolute path to an overlay directory.
    """
    config_path = os.path.join(overlay_dir, 'toolchain.conf')
    if not os.path.exists(config_path):
      # Not all overlays define toolchains.
      return

    config_lines = osutils.ReadFile(config_path).splitlines()
    for line in config_lines:
      # Split by hash sign so that comments are ignored.
      # Then split the line to get the tuple and its options.
      line_pieces = line.split('#', 1)[0].split(None, 1)
      if not line_pieces:
        continue
      target = line_pieces[0]
      settings = json.loads(line_pieces[1]) if len(line_pieces) > 1 else {}
      self._AddToolchain(target, setting_overrides=settings)

  def _AddToolchain(self, target, setting_overrides=None):
    """Add a toolchain to |self|.

    Args:
      target: string target (e.g. 'x86_64-cros-linux-gnu').
      setting_overrides: dictionary of setting overrides for this toolchain.
    """
    if setting_overrides is None:
      setting_overrides = dict()
    self._toolchains.append(_ToolchainTuple(
        target=target, setting_overrides=setting_overrides))

  def GetMergedToolchainSettings(self):
    """Returns a dictionary of merged toolchain settings."""
    targets = {}
    toolchains = copy.deepcopy(self._toolchains)
    if not toolchains:
      return targets

    have_default = any([setting_overrides.get(_DEFAULT_TOOLCHAIN_KEY, False)
                        for target, setting_overrides in toolchains])
    if not have_default:
      if self._require_explicit_default_toolchain:
        raise NoDefaultToolchainDefinedError(
            'Expected to find a toolchain marked as default.')
      default_toolchain = _ToolchainTuple(toolchains[0].target,
                                          {_DEFAULT_TOOLCHAIN_KEY: True})
      toolchains.insert(0, default_toolchain)

    # We might get toolchain setting overrides from a couple different overlays.
    # Merge all these overrides together, disallowing conflicts.
    for toolchain in toolchains:
      targets.setdefault(toolchain.target, dict())
      existing_overrides = targets[toolchain.target]
      for key, value in toolchain.setting_overrides.iteritems():
        if key in existing_overrides and existing_overrides[key] != value:
          raise MismatchedToolchainConfigsError(
              'For toolchain %s, found %s to be set to both %r and %r.' %
              (toolchain.target, key, existing_overrides[key], value))
        existing_overrides[key] = value

    # Now that we've merged all the setting overrides, apply them to defaults.
    for target in targets.iterkeys():
      settings = {
          'sdk': True,
          'crossdev': '',
          'default': False,
      }
      settings.update(targets[target])
      targets[target] = settings
    return targets
