#!/usr/bin/env python2
# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Functionality for mangling repository checkouts that are shared

In particular, this in combination w/ enter_chroot's mount binding, allows
us to access the same repo from inside and outside a chroot at the same time
"""

from __future__ import print_function

__all__ = ('RebuildRepoCheckout',)

import sys
import os
import shutil
import errno

_path = os.path.realpath(__file__)
_path = os.path.normpath(os.path.join(os.path.dirname(_path), '..', '..'))
sys.path.insert(0, _path)
del _path

from chromite.lib import cros_build_lib
from chromite.lib import git
from chromite.lib import osutils


_CACHE_NAME = '.cros_projects.list'


def _FilterNonExistentProjects(project_dir, projects):
  for project in projects:
    if os.path.exists(os.path.join(project_dir, project)):
      yield project


def _CleanAlternates(projects, alt_root):
  alt_root = os.path.normpath(alt_root)

  projects = set(projects)
  # Ignore our cache.
  projects.add(_CACHE_NAME)
  required_directories = set(os.path.dirname(x) for x in projects)

  for abs_root, dirs, files in os.walk(alt_root):
    rel_root = abs_root[len(alt_root):].strip('/')

    if rel_root not in required_directories:
      shutil.rmtree(abs_root)
      dirs[:] = []
      continue

    if rel_root:
      for filename in files:
        if os.path.join(rel_root, filename) not in projects:
          os.unlink(os.path.join(abs_root, filename))


def _UpdateAlternatesDir(alternates_root, reference_maps, projects):
  for project in projects:
    alt_path = os.path.join(alternates_root, project)
    paths = []
    for k, v in reference_maps.iteritems():
      suffix = os.path.join('.repo', 'project-objects', project, 'objects')
      if os.path.exists(os.path.join(k, suffix)):
        paths.append(os.path.join(v, suffix))

    osutils.SafeMakedirs(os.path.dirname(alt_path))
    osutils.WriteFile(alt_path, '%s\n' % ('\n'.join(paths),), atomic=True)


def _UpdateGitAlternates(proj_root, projects):
  for project in projects:

    alt_path = os.path.join(proj_root, project, 'objects', 'info',
                            'alternates')
    tmp_path = '%s.tmp' % alt_path

    # Clean out any tmp files that may have existed prior.
    osutils.SafeUnlink(tmp_path)

    # The pathway is written relative to the alternates files absolute path;
    # literally, .repo/projects/chromite.git/objects/info/alternates.
    relpath = '../' * (project.count('/') + 4)
    relpath = os.path.join(relpath, 'alternates', project)

    osutils.SafeMakedirs(os.path.dirname(tmp_path))
    os.symlink(relpath, tmp_path)
    os.rename(tmp_path, alt_path)


def _GetProjects(repo_root):
  # Note that we cannot rely upon projects.list, nor repo list, nor repo forall
  # here to be authoritive.
  # if we rely on the manifest contents, the local tree may not yet be
  # updated- thus if we drop the alternate for that project, that project is no
  # longer usable (which can tick off repo sync).
  # Thus, we just iterate over the raw underlying projects store, and generate
  # alternates for that; we regenerate based on either the manifest changing,
  # local_manifest having changed, or projects.list having changed (which
  # occurs during partial local repo syncs; aka repo sync chromite for
  # example).
  # Finally, note we have to truncate our mtime awareness to just integers;
  # this is required since utime isn't guaranteed to set floats, despite our
  # being able to get a float back from stat'ing.

  manifest_xml = os.path.join(repo_root, 'manifest.xml')
  times = [os.lstat(manifest_xml).st_mtime,
           os.stat(manifest_xml).st_mtime]
  for path in ('local_manifest.xml', 'project.list'):
    path = os.path.join(repo_root, path)
    if os.path.exists(path):
      times.append(os.stat(path).st_mtime)

  manifest_time = long(max(times))

  cache_path = os.path.join(repo_root, _CACHE_NAME)

  try:
    if long(os.stat(cache_path).st_mtime) == manifest_time:
      return osutils.ReadFile(cache_path).split()
  except EnvironmentError as e:
    if e.errno != errno.ENOENT:
      raise

  # The -a ! section of this find invocation is to block descent
  # into the actual git repository; for IO constrained systems,
  # this avoids a fairly large amount of inode/dentry load up.
  # TLDR; It's faster, don't remove it ;)
  data = cros_build_lib.RunCommand(
      ['find', './', '-type', 'd', '-name', '*.git', '-a',
       '!', '-wholename', '*/*.git/*', '-prune'],
      cwd=os.path.join(repo_root, 'project-objects'), capture_output=True)

  # Drop the leading ./ and the trailing .git
  data = [x[2:-4] for x in data.output.splitlines() if x]

  with open(cache_path, 'w') as f:
    f.write('\n'.join(sorted(data)))

  # Finally, mark the cache with the time of the manifest.xml we examined.
  os.utime(cache_path, (manifest_time, manifest_time))
  return data


class Failed(Exception):
  """Exception used to fail out for a bad environment."""


def _RebuildRepoCheckout(target_root, reference_map,
                         alternates_dir):
  repo_root = os.path.join(target_root, '.repo')
  proj_root = os.path.join(repo_root, 'project-objects')

  manifest_path = os.path.join(repo_root, 'manifest.xml')
  if not os.path.exists(manifest_path):
    raise Failed('%r does not exist, thus cannot be a repo checkout' %
                 manifest_path)

  projects = ['%s.git' % x for x in _GetProjects(repo_root)]
  projects = _FilterNonExistentProjects(proj_root, projects)
  projects = list(sorted(projects))

  if not osutils.SafeMakedirs(alternates_dir, 0o775):
    # We know the directory exists; thus cleanse out
    # dead alternates.
    _CleanAlternates(projects, alternates_dir)

  _UpdateAlternatesDir(alternates_dir, reference_map, projects)
  _UpdateGitAlternates(proj_root, projects)


def WalkReferences(repo_root, max_depth=5, suppress=()):
  """Given a repo checkout root, find the repo's it references up to max_depth.

  Args:
    repo_root: The root of a repo checkout to start from
    max_depth: Git internally limits the max alternates depth to 5;
      this option exists to adjust how deep we're willing to look.
    suppress: List of repos already seen (and so to ignore).

  Returns:
    List of repository roots required for this repo_root.
  """

  original_root = repo_root
  seen = set(os.path.abspath(x) for x in suppress)

  for _x in xrange(0, max_depth):
    repo_root = os.path.abspath(repo_root)

    if repo_root in seen:
      # Cyclic reference graph; break out of it, if someone induced this the
      # necessary objects should be in place.  If they aren't, really isn't
      # much that can be done.
      return

    yield repo_root
    seen.add(repo_root)
    base = os.path.join(repo_root, '.repo', 'manifests.git')
    result = git.RunGit(
        base, ['config', 'repo.reference'], error_code_ok=True)

    if result.returncode not in (0, 1):
      raise Failed('Unexpected returncode %i from examining %s git '
                   'repo.reference configuration' %
                   (result.returncode, base))

    repo_root = result.output.strip()
    if not repo_root:
      break

  else:
    raise Failed('While tracing out the references of %s, we recursed more '
                 'than the allowed %i times ending at %s'
                 % (original_root, max_depth, repo_root))


def RebuildRepoCheckout(repo_root, initial_reference,
                        chroot_reference_root=None):
  """Rebuild a repo checkout's 'alternate tree' rewriting the repo to use it

  Args:
    repo_root: absolute path to the root of a repository checkout
    initial_reference: absolute path to the root of the repository that is
      shared
    chroot_reference_root: if given, repo_root will have it's chroot
      alternates tree configured with this pathway, enabling repo access to
      work from within the chroot.
  """

  reference_roots = list(WalkReferences(initial_reference,
                                        suppress=[repo_root]))

  # Always rebuild the external alternates for any operation; 1) we don't want
  # external out of sync from chroot, 2) if this is the first conversion, if
  # we only update chroot it'll break external access to the repo.
  reference_map = dict((x, x) for x in reference_roots)
  rebuilds = [('alternates', reference_map)]
  if chroot_reference_root:
    alternates_dir = 'chroot/alternates'
    base = os.path.join(chroot_reference_root, '.repo', 'chroot', 'external')
    reference_map = dict((x, '%s%i' % (base, idx + 1))
                         for idx, x in enumerate(reference_roots))
    rebuilds += [('chroot/alternates', reference_map)]

  for alternates_dir, reference_map in rebuilds:
    alternates_dir = os.path.join(repo_root, '.repo', alternates_dir)
    _RebuildRepoCheckout(repo_root,
                         reference_map,
                         alternates_dir)
  return reference_roots


if __name__ == '__main__':
  chroot_root = None
  if len(sys.argv) not in (3, 4):
    sys.stderr.write('Usage: %s <repository_root> <referenced_repository> '
                     '[path_from_within_the_chroot]\n' % (sys.argv[0],))
    sys.exit(1)
  if len(sys.argv) == 4:
    chroot_root = sys.argv[3]
  ret = RebuildRepoCheckout(sys.argv[1], sys.argv[2],
                            chroot_reference_root=chroot_root)
  print('\n'.join(ret))
