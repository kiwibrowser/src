# -*- coding: utf-8 -*-
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Manage projects in the local manifest."""

from __future__ import print_function

import platform
import os
import xml.etree.ElementTree as ElementTree

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import git


class LocalManifest(object):
  """Class which provides an abstraction for manipulating the local manifest."""

  @classmethod
  def FromPath(cls, path, empty_if_missing=False):
    if os.path.isfile(path):
      with open(path) as f:
        return cls(f.read())
    elif empty_if_missing:
      cros_build_lib.Die('Manifest file, %r, not found' % path)
    return cls()

  def __init__(self, text=None):
    self._text = text or '<manifest>\n</manifest>'
    self.nodes = ElementTree.fromstring(self._text)

  def AddNonWorkonProject(self, name, path, remote=None, revision=None):
    """Add a new nonworkon project element to the manifest tree."""
    element = ElementTree.Element('project', name=name, path=path,
                                  remote=remote)
    element.attrib['workon'] = 'False'
    if revision is not None:
      element.attrib['revision'] = revision
    self.nodes.append(element)
    return element

  def GetProject(self, name, path=None):
    """Accessor method for getting a project node from the manifest tree.

    Returns:
      project element node from ElementTree, otherwise, None
    """
    if path is None:
      # Use a unique value that can't ever match.
      path = object()
    for project in self.nodes.findall('project'):
      if project.attrib['name'] == name or project.attrib['path'] == path:
        return project
    return None

  def ToString(self):
    # Reset the tail for each node, then just do a hacky replace.
    project = None
    for project in self.nodes.findall('project'):
      project.tail = '\n  '
    if project is not None:
      # Tweak the last project to not have the trailing space.
      project.tail = '\n'
    # Fix manifest tag text and tail.
    self.nodes.text = '\n  '
    self.nodes.tail = '\n'
    return ElementTree.tostring(self.nodes)

  def GetProjects(self):
    return list(self.nodes.findall('project'))


def _AddProjectsToManifestGroups(options, new_group):
  """Enable the given manifest groups for the configured repository."""

  groups_to_enable = ['name:%s' % x for x in new_group]

  git_config = options.git_config

  cmd = ['config', '-f', git_config, '--get', 'manifest.groups']
  enabled_groups = git.RunGit('.', cmd, error_code_ok=True).output.split(',')

  # Note that ordering actually matters, thus why the following code
  # is written this way.
  # Per repo behaviour, enforce an appropriate platform group if
  # we're converting from a default manifest group to a limited one.
  # Finally, note we reprocess the existing groups; this is to allow
  # us to cleanup any user screwups, or our own screwups.
  requested_groups = (
      ['minilayout', 'platform-%s' % (platform.system().lower(),)] +
      enabled_groups + list(groups_to_enable))

  processed_groups = set()
  finalized_groups = []

  for group in requested_groups:
    if group not in processed_groups:
      finalized_groups.append(group)
      processed_groups.add(group)

  cmd = ['config', '-f', git_config, 'manifest.groups',
         ','.join(finalized_groups)]
  git.RunGit('.', cmd)


def _AssertNotMiniLayout():
  cros_build_lib.Die(
      "Your repository checkout is using the old minilayout.xml workflow; "
      "Autoupdate is no longer supported, reinstall your tree.")


def GetParser():
  """Return a command line parser."""
  parser = commandline.ArgumentParser(description=__doc__)

  subparsers = parser.add_subparsers(dest='command')

  subparser = subparsers.add_parser(
      'add',
      help='Add projects to the manifest.')
  subparser.add_argument('-w', '--workon', action='store_true',
                         default=False, help='Is this a workon package?')
  subparser.add_argument('-r', '--remote',
                         help='Remote project name (for non-workon packages).')
  subparser.add_argument('-v', '--revision',
                         help='Use to override the manifest defined default '
                              'revision used for a given project.')
  subparser.add_argument('project', help='Name of project in the manifest.')
  subparser.add_argument('path', nargs='?', help='Local path to the project.')

  return parser


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)
  repo_dir = git.FindRepoDir(os.getcwd())
  if not repo_dir:
    parser.error("This script must be invoked from within a repository "
                 "checkout.")

  options.git_config = os.path.join(repo_dir, 'manifests.git', 'config')
  options.local_manifest_path = os.path.join(repo_dir, 'local_manifest.xml')

  manifest_sym_path = os.path.join(repo_dir, 'manifest.xml')
  if os.path.basename(os.readlink(manifest_sym_path)) == 'minilayout.xml':
    _AssertNotMiniLayout()

  # For now, we only support the add command.
  assert options.command == 'add'
  if options.workon:
    if options.path is not None:
      parser.error('Adding workon projects do not set project.')
  else:
    if options.remote is None:
      parser.error('Adding non-workon projects requires a remote.')
    if options.path is None:
      parser.error('Adding non-workon projects requires a path.')
  name = options.project
  path = options.path
  revision = options.revision
  if revision is not None:
    if (not git.IsRefsTags(revision) and
        not git.IsSHA1(revision)):
      revision = git.StripRefsHeads(revision, False)

  main_manifest = git.ManifestCheckout(os.getcwd())
  main_element = main_manifest.FindCheckouts(name)
  if path is not None:
    main_element_from_path = main_manifest.FindCheckoutFromPath(
        path, strict=False)
    if main_element_from_path is not None:
      main_element.append(main_element_from_path)

  local_manifest = LocalManifest.FromPath(options.local_manifest_path)

  if options.workon:
    if not main_element:
      parser.error('No project named %r in the default manifest.' % name)
    _AddProjectsToManifestGroups(
        options, [checkout['name'] for checkout in main_element])

  elif main_element:
    if options.remote is not None:
      # Likely this project wasn't meant to be remote, so workon main element
      print("Project already exists in manifest. Using that as workon project.")
      _AddProjectsToManifestGroups(
          options, [checkout['name'] for checkout in main_element])
    else:
      # Conflict will occur; complain.
      parser.error("Requested project name=%r path=%r will conflict with "
                   "your current manifest %s" % (
                       name, path, main_manifest.manifest_path))

  elif local_manifest.GetProject(name, path=path) is not None:
    parser.error("Requested project name=%r path=%r conflicts with "
                 "your local_manifest.xml" % (name, path))

  else:
    element = local_manifest.AddNonWorkonProject(name=name, path=path,
                                                 remote=options.remote,
                                                 revision=revision)
    _AddProjectsToManifestGroups(options, [element.attrib['name']])

    with open(options.local_manifest_path, 'w') as f:
      f.write(local_manifest.ToString())
  return 0
