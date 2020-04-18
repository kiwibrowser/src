# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from file_system import FileSystem, FileNotFoundError, StatInfo
from future import Future
from path_util import AssertIsValid, AssertIsDirectory, IsDirectory


def MoveTo(base, obj):
  '''Returns an object as |obj| moved to |base|. That is,
  MoveTo('foo/bar', {'a': 'b'}) -> {'foo': {'bar': {'a': 'b'}}}
  '''
  AssertIsDirectory(base)
  result = {}
  leaf = result
  for k in base.rstrip('/').split('/'):
    leaf[k] = {}
    leaf = leaf[k]
  leaf.update(obj)
  return result


def MoveAllTo(base, obj):
  '''Moves every value in |obj| to |base|. See MoveTo.
  '''
  result = {}
  for key, value in obj.iteritems():
    result[key] = MoveTo(base, value)
  return result


def _List(file_system):
  '''Returns a list of '/' separated paths derived from |file_system|.
  For example, {'index.html': '', 'www': {'file.txt': ''}} would return
  ['index.html', 'www/file.txt'].
  '''
  assert isinstance(file_system, dict)
  result = {}
  def update_result(item, path):
    AssertIsValid(path)
    if isinstance(item, dict):
      if path != '':
        path += '/'
      result[path] = [p if isinstance(content, basestring) else (p + '/')
                      for p, content in item.iteritems()]
      for subpath, subitem in item.iteritems():
        update_result(subitem, path + subpath)
    elif isinstance(item, basestring):
      result[path] = item
    else:
      raise ValueError('Unsupported item type: %s' % type(item))
  update_result(file_system, '')
  return result


class _StatTracker(object):
  '''Maintains the versions of paths in a file system. The versions of files
  are changed either by |Increment| or |SetVersion|. The versions of
  directories are derived from the versions of files within it.
  '''

  def __init__(self):
    self._path_stats = {}
    self._global_stat = 0

  def Increment(self, path=None, by=1):
    if path is None:
      self._global_stat += by
    else:
      self.SetVersion(path, self._path_stats.get(path, 0) + by)

  def SetVersion(self, path, new_version):
    if IsDirectory(path):
      raise ValueError('Only files have an incrementable stat, '
                       'but "%s" is a directory' % path)

    # Update version of that file.
    self._path_stats[path] = new_version

    # Update all parent directory versions as well.
    slash_index = 0  # (deliberately including '' in the dir paths)
    while slash_index != -1:
      dir_path = path[:slash_index] + '/'
      self._path_stats[dir_path] = max(self._path_stats.get(dir_path, 0),
                                       new_version)
      if dir_path == '/':
        # Legacy support for '/' being the root of the file system rather
        # than ''. Eventually when the path normalisation logic is complete
        # this will be impossible and this logic will change slightly.
        self._path_stats[''] = self._path_stats['/']
      slash_index = path.find('/', slash_index + 1)

  def GetVersion(self, path):
    return self._global_stat + self._path_stats.get(path, 0)


class TestFileSystem(FileSystem):
  '''A FileSystem backed by an object. Create with an object representing file
  paths such that {'a': {'b': 'hello'}} will resolve Read('a/b') as 'hello',
  Read('a/') as ['b'], and Stat determined by a value incremented via
  IncrementStat.
  '''

  def __init__(self, obj, relative_to=None, identity=None):
    assert obj is not None
    if relative_to is not None:
      obj = MoveTo(relative_to, obj)
    self._identity = identity or type(self).__name__
    self._path_values = _List(obj)
    self._stat_tracker = _StatTracker()

  #
  # FileSystem implementation.
  #

  def Read(self, paths, skip_not_found=False):
    for path in paths:
      if path not in self._path_values:
        if skip_not_found: continue
        return FileNotFoundError.RaiseInFuture(
            '%s not in %s' % (path, '\n'.join(self._path_values)))
    return Future(value=dict((k, v) for k, v in self._path_values.iteritems()
                             if k in paths))

  def Refresh(self):
    return Future(value=())

  def Stat(self, path):
    read_result = self.ReadSingle(path).Get()
    stat_result = StatInfo(str(self._stat_tracker.GetVersion(path)))
    if isinstance(read_result, list):
      stat_result.child_versions = dict(
          (file_result,
           str(self._stat_tracker.GetVersion('%s%s' % (path, file_result))))
          for file_result in read_result)
    return stat_result

  #
  # Testing methods.
  #

  def IncrementStat(self, path=None, by=1):
    self._stat_tracker.Increment(path, by=by)

  def GetIdentity(self):
    return self._identity
