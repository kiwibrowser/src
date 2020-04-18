# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict
import posixpath

from future import Future
from path_util import SplitParent
from special_paths import SITE_VERIFICATION_FILE

def _Normalize(file_name, splittext=False):
  normalized = file_name
  if splittext:
    normalized = posixpath.splitext(file_name)[0]
  normalized = normalized.replace('.', '').replace('-', '').replace('_', '')
  return normalized.lower()

def _CommonNormalizedPrefix(first_file, second_file):
  return posixpath.commonprefix((_Normalize(first_file),
                                 _Normalize(second_file)))


class PathCanonicalizer(object):
  '''Transforms paths into their canonical forms. Since the docserver has had
  many incarnations - e.g. there didn't use to be apps/ - there may be old
  paths lying around the webs. We try to redirect those to where they are now.
  '''
  def __init__(self,
               file_system,
               object_store_creator,
               strip_extensions):
    # |strip_extensions| is a list of file extensions (e.g. .html) that should
    # be stripped for a path's canonical form.
    self._cache = object_store_creator.Create(
        PathCanonicalizer, category=file_system.GetIdentity())
    self._file_system = file_system
    self._strip_extensions = strip_extensions

  def _LoadCache(self):
    def load(cached):
      # |canonical_paths| is the pre-calculated set of canonical paths.
      # |simplified_paths_map| is a lazily populated mapping of simplified file
      # names to a list of full paths that contain them. For example,
      #  - browseraction: [extensions/browserAction.html]
      #  - storage: [apps/storage.html, extensions/storage.html]
      canonical_paths, simplified_paths_map = (
          cached.get('canonical_paths'), cached.get('simplified_paths_map'))

      if canonical_paths is None:
        assert simplified_paths_map is None
        canonical_paths = set()
        simplified_paths_map = defaultdict(list)
        for base, dirs, files in self._file_system.Walk(''):
          for path in dirs + files:
            path_without_ext, ext = posixpath.splitext(path)
            canonical_path = posixpath.join(base, path_without_ext)
            if (ext not in self._strip_extensions or
                path == SITE_VERIFICATION_FILE):
              canonical_path += ext
            canonical_paths.add(canonical_path)
            simplified_paths_map[_Normalize(path, splittext=True)].append(
                canonical_path)
        # Store |simplified_paths_map| sorted. Ties in length are broken by
        # taking the shortest, lexicographically smallest path.
        for path_list in simplified_paths_map.itervalues():
          path_list.sort(key=lambda p: (len(p), p))
        self._cache.SetMulti({
          'canonical_paths': canonical_paths,
          'simplified_paths_map': simplified_paths_map,
        })
      else:
        assert simplified_paths_map is not None

      return canonical_paths, simplified_paths_map
    return self._cache.GetMulti(('canonical_paths',
                                 'simplified_paths_map')).Then(load)


  def Canonicalize(self, path):
    '''Returns the canonical path for |path|.
    '''
    canonical_paths, simplified_paths_map = self._LoadCache().Get()

    # Path may already be the canonical path.
    if path in canonical_paths:
      return path

    # Path not found. Our single heuristic: find |base| in the directory
    # structure with the longest common prefix of |path|.
    _, base = SplitParent(path)

    # Paths with a non-extension dot separator lose information in
    # _SimplifyFileName, so we try paths both with and without the dot to
    # maximize the possibility of finding the right path.
    potential_paths = (
        simplified_paths_map.get(_Normalize(base), []) +
        simplified_paths_map.get(_Normalize(base, splittext=True), []))

    if potential_paths == []:
      # There is no file with anything close to that name.
      return path

    # The most likely canonical file is the one with the longest common prefix
    # with |path|. This is slightly weaker than it could be; |path| is
    # compared without symbols, not the simplified form of |path|,
    # which may matter.
    max_prefix = potential_paths[0]
    max_prefix_length = len(_CommonNormalizedPrefix(max_prefix, path))
    for path_for_file in potential_paths[1:]:
      prefix_length = len(_CommonNormalizedPrefix(path_for_file, path))
      if prefix_length > max_prefix_length:
        max_prefix, max_prefix_length = path_for_file, prefix_length

    return max_prefix

  def Refresh(self):
    return self._LoadCache()
