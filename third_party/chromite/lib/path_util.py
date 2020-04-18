# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Handle path inference and translation."""

from __future__ import print_function

import collections
import os
import tempfile

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import git
from chromite.lib import osutils


GENERAL_CACHE_DIR = '.cache'
CHROME_CACHE_DIR = '.cros_cache'

CHECKOUT_TYPE_UNKNOWN = 'unknown'
CHECKOUT_TYPE_GCLIENT = 'gclient'
CHECKOUT_TYPE_REPO = 'repo'

CheckoutInfo = collections.namedtuple(
    'CheckoutInfo', ['type', 'root', 'chrome_src_dir'])


class ChrootPathResolver(object):
  """Perform path resolution to/from the chroot.

  Args:
    source_path: Value to override default source root inference.
    source_from_path_repo: Whether to infer the source root from the converted
      path's repo parent during inbound translation; overrides |source_path|.
  """

  # TODO(garnold) We currently infer the source root based on the path's own
  # encapsulating repository. This is a heuristic catering to paths are being
  # translated to be used in a chroot that's not associated with the currently
  # executing code (for example, cbuildbot run on a build root or a foreign
  # tree checkout). This approach might result in arbitrary repo-contained
  # paths being translated to invalid chroot paths where they actually should
  # not, and other valid source paths failing to translate because they are not
  # repo-contained. Eventually we'll want to make this behavior explicit, by
  # either passing a source_root value, or requesting to infer it from the path
  # (source_from_path_repo=True), but otherwise defaulting to the executing
  # code's source root in the normal case. When that happens, we'll be
  # switching source_from_path_repo to False by default. See chromium:485746.

  def __init__(self, source_path=None, source_from_path_repo=True):
    self._inside_chroot = cros_build_lib.IsInsideChroot()
    self._source_path = (constants.SOURCE_ROOT if source_path is None
                         else source_path)
    self._source_from_path_repo = source_from_path_repo

    # The following are only needed if outside the chroot.
    if self._inside_chroot:
      self._chroot_path = None
      self._chroot_to_host_roots = None
    else:
      self._chroot_path = self._GetSourcePathChroot(self._source_path)

      # Initialize mapping of known root bind mounts.
      self._chroot_to_host_roots = (
          (constants.CHROOT_SOURCE_ROOT, self._source_path),
          (constants.CHROOT_CACHE_ROOT, self._GetCachePath),
      )

  @classmethod
  @cros_build_lib.MemoizedSingleCall
  def _GetCachePath(cls):
    """Returns the cache directory."""
    return os.path.realpath(GetCacheDir())

  def _GetSourcePathChroot(self, source_path):
    """Returns path to the chroot directory of a given source root."""
    if source_path is None:
      return None
    return os.path.join(source_path, constants.DEFAULT_CHROOT_DIR)

  def _TranslatePath(self, path, src_root, dst_root_input):
    """If |path| starts with |src_root|, replace it using |dst_root_input|.

    Args:
      path: An absolute path we want to convert to a destination equivalent.
      src_root: The root that path needs to be contained in.
      dst_root_input: The root we want to relocate the relative path into, or a
        function returning this value.

    Returns:
      A translated path, or None if |src_root| is not a prefix of |path|.

    Raises:
      ValueError: If |src_root| is a prefix but |dst_root_input| yields None,
        which means we don't have sufficient information to do the translation.
    """
    if not path.startswith(os.path.join(src_root, '')) and path != src_root:
      return None
    dst_root = dst_root_input() if callable(dst_root_input) else dst_root_input
    if dst_root is None:
      raise ValueError('No target root to translate path to')
    return os.path.join(dst_root, path[len(src_root):].lstrip(os.path.sep))

  def _GetChrootPath(self, path):
    """Translates a fully-expanded host |path| into a chroot equivalent.

    This checks path prefixes in order from the most to least "contained": the
    chroot itself, then the cache directory, and finally the source tree. The
    idea is to return the shortest possible chroot equivalent.

    Args:
      path: A host path to translate.

    Returns:
      An equivalent chroot path.

    Raises:
      ValueError: If |path| is not reachable from the chroot.
    """
    new_path = None

    # Preliminary: compute the actual source and chroot paths to use. These are
    # generally the precomputed values, unless we're inferring the source root
    # from the path itself.
    source_path = self._source_path
    chroot_path = self._chroot_path
    if self._source_from_path_repo:
      path_repo_dir = git.FindRepoDir(path)
      if path_repo_dir is not None:
        source_path = os.path.abspath(os.path.join(path_repo_dir, '..'))
      chroot_path = self._GetSourcePathChroot(source_path)

    # First, check if the path happens to be in the chroot already.
    if chroot_path is not None:
      new_path = self._TranslatePath(path, chroot_path, '/')

    # Second, check the cache directory.
    if new_path is None:
      new_path = self._TranslatePath(path, self._GetCachePath(),
                                     constants.CHROOT_CACHE_ROOT)

    # Finally, check the current SDK checkout tree.
    if new_path is None and source_path is not None:
      new_path = self._TranslatePath(path, source_path,
                                     constants.CHROOT_SOURCE_ROOT)

    if new_path is None:
      raise ValueError('Path is not reachable from the chroot')

    return new_path

  def _GetHostPath(self, path):
    """Translates a fully-expanded chroot |path| into a host equivalent.

    We first attempt translation of known roots (source). If any is successful,
    we check whether the result happens to point back to the chroot, in which
    case we trim the chroot path prefix and recurse. If neither was successful,
    just prepend the chroot path.

    Args:
      path: A chroot path to translate.

    Returns:
      An equivalent host path.

    Raises:
      ValueError: If |path| could not be mapped to a proper host destination.
    """
    new_path = None

    # Attempt resolution of known roots.
    for src_root, dst_root in self._chroot_to_host_roots:
      new_path = self._TranslatePath(path, src_root, dst_root)
      if new_path is not None:
        break

    if new_path is None:
      # If no known root was identified, just prepend the chroot path.
      new_path = self._TranslatePath(path, '', self._chroot_path)
    else:
      # Check whether the resolved path happens to point back at the chroot, in
      # which case trim the chroot path prefix and continue recursively.
      path = self._TranslatePath(new_path, self._chroot_path, '/')
      if path is not None:
        new_path = self._GetHostPath(path)

    return new_path

  def _ConvertPath(self, path, get_converted_path):
    """Expands |path|; if outside the chroot, applies |get_converted_path|.

    Args:
      path: A path to be converted.
      get_converted_path: A conversion function.

    Returns:
      An expanded and (if needed) converted path.

    Raises:
      ValueError: If path conversion failed.
    """
    # NOTE: We do not want to expand wrapper script symlinks because this
    # prevents them from working. Therefore, if the path points to a file we
    # only resolve its dirname but leave the basename intact. This means our
    # path resolution might return unusable results for file symlinks that
    # point outside the reachable space. These are edge cases in which the user
    # is expected to resolve the realpath themselves in advance.
    expanded_path = os.path.expanduser(path)
    if os.path.isfile(expanded_path):
      expanded_path = os.path.join(
          os.path.realpath(os.path.dirname(expanded_path)),
          os.path.basename(expanded_path))
    else:
      expanded_path = os.path.realpath(expanded_path)

    if self._inside_chroot:
      return expanded_path

    try:
      return get_converted_path(expanded_path)
    except ValueError as e:
      raise ValueError('%s: %s' % (e, path))

  def ToChroot(self, path):
    """Resolves current environment |path| for use in the chroot."""
    return self._ConvertPath(path, self._GetChrootPath)

  def FromChroot(self, path):
    """Resolves chroot |path| for use in the current environment."""
    return self._ConvertPath(path, self._GetHostPath)


def DetermineCheckout(cwd):
  """Gather information on the checkout we are in.

  There are several checkout types, as defined by CHECKOUT_TYPE_XXX variables.
  This function determines what checkout type |cwd| is in, for example, if |cwd|
  belongs to a `repo` checkout.

  Returns:
    A CheckoutInfo object with these attributes:
      type: The type of checkout.  Valid values are CHECKOUT_TYPE_*.
      root: The root of the checkout.
      chrome_src_dir: If the checkout is a Chrome checkout, the path to the
        Chrome src/ directory.
  """
  checkout_type = CHECKOUT_TYPE_UNKNOWN
  root, path = None, None

  for path in osutils.IteratePathParents(cwd):
    gclient_file = os.path.join(path, '.gclient')
    if os.path.exists(gclient_file):
      checkout_type = CHECKOUT_TYPE_GCLIENT
      break
    repo_dir = os.path.join(path, '.repo')
    if os.path.isdir(repo_dir):
      checkout_type = CHECKOUT_TYPE_REPO
      break

  if checkout_type != CHECKOUT_TYPE_UNKNOWN:
    root = path

  # Determine the chrome src directory.
  chrome_src_dir = None
  if checkout_type == CHECKOUT_TYPE_GCLIENT:
    chrome_src_dir = os.path.join(root, 'src')

  return CheckoutInfo(checkout_type, root, chrome_src_dir)


def FindCacheDir():
  """Returns the cache directory location based on the checkout type."""
  cwd = os.getcwd()
  checkout = DetermineCheckout(cwd)
  path = None
  if checkout.type == CHECKOUT_TYPE_REPO:
    path = os.path.join(checkout.root, GENERAL_CACHE_DIR)
  elif checkout.type == CHECKOUT_TYPE_GCLIENT:
    path = os.path.join(checkout.root, CHROME_CACHE_DIR)
  elif checkout.type == CHECKOUT_TYPE_UNKNOWN:
    path = os.path.join(tempfile.gettempdir(), 'chromeos-cache')
  else:
    raise AssertionError('Unexpected type %s' % checkout.type)

  return path


def GetCacheDir():
  """Returns the current cache dir."""
  return os.environ.get(constants.SHARED_CACHE_ENVVAR, FindCacheDir())


def ToChrootPath(path):
  """Resolves current environment |path| for use in the chroot."""
  return ChrootPathResolver().ToChroot(path)


def FromChrootPath(path):
  """Resolves chroot |path| for use in the current environment."""
  return ChrootPathResolver().FromChroot(path)
