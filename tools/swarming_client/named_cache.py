# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""This file implements Named Caches."""

import contextlib
import logging
import optparse
import os
import random
import re
import string
import sys

from utils import lru
from utils import file_path
from utils import fs
from utils import threading_utils

import local_caching

# Keep synced with task_request.py
CACHE_NAME_RE = re.compile(ur'^[a-z0-9_]{1,4096}$')


class Error(Exception):
  """Named cache specific error."""


class CacheManager(object):
  """Manages cache directories exposed to a task.

  A task can specify that caches should be present on a bot. A cache is
  tuple (name, path), where
    name is a short identifier that describes the contents of the cache, e.g.
      "git_v8" could be all git repositories required by v8 builds, or
      "build_chromium" could be build artefacts of the Chromium.
    path is a directory path relative to the task run dir. Cache installation
      puts the requested cache directory at the path.
    policies is a local_caching.CachePolicies instance.
  """

  def __init__(self, root_dir, policies):
    """Initializes NamedCaches.

    |root_dir| is a directory for persistent cache storage.
    """
    assert isinstance(root_dir, unicode), root_dir
    assert file_path.isabs(root_dir), root_dir
    self.root_dir = root_dir
    self._policies = policies
    self._lock = threading_utils.LockWithAssert()
    # LRU {cache_name -> cache_location}
    # It is saved to |root_dir|/state.json.
    self._lru = None

  @contextlib.contextmanager
  def open(self, time_fn=None):
    """Opens NamedCaches for mutation operations, such as install.

    Only one caller can open the cache manager at a time. If the same thread
    calls this function after opening it earlier, the call will deadlock.

    time_fn is a function that returns timestamp (float) and used to take
    timestamps when new caches are requested.

    Returns a context manager that must be closed as soon as possible.
    """
    with self._lock:
      state_path = os.path.join(self.root_dir, u'state.json')
      assert self._lru is None, 'acquired lock, but self._lru is not None'
      if os.path.isfile(state_path):
        try:
          self._lru = lru.LRUDict.load(state_path)
        except ValueError:
          logging.exception('failed to load named cache state file')
          logging.warning('deleting named caches')
          file_path.rmtree(self.root_dir)
      self._lru = self._lru or lru.LRUDict()
      if time_fn:
        self._lru.time_fn = time_fn
      try:
        yield
      finally:
        file_path.ensure_tree(self.root_dir)
        self._lru.save(state_path)
        self._lru = None

  def __len__(self):
    """Returns number of items in the cache.

    NamedCache must be open.
    """
    return len(self._lru)

  def get_oldest(self):
    """Returns name of the LRU cache or None.

    NamedCache must be open.
    """
    self._lock.assert_locked()
    try:
      return self._lru.get_oldest()[0]
    except KeyError:
      return None

  def get_timestamp(self, name):
    """Returns timestamp of last use of an item.

    NamedCache must be open.

    Raises KeyError if cache is not found.
    """
    self._lock.assert_locked()
    assert isinstance(name, basestring), name
    return self._lru.get_timestamp(name)

  @property
  def available(self):
    """Returns a set of names of available caches.

    NamedCache must be open.
    """
    self._lock.assert_locked()
    return self._lru.keys_set()

  def install(self, path, name):
    """Moves the directory for the specified named cache to |path|.

    NamedCache must be open. path must be absolute, unicode and must not exist.

    Raises Error if cannot install the cache.
    """
    self._lock.assert_locked()
    logging.info('Installing named cache %r to %r', name, path)
    try:
      _check_abs(path)
      if os.path.isdir(path):
        raise Error('installation directory %r already exists' % path)

      rel_cache = self._lru.get(name)
      if rel_cache:
        abs_cache = os.path.join(self.root_dir, rel_cache)
        if os.path.isdir(abs_cache):
          logging.info('Moving %r to %r', abs_cache, path)
          file_path.ensure_tree(os.path.dirname(path))
          fs.rename(abs_cache, path)
          self._remove(name)
          return

        logging.warning('directory for named cache %r does not exist', name)
        self._remove(name)

      # The named cache does not exist, create an empty directory.
      # When uninstalling, we will move it back to the cache and create an
      # an entry.
      file_path.ensure_tree(path)
    except (OSError, Error) as ex:
      raise Error(
          'cannot install cache named %r at %r: %s' % (
            name, path, ex))

  def uninstall(self, path, name):
    """Moves the cache directory back. Opposite to install().

    NamedCache must be open. path must be absolute and unicode.

    Raises Error if cannot uninstall the cache.
    """
    logging.info('Uninstalling named cache %r from %r', name, path)
    try:
      _check_abs(path)
      if not os.path.isdir(path):
        logging.warning(
            'Directory %r does not exist anymore. Cache lost.', path)
        return

      rel_cache = self._lru.get(name)
      if rel_cache:
        # Do not crash because cache already exists.
        logging.warning('overwriting an existing named cache %r', name)
        create_named_link = False
      else:
        rel_cache = self._allocate_dir()
        create_named_link = True

      # Move the dir and create an entry for the named cache.
      abs_cache = os.path.join(self.root_dir, rel_cache)
      logging.info('Moving %r to %r', path, abs_cache)
      file_path.ensure_tree(os.path.dirname(abs_cache))
      fs.rename(path, abs_cache)
      self._lru.add(name, rel_cache)

      if create_named_link:
        # Create symlink <root_dir>/<named>/<name> -> <root_dir>/<short name>
        # for user convenience.
        named_path = self._get_named_path(name)
        if os.path.exists(named_path):
          file_path.remove(named_path)
        else:
          file_path.ensure_tree(os.path.dirname(named_path))
        try:
          fs.symlink(abs_cache, named_path)
          logging.info('Created symlink %r to %r', named_path, abs_cache)
        except OSError:
          # Ignore on Windows. It happens when running as a normal user or when
          # UAC is enabled and the user is a filtered administrator account.
          if sys.platform != 'win32':
            raise
    except (OSError, Error) as ex:
      raise Error(
          'cannot uninstall cache named %r at %r: %s' % (
            name, path, ex))

  def trim(self):
    """Purges cache entries that do not comply with the cache policies.

    NamedCache must be open.

    Returns:
      Number of caches deleted.
    """
    self._lock.assert_locked()
    if not os.path.isdir(self.root_dir):
      return 0

    removed = []

    def _remove_lru_file():
      """Removes the oldest LRU entry. LRU must not be empty."""
      name, _data = self._lru.get_oldest()
      logging.info('Removing named cache %r', name)
      self._remove(name)
      removed.append(name)

    # Trim according to maximum number of items.
    while len(self._lru) > self._policies.max_items:
      _remove_lru_file()

    # Trim according to maximum age.
    if self._policies.max_age_secs:
      cutoff = self._lru.time_fn() - self._policies.max_age_secs
      while self._lru:
        _name, (_content, timestamp) = self._lru.get_oldest()
        if timestamp >= cutoff:
          break
        _remove_lru_file()

    # Trim according to minimum free space.
    if self._policies.min_free_space:
      while True:
        free_space = file_path.get_free_space(self.root_dir)
        if not self._lru or free_space >= self._policies.min_free_space:
          break
        _remove_lru_file()

    # TODO(maruel): Trim according to self._policies.max_cache_size. Do it last
    # as it requires counting the size of each entry.

    # TODO(maruel): Trim empty directories. An empty directory is not a cache,
    # something needs to be in it.

    return len(removed)

  _DIR_ALPHABET = string.ascii_letters + string.digits

  def _allocate_dir(self):
    """Creates and returns relative path of a new cache directory."""
    # We randomly generate directory names that have two lower/upper case
    # letters or digits. Total number of possibilities is (26*2 + 10)^2 = 3844.
    abc_len = len(self._DIR_ALPHABET)
    tried = set()
    while len(tried) < 1000:
      i = random.randint(0, abc_len * abc_len - 1)
      rel_path = (
        self._DIR_ALPHABET[i / abc_len] +
        self._DIR_ALPHABET[i % abc_len])
      if rel_path in tried:
        continue
      abs_path = os.path.join(self.root_dir, rel_path)
      if not fs.exists(abs_path):
        return rel_path
      tried.add(rel_path)
    raise Error('could not allocate a new cache dir, too many cache dirs')

  def _remove(self, name):
    """Removes a cache directory and entry.

    NamedCache must be open.

    Returns:
      Number of caches deleted.
    """
    self._lock.assert_locked()
    rel_path = self._lru.get(name)
    if not rel_path:
      return

    named_dir = self._get_named_path(name)
    if fs.islink(named_dir):
      fs.unlink(named_dir)

    abs_path = os.path.join(self.root_dir, rel_path)
    if os.path.isdir(abs_path):
      file_path.rmtree(abs_path)
    self._lru.pop(name)

  def _get_named_path(self, name):
    return os.path.join(self.root_dir, 'named', name)


def add_named_cache_options(parser):
  group = optparse.OptionGroup(parser, 'Named caches')
  group.add_option(
      '--named-cache',
      dest='named_caches',
      action='append',
      nargs=2,
      default=[],
      help='A named cache to request. Accepts two arguments, name and path. '
           'name identifies the cache, must match regex [a-z0-9_]{1,4096}. '
           'path is a path relative to the run dir where the cache directory '
           'must be put to. '
           'This option can be specified more than once.')
  group.add_option(
      '--named-cache-root',
      help='Cache root directory. Default=%default')
  parser.add_option_group(group)


def process_named_cache_options(parser, options):
  """Validates named cache options and returns a CacheManager."""
  if options.named_caches and not options.named_cache_root:
    parser.error('--named-cache is specified, but --named-cache-root is empty')
  for name, path in options.named_caches:
    if not CACHE_NAME_RE.match(name):
      parser.error(
          'cache name %r does not match %r' % (name, CACHE_NAME_RE.pattern))
    if not path:
      parser.error('cache path cannot be empty')
  if options.named_cache_root:
    # Make these configurable later if there is use case but for now it's fairly
    # safe values.
    # In practice, a fair chunk of bots are already recycled on a daily schedule
    # so this code doesn't have any effect to them, unless they are preloaded
    # with a really old cache.
    policies = local_caching.CachePolicies(
        # 1TiB.
        max_cache_size=1024*1024*1024*1024,
        min_free_space=options.min_free_space,
        max_items=50,
        # 3 weeks.
        max_age_secs=21*24*60*60)
    root_dir = unicode(os.path.abspath(options.named_cache_root))
    return CacheManager(root_dir, policies)
  return None


def _check_abs(path):
  if not isinstance(path, unicode):
    raise Error('named cache installation path must be unicode')
  if not os.path.isabs(path):
    raise Error('named cache installation path must be absolute')
