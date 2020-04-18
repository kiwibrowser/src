# -*- coding: utf-8 -*-
# Copyright (c) 2011-2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that contains trybot patch pool code."""

from __future__ import print_function

import functools

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import gerrit
from chromite.lib import git
from chromite.lib import patch as cros_patch


site_config = config_lib.GetConfig()


def ChromiteFilter(patch):
  """Used with FilterFn to isolate patches to chromite."""
  return patch.project == constants.CHROMITE_PROJECT


def ExtManifestFilter(patch):
  """Used with FilterFn to isolate patches to the external manifest."""
  return patch.project == site_config.params.MANIFEST_PROJECT


def IntManifestFilter(patch):
  """Used with FilterFn to isolate patches to the internal manifest."""
  return patch.project == site_config.params.MANIFEST_INT_PROJECT


def ManifestFilter(patch):
  """Used with FilterFn to isolate patches to the manifest."""
  return ExtManifestFilter(patch) or IntManifestFilter(patch)


def BranchFilter(branch, patch):
  """Used with FilterFn to isolate patches based on a specific upstream."""
  return patch.tracking_branch == branch


def GitRemoteUrlFilter(url, patch):
  """Used with FilterFn to isolate a patch based on the url of its remote."""
  return patch.git_remote_url == url


class TrybotPatchPool(object):
  """Represents patches specified by the user to test."""
  def __init__(self, gerrit_patches=(), local_patches=(), remote_patches=()):
    self.gerrit_patches = tuple(gerrit_patches)
    self.local_patches = tuple(local_patches)
    self.remote_patches = tuple(remote_patches)

  def __nonzero__(self):
    """Returns True if the pool has any patches."""
    return any([self.gerrit_patches, self.local_patches, self.remote_patches])

  def Filter(self, **kwargs):
    """Returns a new pool with only patches that match constraints.

    Args:
      **kwargs: constraints in the form of attr=value.  I.e.,
                project='chromiumos/chromite', tracking_branch='master'.
    """
    def AttributeFilter(patch):
      for key in kwargs:
        if getattr(patch, key, object()) != kwargs[key]:
          return False
      return True

    return self.FilterFn(AttributeFilter)

  def FilterFn(self, filter_fn, negate=False):
    """Returns a new pool with only patches that match constraints.

    Args:
      filter_fn: Functor that accepts a 'patch' argument, and returns whether to
                 include the patch in the results.
      negate: Return patches that don't pass the filter_fn.
    """
    f = filter_fn
    if negate:
      f = lambda p: not filter_fn(p)

    return self.__class__(
        gerrit_patches=filter(f, self.gerrit_patches),
        local_patches=filter(f, self.local_patches),
        remote_patches=filter(f, self.remote_patches))

  def FilterManifest(self, negate=False):
    """Return a patch pool with only patches to the manifest."""
    return self.FilterFn(ManifestFilter, negate=negate)

  def FilterIntManifest(self, negate=False):
    """Return a patch pool with only patches to the internal manifest."""
    return self.FilterFn(IntManifestFilter, negate=negate)

  def FilterExtManifest(self, negate=False):
    """Return a patch pool with only patches to the external manifest."""
    return self.FilterFn(ExtManifestFilter, negate=negate)

  def FilterBranch(self, branch, negate=False):
    """Return a patch pool with only patches based on a particular branch."""
    return self.FilterFn(functools.partial(BranchFilter, branch), negate=negate)

  def FilterGitRemoteUrl(self, url, negate=False):
    """Return a patch pool where patches have a particular remote url."""
    return self.FilterFn(functools.partial(GitRemoteUrlFilter, url),
                         negate=negate)

  def __iter__(self):
    for source in [self.local_patches, self.remote_patches,
                   self.gerrit_patches]:
      for patch in source:
        yield patch

  @classmethod
  def FromOptions(cls, gerrit_patches=None, local_patches=None, sourceroot=None,
                  remote_patches=None):
    """Generate patch objects from passed in options.

    Args:
      gerrit_patches: Gerrit ids that gerrit.GetGerritPatchInfo accepts.
      local_patches: Local ids that cros_patch.PrepareLocalPatches accepts.
      sourceroot: The source repository to look up |local_patches|.
      remote_patches: Remote ids that cros_patch.PrepareRemotePatches accepts.

    Returns:
      A TrybotPatchPool object.

    Raises:
      gerrit.GerritException, cros_patch.PatchException
    """
    if gerrit_patches:
      gerrit_patches = gerrit.GetGerritPatchInfo(gerrit_patches)
      for patch in gerrit_patches:
        if patch.IsAlreadyMerged():
          logging.warning('Patch %s has already been merged.', patch)
    else:
      gerrit_patches = ()

    if local_patches:
      manifest = git.ManifestCheckout.Cached(sourceroot)
      local_patches = cros_patch.PrepareLocalPatches(manifest, local_patches)
    else:
      local_patches = ()

    if remote_patches:
      remote_patches = cros_patch.PrepareRemotePatches(remote_patches)
    else:
      remote_patches = ()

    return cls(gerrit_patches=gerrit_patches, local_patches=local_patches,
               remote_patches=remote_patches)
