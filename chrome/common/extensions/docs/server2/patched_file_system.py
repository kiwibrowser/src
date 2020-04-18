# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from copy import deepcopy

from file_system import FileSystem, StatInfo, FileNotFoundError
from future import Future


class PatchedFileSystem(FileSystem):
  ''' Class to fetch resources with a patch applied.
  '''
  def __init__(self, base_file_system, patcher):
    self._base_file_system = base_file_system
    self._patcher = patcher

  def Read(self, paths, skip_not_found=False):
    patched_files = set()
    added, deleted, modified = self._patcher.GetPatchedFiles()
    if set(paths) & set(deleted):
      def raise_file_not_found():
        raise FileNotFoundError('Files are removed from the patch.')
      return Future(callback=raise_file_not_found)

    patched_files |= (set(added) | set(modified))
    dir_paths = set(path for path in paths if path.endswith('/'))
    file_paths = set(paths) - dir_paths
    patched_paths = file_paths & patched_files
    unpatched_paths = file_paths - patched_files

    def patch_directory_listing(path, original_listing):
      added, deleted, modified = (
          self._GetDirectoryListingFromPatch(path))
      if original_listing is None:
        if len(added) == 0:
          raise FileNotFoundError('Directory %s not found in the patch.' % path)
        return added
      return list((set(original_listing) | set(added)) - set(deleted))

    def next(files):
      dirs_value = self._TryReadDirectory(dir_paths)
      files.update(self._patcher.Apply(patched_paths,
                                       self._base_file_system).Get())
      files.update(dict((path, patch_directory_listing(path, dirs_value[path]))
                        for path in dirs_value))
      return files
    return self._base_file_system.Read(unpatched_paths,
                                       skip_not_found=skip_not_found).Then(next)

  def Refresh(self):
    return self._base_file_system.Refresh()

  ''' Given the list of patched files, it's not possible to determine whether
  a directory to read exists in self._base_file_system. So try reading each one
  and handle FileNotFoundError.
  '''
  def _TryReadDirectory(self, paths):
    value = {}
    for path in paths:
      assert path.endswith('/')
      try:
        value[path] = self._base_file_system.ReadSingle(path).Get()
      except FileNotFoundError:
        value[path] = None
    return value

  def _GetDirectoryListingFromPatch(self, path):
    assert path.endswith('/')
    def _FindChildrenInPath(files, path):
      result = []
      for f in files:
        if f.startswith(path):
          child_path = f[len(path):]
          if '/' in child_path:
            child_name = child_path[0:child_path.find('/') + 1]
          else:
            child_name = child_path
          result.append(child_name)
      return result

    added, deleted, modified = (tuple(
        _FindChildrenInPath(files, path)
        for files in self._patcher.GetPatchedFiles()))

    # A patch applies to files only. It cannot delete directories.
    deleted_files = [child for child in deleted if not child.endswith('/')]
    # However, these directories are actually modified because their children
    # are patched.
    modified += [child for child in deleted if child.endswith('/')]

    return (added, deleted_files, modified)

  def _PatchStat(self, stat_info, version, added, deleted, modified):
    assert len(added) + len(deleted) + len(modified) > 0
    assert stat_info.child_versions is not None

    # Deep copy before patching to make sure it doesn't interfere with values
    # cached in memory.
    stat_info = deepcopy(stat_info)

    stat_info.version = version
    for child in added + modified:
      stat_info.child_versions[child] = version
    for child in deleted:
      if stat_info.child_versions.get(child):
        del stat_info.child_versions[child]

    return stat_info

  def Stat(self, path):
    version = self._patcher.GetVersion()
    assert version is not None
    version = 'patched_%s' % version

    directory, filename = path.rsplit('/', 1)
    added, deleted, modified = self._GetDirectoryListingFromPatch(
        directory + '/')

    if len(added) > 0:
      # There are new files added. It's possible (if |directory| is new) that
      # self._base_file_system.Stat will throw an exception.
      try:
        stat_info = self._PatchStat(
            self._base_file_system.Stat(directory + '/'),
            version,
            added,
            deleted,
            modified)
      except FileNotFoundError:
        stat_info = StatInfo(
            version,
            dict((child, version) for child in added + modified))
    elif len(deleted) + len(modified) > 0:
      # No files were added.
      stat_info = self._PatchStat(self._base_file_system.Stat(directory + '/'),
                                  version,
                                  added,
                                  deleted,
                                  modified)
    else:
      # No changes are made in this directory.
      return self._base_file_system.Stat(path)

    if stat_info.child_versions is not None:
      if filename:
        if filename in stat_info.child_versions:
          stat_info = StatInfo(stat_info.child_versions[filename])
        else:
          raise FileNotFoundError('%s was not in child versions' % filename)
    return stat_info

  def GetIdentity(self):
    return '%s(%s,%s)' % (self.__class__.__name__,
                          self._base_file_system.GetIdentity(),
                          self._patcher.GetIdentity())
