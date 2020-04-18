# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import DEPS
CONFIG_CTX = DEPS['path'].CONFIG_CTX


@CONFIG_CTX()
def infra_common(c):
  c.dynamic_paths['checkout'] = None


@CONFIG_CTX(includes=['infra_common'])
def infra_buildbot(c):
  """Used on BuildBot by "annotated_run"."""
  c.base_paths['root'] = c.START_DIR[:-4]
  c.base_paths['cache'] = c.base_paths['root'] + (
      'build', 'slave', 'cache')
  c.base_paths['git_cache'] = c.base_paths['root'] + (
      'build', 'slave', 'cache_dir')
  c.base_paths['cleanup'] = c.START_DIR[:-1] + ('build.dead',)
  c.base_paths['goma_cache'] = c.base_paths['root'] + (
      'build', 'slave', 'goma_cache')
  for token in ('build_internal', 'build', 'depot_tools'):
    c.base_paths[token] = c.base_paths['root'] + (token,)


@CONFIG_CTX(includes=['infra_common'])
def infra_kitchen(c):
  """Used on BuildBot by "remote_run" when NOT running Kitchen."""
  c.base_paths['root'] = c.START_DIR
  # TODO(phajdan.jr): have one cache dir, let clients append suffixes.

  b_dir = c.START_DIR
  while b_dir and b_dir[-1] != 'b':
    b_dir = b_dir[:-1]

  if c.PLATFORM in ('linux', 'mac'):
    c.base_paths['cache'] = ('/', 'b', 'c')
    c.base_paths['builder_cache'] = c.base_paths['cache'] + ('b',)
    for path in ('git_cache', 'goma_cache'):
      c.base_paths[path] = c.base_paths['cache'] + (path,)
  elif b_dir:
    c.base_paths['cache'] = b_dir + ('c',)
    c.base_paths['builder_cache'] = c.base_paths['cache'] + ('b',)
    for path in ('git_cache', 'goma_cache'):
      c.base_paths[path] = c.base_paths['cache'] + (path,)
  else:  # pragma: no cover
    c.base_paths['cache'] = c.base_paths['root'] + ('c',)
    c.base_paths['builder_cache'] = c.base_paths['cache'] + ('b',)
    c.base_paths['git_cache'] = c.base_paths['root'] + ('cache_dir',)
    for path in ('goma_cache',):
      c.base_paths[path] = c.base_paths['cache'] + (path,)


@CONFIG_CTX()
def infra_generic(c):
  """Used by Kitchen runs on both SwarmBucket and "remote_run"+Kitchen.

  The default path values (ones not set explicitly here) are either installed by
  Kitchen directly, or are recipe engine defaults. Note that in "kitchen",
  the "start_dir" is ephemeral.
  """
  c.base_paths['builder_cache'] = c.base_paths['cache'] + ('builder',)
  c.base_paths['git_cache'] = c.base_paths['cache'] + ('git',)
  c.base_paths['goma_cache'] = c.base_paths['cache'] + ('goma',)
