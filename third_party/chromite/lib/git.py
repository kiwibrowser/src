# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common functions for interacting with git and repo."""

from __future__ import print_function

import collections
import datetime
import errno
import hashlib
import os
import re
import string
from xml import sax

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils


# Retry a git operation if git returns a error response with any of these
# messages. It's all observed 'bad' GoB responses so far.
GIT_TRANSIENT_ERRORS = (
    # crbug.com/285832
    r'! \[remote rejected\].*\(error in hook\)',

    # crbug.com/289932
    r'! \[remote rejected\].*\(failed to lock\)',

    # crbug.com/307156
    r'! \[remote rejected\].*\(error in Gerrit backend\)',

    # crbug.com/285832
    r'remote error: Internal Server Error',

    # crbug.com/294449
    r'fatal: Couldn\'t find remote ref ',

    # crbug.com/220543
    r'git fetch_pack: expected ACK/NAK, got',

    # crbug.com/189455
    r'protocol error: bad pack header',

    # crbug.com/202807
    r'The remote end hung up unexpectedly',

    # crbug.com/298189
    r'TLS packet with unexpected length was received',

    # crbug.com/187444
    r'RPC failed; result=\d+, HTTP code = \d+',

    # crbug.com/315421, b2/18249316
    r'The requested URL returned error: 5',

    # crbug.com/388876
    r'Connection timed out',

    # crbug.com/451458, b/19202011
    r'repository cannot accept new pushes; contact support',

    # crbug.com/535306
    r'Service Temporarily Unavailable',

    # crbug.com/675262
    r'Connection refused',

    # crbug.com/725233
    r'Operation too slow',
)

GIT_TRANSIENT_ERRORS_RE = re.compile('|'.join(GIT_TRANSIENT_ERRORS),
                                     re.IGNORECASE)

DEFAULT_RETRY_INTERVAL = 3
DEFAULT_RETRIES = 10


class GitException(Exception):
  """An exception related to git."""


# remote: git remote name (e.g., 'origin',
#   'https://chromium.googlesource.com/chromiumos/chromite.git', etc.).
# ref: git remote/local ref name (e.g., 'refs/heads/master').
# project_name: git project name (e.g., 'chromiumos/chromite'.)
_RemoteRef = collections.namedtuple(
    '_RemoteRef', ('remote', 'ref', 'project_name'))


class RemoteRef(_RemoteRef):
  """Object representing a remote ref."""

  def __new__(cls, remote, ref, project_name=None):
    return super(RemoteRef, cls).__new__(cls, remote, ref, project_name)

  def __init__(self, remote, ref, project_name=None):
    super(RemoteRef, self).__init__(remote, ref, project_name)


def FindRepoDir(path):
  """Returns the nearest higher-level repo dir from the specified path.

  Args:
    path: The path to use. Defaults to cwd.
  """
  return osutils.FindInPathParents(
      '.repo', path, test_func=os.path.isdir)


def FindRepoCheckoutRoot(path):
  """Get the root of your repo managed checkout."""
  repo_dir = FindRepoDir(path)
  if repo_dir:
    return os.path.dirname(repo_dir)
  else:
    return None


def IsSubmoduleCheckoutRoot(path, remote, url):
  """Tests to see if a directory is the root of a git submodule checkout.

  Args:
    path: The directory to test.
    remote: The remote to compare the |url| with.
    url: The exact URL the |remote| needs to be pointed at.
  """
  if os.path.isdir(path):
    remote_url = cros_build_lib.RunCommand(
        ['git', '--git-dir', path, 'config', 'remote.%s.url' % remote],
        redirect_stdout=True, debug_level=logging.DEBUG,
        error_code_ok=True).output.strip()
    if remote_url == url:
      return True
  return False


def IsGitRepo(cwd):
  """Checks if there's a git repo rooted at a directory."""
  return os.path.isdir(os.path.join(cwd, '.git'))


def IsGitRepositoryCorrupted(cwd):
  """Verify that the specified git repository is not corrupted.

  Args:
    cwd: The git repository to verify.

  Returns:
    True if the repository is corrupted.
  """
  cmd = ['fsck', '--no-progress', '--no-dangling']
  try:
    GarbageCollection(cwd)
    RunGit(cwd, cmd)
    return False
  except cros_build_lib.RunCommandError as ex:
    logging.warning(str(ex))
    return True


_HEX_CHARS = frozenset(string.hexdigits)


def IsSHA1(value, full=True):
  """Returns True if the given value looks like a sha1.

  If full is True, then it must be full length- 40 chars.  If False, >=6, and
  <40.
  """
  if not all(x in _HEX_CHARS for x in value):
    return False
  l = len(value)
  if full:
    return l == 40
  return l >= 6 and l <= 40


def IsRefsTags(value):
  """Return True if the given value looks like a tag.

  Currently this is identified via refs/tags/ prefixing.
  """
  return value.startswith('refs/tags/')


def GetGitRepoRevision(cwd, branch='HEAD', short=False):
  """Find the revision of a branch.

  Args:
    cwd: The git repository to work with.
    branch: Branch name. Defaults to current branch.
    short: If set, output shorter unique SHA-1.

  Returns:
    Revision SHA-1.
  """
  cmd = ['rev-parse', branch]
  if short:
    cmd.insert(1, '--short')
  return RunGit(cwd, cmd).output.strip()


def DoesCommitExistInRepo(cwd, commit):
  """Determine whether a commit (SHA1 or ref) exists in a repo.

  Args:
    cwd: A directory within the project repo.
    commit: The commit to look for. This can be a SHA1 or it can be a ref.

  Returns:
    True if the commit exists in the repo.
  """
  try:
    RunGit(cwd, ['rev-list', '-n1', commit, '--'])
  except cros_build_lib.RunCommandError as e:
    if e.result.returncode == 128:
      return False
    raise
  return True


def GetCurrentBranch(cwd):
  """Returns current branch of a repo, and None if repo is on detached HEAD."""
  try:
    ret = RunGit(cwd, ['symbolic-ref', '-q', 'HEAD'])
    return StripRefsHeads(ret.output.strip(), False)
  except cros_build_lib.RunCommandError as e:
    if e.result.returncode != 1:
      raise
    return None


def StripRefsHeads(ref, strict=True):
  """Remove leading 'refs/heads/' from a ref name.

  If strict is True, an Exception is thrown if the ref doesn't start with
  refs/heads.  If strict is False, the original ref is returned.
  """
  if not ref.startswith('refs/heads/') and strict:
    raise Exception('Ref name %s does not start with refs/heads/' % ref)

  return ref.replace('refs/heads/', '')


def StripRefs(ref):
  """Remove leading 'refs/heads', 'refs/remotes/[^/]+/' from a ref name."""
  ref = StripRefsHeads(ref, False)
  if ref.startswith('refs/remotes/'):
    return ref.split('/', 3)[-1]
  return ref


def NormalizeRef(ref):
  """Convert git branch refs into fully qualified form."""
  if ref and not ref.startswith('refs/'):
    ref = 'refs/heads/%s' % ref
  return ref


def NormalizeRemoteRef(remote, ref):
  """Convert git branch refs into fully qualified remote form."""
  if ref:
    # Support changing local ref to remote ref, or changing the remote
    # for a remote ref.
    ref = StripRefs(ref)

    if not ref.startswith('refs/'):
      ref = 'refs/remotes/%s/%s' % (remote, ref)

  return ref


class ProjectCheckout(dict):
  """Attributes of a given project in the manifest checkout.

  TODO(davidjames): Convert this into an ordinary object instead of a dict.
  """

  def __init__(self, attrs):
    """Constructor.

    Args:
      attrs: The attributes associated with this checkout, as a dictionary.
    """
    dict.__init__(self, attrs)

  def AssertPushable(self):
    """Verify that it is safe to push changes to this repository."""
    if not self['pushable']:
      remote = self['remote']
      raise AssertionError('Remote %s is not pushable.' % (remote,))

  def IsBranchableProject(self):
    """Return whether we can create a branch in the repo for this project."""
    # Backwards compatibility is an issue here. Older manifests used a heuristic
    # based on where the project is hosted. We must continue supporting it.
    # (crbug.com/470690)
    # Prefer explicit tagging.
    if (self[constants.MANIFEST_ATTR_BRANCHING] ==
        constants.MANIFEST_ATTR_BRANCHING_CREATE):
      return True
    if self[constants.MANIFEST_ATTR_BRANCHING] in (
        constants.MANIFEST_ATTR_BRANCHING_PIN,
        constants.MANIFEST_ATTR_BRANCHING_TOT):
      return False

    # Old heuristic.
    site_config = config_lib.GetConfig()
    if (self['remote'] not in site_config.params.CROS_REMOTES or
        self['remote'] not in site_config.params.BRANCHABLE_PROJECTS):
      return False
    return re.match(site_config.params.BRANCHABLE_PROJECTS[self['remote']],
                    self['name'])

  def IsPinnableProject(self):
    """Return whether we should pin to a revision on the CrOS branch."""
    # Backwards compatibility is an issue here. Older manifests used a different
    # tag to spcify pinning behaviour. Support both for now. (crbug.com/470690)
    # Prefer explicit tagging.
    if self[constants.MANIFEST_ATTR_BRANCHING] != '':
      return (self[constants.MANIFEST_ATTR_BRANCHING] ==
              constants.MANIFEST_ATTR_BRANCHING_PIN)

    # Old heuristic.
    return cros_build_lib.BooleanShellValue(self.get('pin'), True)

  def GetPath(self, absolute=False):
    """Get the path to the checkout.

    Args:
      absolute: If True, return an absolute path. If False,
        return a path relative to the repo root.
    """
    return self['local_path'] if absolute else self['path']


class Manifest(object):
  """SAX handler that parses the manifest document.

  Properties:
    checkouts_by_name: A dictionary mapping the names for <project> tags to a
      list of ProjectCheckout objects.
    checkouts_by_path: A dictionary mapping paths for <project> tags to a single
      ProjectCheckout object.
    default: The attributes of the <default> tag.
    includes: A list of XML files that should be pulled in to the manifest.
      These includes are represented as a list of (name, path) tuples.
    manifest_include_dir: If given, this is where to start looking for
      include targets.
    projects: DEPRECATED. A dictionary mapping the names for <project> tags to
      a single ProjectCheckout object. This is now deprecated, since each
      project can map to multiple ProjectCheckout objects.
    remotes: A dictionary mapping <remote> tags to the associated attributes.
    revision: The revision of the manifest repository. If not specified, this
      will be TOT.
  """

  _instance_cache = {}

  def __init__(self, source, manifest_include_dir=None):
    """Initialize this instance.

    Args:
      source: The path to the manifest to parse.  May be a file handle.
      manifest_include_dir: If given, this is where to start looking for
        include targets.
    """
    self.source = source
    self.default = {}
    self._current_project_path = None
    self._current_project_name = None
    self._annotations = {}
    self.checkouts_by_path = {}
    self.checkouts_by_name = {}
    self.remotes = {}
    self.includes = []
    self.revision = None
    self.manifest_include_dir = manifest_include_dir
    self._RunParser(source)
    self.includes = tuple(self.includes)

  def _RequireAttr(self, attr, attrs):
    name = attrs.get('name')
    assert attr in attrs, ('%s is missing a "%s" attribute; attrs: %r' %
                           (name, attr, attrs))

  def _RunParser(self, source, finalize=True):
    parser = sax.make_parser()
    handler = sax.handler.ContentHandler()
    handler.startElement = self._StartElement
    handler.endElement = self._EndElement
    parser.setContentHandler(handler)
    parser.parse(source)
    if finalize:
      self._FinalizeAllProjectData()

  def _StartElement(self, name, attrs):
    """Stores the default manifest properties and per-project overrides."""
    attrs = dict(attrs.items())
    if name == 'default':
      self.default = attrs
    elif name == 'remote':
      self._RequireAttr('name', attrs)
      attrs.setdefault('alias', attrs['name'])
      self.remotes[attrs['name']] = attrs
    elif name == 'project':
      self._RequireAttr('name', attrs)
      self._current_project_path = attrs.get('path', attrs['name'])
      self._current_project_name = attrs['name']
      self.checkouts_by_path[self._current_project_path] = attrs
      checkout = self.checkouts_by_name.setdefault(self._current_project_name,
                                                   [])
      checkout.append(attrs)
      self._annotations = {}
    elif name == 'annotation':
      self._RequireAttr('name', attrs)
      self._RequireAttr('value', attrs)
      self._annotations[attrs['name']] = attrs['value']
    elif name == 'manifest':
      self.revision = attrs.get('revision')
    elif name == 'include':
      if self.manifest_include_dir is None:
        raise OSError(
            errno.ENOENT, 'No manifest_include_dir given, but an include was '
            'encountered; attrs=%r' % (attrs,))
      # Include is calculated relative to the manifest that has the include;
      # thus set the path temporarily to the dirname of the target.
      original_include_dir = self.manifest_include_dir
      include_path = os.path.realpath(
          os.path.join(original_include_dir, attrs['name']))
      self.includes.append((attrs['name'], include_path))
      self._RunParser(include_path, finalize=False)

  def _EndElement(self, name):
    """Store any child element properties into the parent element."""
    if name == 'project':
      assert (self._current_project_name is not None and
              self._current_project_path is not None), (
                  'Malformed xml: Encountered unmatched </project>')
      self.checkouts_by_path[self._current_project_path].update(
          self._annotations)
      for checkout in self.checkouts_by_name[self._current_project_name]:
        checkout.update(self._annotations)
      self._current_project_path = None
      self._current_project_name = None

  def _FinalizeAllProjectData(self):
    """Rewrite projects mixing defaults in and adding our attributes."""
    for path_data in self.checkouts_by_path.itervalues():
      self._FinalizeProjectData(path_data)

  def _FinalizeProjectData(self, attrs):
    """Sets up useful properties for a project.

    Args:
      attrs: The attribute dictionary of a <project> tag.
    """
    for key in ('remote', 'revision'):
      attrs.setdefault(key, self.default.get(key))

    remote = attrs['remote']
    assert remote in self.remotes, ('%s: %s not in %s' %
                                    (self.source, remote, self.remotes))
    remote_name = attrs['remote_alias'] = self.remotes[remote]['alias']

    # 'repo manifest -r' adds an 'upstream' attribute to the project tag for the
    # manifests it generates.  We can use the attribute to get a valid branch
    # instead of a sha1 for these types of manifests.
    upstream = attrs.get('upstream', attrs['revision'])
    if IsSHA1(upstream):
      # The current version of repo we use has a bug: When you create a new
      # repo checkout from a revlocked manifest, the 'upstream' attribute will
      # just point at a SHA1. The default revision will still be correct,
      # however. For now, return the default revision as our best guess as to
      # what the upstream branch for this repository would be. This guess may
      # sometimes be wrong, but it's correct for all of the repositories where
      # we need to push changes (e.g., the overlays).
      # TODO(davidjames): Either fix the repo bug, or update our logic here to
      # check the manifest repository to find the right tracking branch.
      upstream = self.default.get('revision', 'refs/heads/master')

    attrs['tracking_branch'] = 'refs/remotes/%s/%s' % (
        remote_name, StripRefs(upstream),
    )

    site_config = config_lib.GetConfig()
    attrs['pushable'] = remote in site_config.params.GIT_REMOTES
    if attrs['pushable']:
      attrs['push_remote'] = remote
      attrs['push_remote_url'] = site_config.params.GIT_REMOTES[remote]
      attrs['push_url'] = '%s/%s' % (attrs['push_remote_url'], attrs['name'])
    groups = set(attrs.get('groups', 'default').replace(',', ' ').split())
    groups.add('default')
    attrs['groups'] = frozenset(groups)

    # Compute the local ref space.
    # Sanitize a couple path fragments to simplify assumptions in this
    # class, and in consuming code.
    attrs.setdefault('path', attrs['name'])
    for key in ('name', 'path'):
      attrs[key] = os.path.normpath(attrs[key])

    if constants.MANIFEST_ATTR_BRANCHING in attrs:
      assert (attrs[constants.MANIFEST_ATTR_BRANCHING] in
              constants.MANIFEST_ATTR_BRANCHING_ALL)
    else:
      attrs[constants.MANIFEST_ATTR_BRANCHING] = ''

  @staticmethod
  def _GetManifestHash(source, ignore_missing=False):
    if isinstance(source, basestring):
      try:
        # TODO(build): convert this to osutils.ReadFile once these
        # classes are moved out into their own module (if possible;
        # may still be cyclic).
        with open(source, 'rb') as f:
          return hashlib.md5(f.read()).hexdigest()
      except EnvironmentError as e:
        if e.errno != errno.ENOENT or not ignore_missing:
          raise
    source.seek(0)
    md5 = hashlib.md5(source.read()).hexdigest()
    source.seek(0)
    return md5

  @classmethod
  def Cached(cls, source, manifest_include_dir=None):
    """Return an instance, reusing an existing one if possible.

    May be a seekable filehandle, or a filepath.
    See __init__ for an explanation of these arguments.
    """

    md5 = cls._GetManifestHash(source)
    obj, sources = cls._instance_cache.get(md5, (None, ()))
    if manifest_include_dir is None and sources:
      # We're being invoked in a different way than the orignal
      # caching; disregard the cached entry.
      # Most likely, the instantiation will explode; let it fly.
      obj, sources = None, ()
    for include_target, target_md5 in sources:
      if cls._GetManifestHash(include_target, True) != target_md5:
        obj = None
        break
    if obj is None:
      obj = cls(source, manifest_include_dir=manifest_include_dir)
      sources = tuple((abspath, cls._GetManifestHash(abspath))
                      for (target, abspath) in obj.includes)
      cls._instance_cache[md5] = (obj, sources)

    return obj


class ManifestCheckout(Manifest):
  """A Manifest Handler for a specific manifest checkout."""

  _instance_cache = {}

  def __init__(self, path, manifest_path=None, search=True):
    """Initialize this instance.

    Args:
      path: Path into a manifest checkout (doesn't have to be the root).
      manifest_path: If supplied, the manifest to use.  Else the manifest
        in the root of the checkout is used.  May be a seekable file handle.
      search: If True, the path can point into the repo, and the root will
        be found automatically.  If False, the path *must* be the root, else
        an OSError ENOENT will be thrown.

    Raises:
      OSError: if a failure occurs.
    """
    self.root, manifest_path = self._NormalizeArgs(
        path, manifest_path, search=search)

    self.manifest_path = os.path.realpath(manifest_path)
    manifest_include_dir = os.path.dirname(self.manifest_path)
    self.manifest_branch = self._GetManifestsBranch(self.root)
    self._content_merging = {}
    Manifest.__init__(self, self.manifest_path,
                      manifest_include_dir=manifest_include_dir)

  @staticmethod
  def _NormalizeArgs(path, manifest_path=None, search=True):
    root = FindRepoCheckoutRoot(path)
    if root is None:
      raise OSError(errno.ENOENT, "Couldn't find repo root: %s" % (path,))
    root = os.path.normpath(os.path.realpath(root))
    if not search:
      if os.path.normpath(os.path.realpath(path)) != root:
        raise OSError(errno.ENOENT, 'Path %s is not a repo root, and search '
                      'is disabled.' % path)
    if manifest_path is None:
      manifest_path = os.path.join(root, '.repo', 'manifest.xml')
    return root, manifest_path

  @staticmethod
  def IsFullManifest(checkout_root):
    """Returns True iff the given checkout is using a full manifest.

    This method should go away as part of the cleanup related to brbug.com/854.

    Args:
      checkout_root: path to the root of an SDK checkout.

    Returns:
      True iff the manifest selected for the given SDK is a full manifest.
      In this context we'll accept any manifest for which there are no groups
      defined.
    """
    manifests_git_repo = os.path.join(checkout_root, '.repo', 'manifests.git')
    cmd = ['config', '--local', '--get', 'manifest.groups']
    result = RunGit(manifests_git_repo, cmd, error_code_ok=True)

    if result.output.strip():
      # Full layouts don't define groups.
      return False

    return True

  def FindCheckouts(self, project, branch=None):
    """Returns the list of checkouts for a given |project|/|branch|.

    Args:
      project: Project name to search for.
      branch: Branch to use.

    Returns:
      A list of ProjectCheckout objects.
    """
    checkouts = []
    for checkout in self.checkouts_by_name.get(project, []):
      tracking_branch = checkout['tracking_branch']
      if branch is None or StripRefs(branch) == StripRefs(tracking_branch):
        checkouts.append(checkout)
    return checkouts

  def FindCheckout(self, project, branch=None, strict=True):
    """Returns the checkout associated with a given project/branch.

    Args:
      project: The project to look for.
      branch: The branch that the project is tracking.
      strict: Raise AssertionError if a checkout cannot be found.

    Returns:
      A ProjectCheckout object.

    Raises:
      AssertionError if there is more than one checkout associated with the
      given project/branch combination.
    """
    checkouts = self.FindCheckouts(project, branch)
    if len(checkouts) < 1:
      if strict:
        raise AssertionError('Could not find checkout of %s' % (project,))
      return None
    elif len(checkouts) > 1:
      raise AssertionError('Too many checkouts found for %s' % project)
    return checkouts[0]

  def ListCheckouts(self):
    """List the checkouts in the manifest.

    Returns:
      A list of ProjectCheckout objects.
    """
    return self.checkouts_by_path.values()

  def FindCheckoutFromPath(self, path, strict=True):
    """Find the associated checkouts for a given |path|.

    The |path| can either be to the root of a project, or within the
    project itself (chromite.cbuildbot for example).  It may be relative
    to the repo root, or an absolute path.  If |path| is not within a
    checkout, return None.

    Args:
      path: Path to examine.
      strict: If True, fail when no checkout is found.

    Returns:
      None if no checkout is found, else the checkout.
    """
    # Realpath everything sans the target to keep people happy about
    # how symlinks are handled; exempt the final node since following
    # through that is unlikely even remotely desired.
    tmp = os.path.join(self.root, os.path.dirname(path))
    path = os.path.join(os.path.realpath(tmp), os.path.basename(path))
    path = os.path.normpath(path) + '/'
    candidates = []
    for checkout in self.ListCheckouts():
      if path.startswith(checkout['local_path'] + '/'):
        candidates.append((checkout['path'], checkout))

    if not candidates:
      if strict:
        raise AssertionError('Could not find repo project at %s' % (path,))
      return None

    # The checkout with the greatest common path prefix is the owner of
    # the given pathway. Return that.
    return max(candidates)[1]

  def _FinalizeAllProjectData(self):
    """Rewrite projects mixing defaults in and adding our attributes."""
    Manifest._FinalizeAllProjectData(self)
    for key, value in self.checkouts_by_path.iteritems():
      self.checkouts_by_path[key] = ProjectCheckout(value)
    for key, value in self.checkouts_by_name.iteritems():
      self.checkouts_by_name[key] = \
          [ProjectCheckout(x) for x in value]

  def _FinalizeProjectData(self, attrs):
    Manifest._FinalizeProjectData(self, attrs)
    attrs['local_path'] = os.path.join(self.root, attrs['path'])

  @staticmethod
  def _GetManifestsBranch(root):
    """Get the tracking branch of the manifest repository.

    Returns:
      The branch name.
    """
    # Suppress the normal "if it ain't refs/heads, we don't want none o' that"
    # check for the merge target; repo writes the ambigious form of the branch
    # target for `repo init -u url -b some-branch` usages (aka, 'master'
    # instead of 'refs/heads/master').
    path = os.path.join(root, '.repo', 'manifests')
    current_branch = GetCurrentBranch(path)
    if current_branch != 'default':
      raise OSError(errno.ENOENT,
                    'Manifest repository at %s is checked out to %s.  '
                    "It should be checked out to 'default'."
                    % (root, 'detached HEAD' if current_branch is None
                       else current_branch))

    result = GetTrackingBranchViaGitConfig(
        path, 'default', allow_broken_merge_settings=True, for_checkout=False)

    if result is not None:
      return StripRefsHeads(result.ref, False)

    raise OSError(errno.ENOENT,
                  "Manifest repository at %s is checked out to 'default', but "
                  'the git tracking configuration for that branch is broken; '
                  'failing due to that.' % (root,))

  # pylint: disable=arguments-differ
  @classmethod
  def Cached(cls, path, manifest_path=None, search=True):
    """Return an instance, reusing an existing one if possible.

    Args:
      path: The pathway into a checkout; the root will be found automatically.
      manifest_path: if given, the manifest.xml to use instead of the
        checkouts internal manifest.  Use with care.
      search: If True, the path can point into the repo, and the root will
        be found automatically.  If False, the path *must* be the root, else
        an OSError ENOENT will be thrown.
    """
    root, manifest_path = cls._NormalizeArgs(path, manifest_path,
                                             search=search)

    md5 = cls._GetManifestHash(manifest_path)
    obj, sources = cls._instance_cache.get((root, md5), (None, ()))
    for include_target, target_md5 in sources:
      if cls._GetManifestHash(include_target, True) != target_md5:
        obj = None
        break
    if obj is None:
      obj = cls(root, manifest_path=manifest_path)
      sources = tuple((abspath, cls._GetManifestHash(abspath))
                      for (target, abspath) in obj.includes)
      cls._instance_cache[(root, md5)] = (obj, sources)
    return obj


def RunGit(git_repo, cmd, **kwargs):
  """RunCommand wrapper for git commands.

  This suppresses print_cmd, and suppresses output by default.  Git
  functionality w/in this module should use this unless otherwise
  warranted, to standardize git output (primarily, keeping it quiet
  and being able to throw useful errors for it).

  Args:
    git_repo: Pathway to the git repo to operate on.
    cmd: A sequence of the git subcommand to run.  The 'git' prefix is
      added automatically.  If you wished to run 'git remote update',
      this would be ['remote', 'update'] for example.
    kwargs: Any RunCommand or GenericRetry options/overrides to use.

  Returns:
    A CommandResult object.
  """

  kwargs.setdefault('print_cmd', False)
  kwargs.setdefault('cwd', git_repo)
  kwargs.setdefault('capture_output', True)
  return cros_build_lib.RunCommand(['git'] + cmd, **kwargs)


def Init(git_repo):
  """Create a new git repository, in the given location.

  Args:
    git_repo: Path for where to create a git repo. Directory will be created if
              it doesnt exist.
  """
  osutils.SafeMakedirs(git_repo)
  RunGit(git_repo, ['init'])


def Clone(git_repo, git_url, branch=None):
  """Clone a git repository, into the given directory.

  Args:
    git_repo: Path for where to create a git repo. Directory will be created if
              it doesnt exist.
    git_url: Url to clone the git repository from.
    branch: Branch to checkout ('stabilize.5978.51.B'), or None.
  """
  osutils.SafeMakedirs(git_repo)

  cmd = ['clone', git_url, git_repo]
  if branch:
    cmd += ['-b', branch]
  RunGit(git_repo, cmd)


def ShallowFetch(git_repo, git_url, sparse_checkout=None):
  """Fetch a shallow git repository.

  Args:
    git_repo: Path of the git repo.
    git_url: Url to fetch the git repository from.
    sparse_checkout: List of file paths to fetch.
  """
  Init(git_repo)
  RunGit(git_repo, ['remote', 'add', 'origin', git_url])
  if sparse_checkout is not None:
    assert isinstance(sparse_checkout, list)
    RunGit(git_repo, ['config', 'core.sparsecheckout', 'true'])
    osutils.WriteFile(os.path.join(git_repo, '.git/info/sparse-checkout'),
                      '\n'.join(sparse_checkout))
    logging.info('Sparse checkout: %s', sparse_checkout)

  utcnow = datetime.datetime.utcnow
  start = utcnow()
  # Only fetch TOT git metadata without revision history.
  RunGit(git_repo, ['fetch', '--depth=1'],
         print_cmd=True, redirect_stderr=True, capture_output=False)
  # Pull the files in sparse_checkout.
  RunGit(git_repo, ['pull', 'origin', 'master'],
         print_cmd=True, redirect_stderr=True, capture_output=False)
  logging.info('ShallowFetch completed in %s.', utcnow() - start)


def GetProjectUserEmail(git_repo):
  """Get the email configured for the project."""
  output = RunGit(git_repo, ['var', 'GIT_COMMITTER_IDENT']).output
  m = re.search(r'<([^>]*)>', output.strip())
  return m.group(1) if m else None


def MatchBranchName(git_repo, pattern, namespace=''):
  """Return branches who match the specified regular expression.

  Args:
    git_repo: The git repository to operate upon.
    pattern: The regexp to search with.
    namespace: The namespace to restrict search to (e.g. 'refs/heads/').

  Returns:
    List of matching branch names (with |namespace| trimmed).
  """
  match = re.compile(pattern, flags=re.I)
  output = RunGit(git_repo, ['ls-remote', git_repo, namespace + '*']).output
  branches = [x.split()[1] for x in output.splitlines()]
  branches = [x[len(namespace):] for x in branches if x.startswith(namespace)]
  return [x for x in branches if match.search(x)]


class AmbiguousBranchName(Exception):
  """Error if given branch name matches too many branches."""


def MatchSingleBranchName(*args, **kwargs):
  """Match exactly one branch name, else throw an exception.

  Args:
    See MatchBranchName for more details; all args are passed on.

  Returns:
    The branch name.

  Raises:
    raise AmbiguousBranchName if we did not match exactly one branch.
  """
  ret = MatchBranchName(*args, **kwargs)
  if len(ret) != 1:
    raise AmbiguousBranchName('Did not match exactly 1 branch: %r' % ret)
  return ret[0]


def GetTrackingBranchViaGitConfig(git_repo, branch, for_checkout=True,
                                  allow_broken_merge_settings=False,
                                  recurse=10):
  """Pull the remote and upstream branch of a local branch

  Args:
    git_repo: The git repository to operate upon.
    branch: The branch to inspect.
    for_checkout: Whether to return localized refspecs, or the remote's
      view of it.
    allow_broken_merge_settings: Repo in a couple of spots writes invalid
      branch.mybranch.merge settings; if these are encountered, they're
      normally treated as an error and this function returns None.  If
      this option is set to True, it suppresses this check.
    recurse: If given and the target is local, then recurse through any
      remote=. (aka locals).  This is enabled by default, and is what allows
      developers to have multiple local branches of development dependent
      on one another; disabling this makes that work flow impossible,
      thus disable it only with good reason.  The value given controls how
      deeply to recurse.  Defaults to tracing through 10 levels of local
      remotes. Disabling it is a matter of passing 0.

  Returns:
    A RemoteRef, or None.  If for_checkout, then it returns the localized
    version of it.
  """
  try:
    cmd = ['config', '--get-regexp',
           r'branch\.%s\.(remote|merge)' % re.escape(branch)]
    data = RunGit(git_repo, cmd).output.splitlines()

    prefix = 'branch.%s.' % (branch,)
    data = [x.split() for x in data]
    vals = dict((x[0][len(prefix):], x[1]) for x in data)
    if len(vals) != 2:
      if not allow_broken_merge_settings:
        return None
      elif 'merge' not in vals:
        # There isn't anything we can do here.
        return None
      elif 'remote' not in vals:
        # Repo v1.9.4 and up occasionally invalidly leave the remote out.
        # Only occurs for the manifest repo fortunately.
        vals['remote'] = 'origin'
    remote, rev = vals['remote'], vals['merge']
    # Suppress non branches; repo likes to write revisions and tags here,
    # which is wrong (git hates it, nor will it honor it).
    if rev.startswith('refs/remotes/'):
      if for_checkout:
        return RemoteRef(remote, rev)
      # We can't backtrack from here, or at least don't want to.
      # This is likely refs/remotes/m/ which repo writes when dealing
      # with a revision locked manifest.
      return None
    if not rev.startswith('refs/heads/'):
      # We explicitly don't allow pushing to tags, nor can one push
      # to a sha1 remotely (makes no sense).
      if not allow_broken_merge_settings:
        return None
    elif remote == '.':
      if recurse == 0:
        raise Exception(
            'While tracing out tracking branches, we recursed too deeply: '
            'bailing at %s' % branch)
      return GetTrackingBranchViaGitConfig(
          git_repo, StripRefsHeads(rev), for_checkout=for_checkout,
          allow_broken_merge_settings=allow_broken_merge_settings,
          recurse=recurse - 1)
    elif for_checkout:
      rev = 'refs/remotes/%s/%s' % (remote, StripRefsHeads(rev))
    return RemoteRef(remote, rev)
  except cros_build_lib.RunCommandError as e:
    # 1 is the retcode for no matches.
    if e.result.returncode != 1:
      raise
  return None


def GetTrackingBranchViaManifest(git_repo, for_checkout=True, for_push=False,
                                 manifest=None):
  """Gets the appropriate push branch via the manifest if possible.

  Args:
    git_repo: The git repo to operate upon.
    for_checkout: Whether to return localized refspecs, or the remote's
      view of it.  Note that depending on the remote, the remote may differ
      if for_push is True or set to False.
    for_push: Controls whether the remote and refspec returned is explicitly
      for pushing.
    manifest: A Manifest instance if one is available, else a
      ManifestCheckout is created and used.

  Returns:
    A RemoteRef, or None.  If for_checkout, then it returns the localized
    version of it.
  """
  try:
    if manifest is None:
      manifest = ManifestCheckout.Cached(git_repo)

    checkout = manifest.FindCheckoutFromPath(git_repo, strict=False)

    if checkout is None:
      return None

    if for_push:
      checkout.AssertPushable()

    if for_push:
      remote = checkout['push_remote']
    else:
      remote = checkout['remote']

    if for_checkout:
      revision = checkout['tracking_branch']
    else:
      revision = checkout['revision']
      if not revision.startswith('refs/heads/'):
        return None

    project_name = checkout.get('name', None)

    return RemoteRef(remote, revision, project_name=project_name)
  except EnvironmentError as e:
    if e.errno != errno.ENOENT:
      raise
  return None


def GetTrackingBranch(git_repo, branch=None, for_checkout=True, fallback=True,
                      manifest=None, for_push=False):
  """Gets the appropriate push branch for the specified directory.

  This function works on both repo projects and regular git checkouts.

  Assumptions:
   1. We assume the manifest defined upstream is desirable.
   2. No manifest?  Assume tracking if configured is accurate.
   3. If none of the above apply, you get 'origin', 'master' or None,
      depending on fallback.

  Args:
    git_repo: Git repository to operate upon.
    branch: Find the tracking branch for this branch.  Defaults to the
      current branch for |git_repo|.
    for_checkout: Whether to return localized refspecs, or the remotes
      view of it.
    fallback: If true and no remote/branch could be discerned, return
      'origin', 'master'.  If False, you get None.
      Note that depending on the remote, the remote may differ
      if for_push is True or set to False.
    for_push: Controls whether the remote and refspec returned is explicitly
      for pushing.
    manifest: A Manifest instance if one is available, else a
      ManifestCheckout is created and used.

  Returns:
    A RemoteRef, or None.
  """
  result = GetTrackingBranchViaManifest(git_repo, for_checkout=for_checkout,
                                        manifest=manifest, for_push=for_push)
  if result is not None:
    return result

  if branch is None:
    branch = GetCurrentBranch(git_repo)
  if branch:
    result = GetTrackingBranchViaGitConfig(git_repo, branch,
                                           for_checkout=for_checkout)
    if result is not None:
      if (result.ref.startswith('refs/heads/') or
          result.ref.startswith('refs/remotes/')):
        return result

  if not fallback:
    return None
  if for_checkout:
    return RemoteRef('origin', 'refs/remotes/origin/master')
  return RemoteRef('origin', 'master')


def CreateBranch(git_repo, branch, branch_point='HEAD', track=False):
  """Create a branch.

  Args:
    git_repo: Git repository to act on.
    branch: Name of the branch to create.
    branch_point: The ref to branch from.  Defaults to 'HEAD'.
    track: Whether to setup the branch to track its starting ref.
  """
  cmd = ['checkout', '-B', branch, branch_point]
  if track:
    cmd.append('--track')
  RunGit(git_repo, cmd)


def AddPath(path):
  """Use 'git add' on a path.

  Args:
    path: Path to the git repository and the path to add.
  """
  dirname, filename = os.path.split(path)
  RunGit(dirname, ['add', '--', filename])


def RmPath(path):
  """Use 'git rm' on a file.

  Args:
    path: Path to the git repository and the path to rm.
  """
  dirname, filename = os.path.split(path)
  RunGit(dirname, ['rm', '--', filename])


def GetObjectAtRev(git_repo, obj, rev):
  """Return the contents of a git object at a particular revision.

  This could be used to look at an old version of a file or directory, for
  instance, without modifying the working directory.

  Args:
    git_repo: Path to a directory in the git repository to query.
    obj: The name of the object to read.
    rev: The revision to retrieve.

  Returns:
    The content of the object.
  """
  rev_obj = '%s:%s' % (rev, obj)
  return RunGit(git_repo, ['show', rev_obj]).output


def RevertPath(git_repo, filename, rev):
  """Revert a single file back to a particular revision and 'add' it with git.

  Args:
    git_repo: Path to the directory holding the file.
    filename: Name of the file to revert.
    rev: Revision to revert the file to.
  """
  RunGit(git_repo, ['checkout', rev, '--', filename])


def Commit(git_repo, message, amend=False, allow_empty=False,
           reset_author=False):
  """Commit with git.

  Args:
    git_repo: Path to the git repository to commit in.
    message: Commit message to use.
    amend: Whether to 'amend' the CL, default False
    allow_empty: Whether to allow an empty commit. Default False.
    reset_author: Whether to reset author according to current config.

  Returns:
    The Gerrit Change-ID assigned to the CL if it exists.
  """
  cmd = ['commit', '-m', message]
  if amend:
    cmd.append('--amend')
  if allow_empty:
    cmd.append('--allow-empty')
  if reset_author:
    cmd.append('--reset-author')
  RunGit(git_repo, cmd)

  log = RunGit(git_repo, ['log', '-n', '1', '--format=format:%B']).output
  match = re.search('Change-Id: (?P<ID>I[a-fA-F0-9]*)', log)
  return match.group('ID') if match else None


_raw_diff_components = ('src_mode', 'dst_mode', 'src_sha', 'dst_sha',
                        'status', 'score', 'src_file', 'dst_file')
# RawDiffEntry represents a line of raw formatted git diff output.
RawDiffEntry = collections.namedtuple('RawDiffEntry', _raw_diff_components)


# This regular expression pulls apart a line of raw formatted git diff output.
DIFF_RE = re.compile(
    r':(?P<src_mode>[0-7]*) (?P<dst_mode>[0-7]*) '
    r'(?P<src_sha>[0-9a-f]*)(\.)* (?P<dst_sha>[0-9a-f]*)(\.)* '
    r'(?P<status>[ACDMRTUX])(?P<score>[0-9]+)?\t'
    r'(?P<src_file>[^\t]+)\t?(?P<dst_file>[^\t]+)?')


def RawDiff(path, target):
  """Return the parsed raw format diff of target

  Args:
    path: Path to the git repository to diff in.
    target: The target to diff.

  Returns:
    A list of RawDiffEntry's.
  """
  entries = []

  cmd = ['diff', '-M', '--raw', target]
  diff = RunGit(path, cmd).output
  diff_lines = diff.strip().split('\n')
  for line in diff_lines:
    match = DIFF_RE.match(line)
    if not match:
      raise GitException('Failed to parse diff output: %s' % line)
    entries.append(RawDiffEntry(*match.group(*_raw_diff_components)))

  return entries


def UploadCL(git_repo, remote, branch, local_branch='HEAD', draft=False,
             reviewers=None, **kwargs):
  """Upload a CL to gerrit. The CL should be checked out currently.

  Args:
    git_repo: Path to the git repository with the CL to upload checked out.
    remote: The remote to upload the CL to.
    branch: Branch to upload to.
    local_branch: Branch to upload.
    draft: Whether to upload as a draft.
    reviewers: Add the reviewers to the CL.
    kwargs: Extra options for GitPush. capture_output defaults to False so
      that the URL for new or updated CLs is shown to the user.
  """
  ref = ('refs/drafts/%s' if draft else 'refs/for/%s') % branch
  if reviewers:
    reviewer_list = ['r=%s' % i for i in reviewers]
    ref = ref + '%'+ ','.join(reviewer_list)
  remote_ref = RemoteRef(remote, ref)
  kwargs.setdefault('capture_output', False)
  kwargs.setdefault('combine_stdout_stderr', True)
  return GitPush(git_repo, local_branch, remote_ref, **kwargs)


def GitPush(git_repo, refspec, push_to, force=False,
            capture_output=True, skip=False, **kwargs):
  """Wrapper for pushing to a branch.

  Args:
    git_repo: Git repository to act on.
    refspec: The local ref to push to the remote.
    push_to: A RemoteRef object representing the remote ref to push to.
    force: Whether to bypass non-fastforward checks.
    capture_output: Whether to capture output for this command.
    skip: Do not actually push anything.
  """
  cmd = ['push', push_to.remote, '%s:%s' % (refspec, push_to.ref)]
  if force:
    cmd.append('--force')

  if skip:
    # git-push has a --dry-run option but we can't use it because that still
    # runs push-access checks, and we want the skip mode to be available to
    # users who can't really push to remote.
    logging.info('Would have run "%s"', cmd)
    return

  return RunGit(git_repo, cmd, capture_output=capture_output,
                **kwargs)


# TODO(build): Switch callers of this function to use CreateBranch instead.
def CreatePushBranch(branch, git_repo, sync=True, remote_push_branch=None):
  """Create a local branch for pushing changes inside a repo repository.

  Args:
    branch: Local branch to create.
    git_repo: Git repository to create the branch in.
    sync: Update remote before creating push branch.
    remote_push_branch: A RemoteRef to push to. i.e.,
                        RemoteRef('cros', 'master').  By default it tries to
                        automatically determine which tracking branch to use
                        (see GetTrackingBranch()).
  """
  if not remote_push_branch:
    remote_push_branch = GetTrackingBranch(git_repo, for_push=True)

  if sync:
    cmd = ['remote', 'update', remote_push_branch.remote]
    RunGit(git_repo, cmd)

  RunGit(git_repo, ['checkout', '-B', branch, '-t', remote_push_branch.ref])


def SyncPushBranch(git_repo, remote, target, use_merge=False, **kwargs):
  """Sync and rebase or merge a local push branch to the latest remote version.

  Args:
    git_repo: Git repository to rebase in.
    remote: The remote returned by GetTrackingBranch(for_push=True)
    target: The branch name returned by GetTrackingBranch().  Must
      start with refs/remotes/ (specifically must be a proper remote
      target rather than an ambiguous name).
    use_merge: Default: False. If True, use merge to bring local branch up to
      date with remote branch. Otherwise, use rebase.
    kwargs: Arguments passed through to RunGit.
  """
  subcommand = 'merge' if use_merge else 'rebase'

  if not target.startswith('refs/remotes/'):
    raise Exception(
        'Was asked to %s to a non branch target w/in the push pathways.  '
        'This is highly indicative of an internal bug.  remote %s, %s %s'
        % (subcommand, remote, subcommand, target))

  cmd = ['remote', 'update', remote]
  RunGit(git_repo, cmd, **kwargs)

  try:
    RunGit(git_repo, [subcommand, target], **kwargs)
  except cros_build_lib.RunCommandError:
    # Looks like our change conflicts with upstream. Cleanup our failed
    # rebase.
    RunGit(git_repo, [subcommand, '--abort'], error_code_ok=True, **kwargs)
    raise


def PushBranch(branch, git_repo, dryrun=False, staging_branch=None):
  """General method to push local git changes.

  This method only works with branches created via the CreatePushBranch
  function.

  Args:
    branch: Local branch to push.  Branch should have already been created
      with a local change committed ready to push to the remote branch.  Must
      also already be checked out to that branch.
    git_repo: Git repository to push from.
    dryrun: Git push --dry-run if set to True.
    staging_branch: Push change commits to the staging_branch if it's not None

  Raises:
    GitPushFailed if push was unsuccessful after retries
  """
  remote_ref = GetTrackingBranch(git_repo, branch, for_checkout=False,
                                 for_push=True)
  # Don't like invoking this twice, but there is a bit of API
  # impedence here; cros_mark_as_stable
  local_ref = GetTrackingBranch(git_repo, branch, for_push=True)

  if not remote_ref.ref.startswith('refs/heads/'):
    raise Exception('Was asked to push to a non branch namespace: %s' %
                    remote_ref.ref)

  #reference = staging_branch if staging_branch is not None else remote_ref.ref
  if staging_branch is not None:
    remote_ref = remote_ref._replace(ref=staging_branch)

  logging.debug('Trying to push %s to %s:%s',
                git_repo, branch, remote_ref.ref)

  if dryrun:
    dryrun = True

  SyncPushBranch(git_repo, remote_ref.remote, local_ref.ref)

  try:
    GitPush(git_repo, branch, remote_ref, skip=dryrun)
  except cros_build_lib.RunCommandError:
    raise

  logging.info('Successfully pushed %s to %s %s:%s',
               git_repo, remote_ref.remote, branch, remote_ref.ref)


def CleanAndDetachHead(git_repo):
  """Remove all local changes and checkout a detached head.

  Args:
    git_repo: Directory of git repository.
  """
  RunGit(git_repo, ['am', '--abort'], error_code_ok=True)
  RunGit(git_repo, ['rebase', '--abort'], error_code_ok=True)
  RunGit(git_repo, ['clean', '-dfx'])
  RunGit(git_repo, ['checkout', '--detach', '-f', 'HEAD'])


def CleanAndCheckoutUpstream(git_repo, refresh_upstream=True):
  """Remove all local changes and checkout the latest origin.

  All local changes in the supplied repo will be removed. The branch will
  also be switched to a detached head pointing at the latest origin.

  Args:
    git_repo: Directory of git repository.
    refresh_upstream: If True, run a remote update prior to checking it out.
  """
  remote_ref = GetTrackingBranch(git_repo, for_push=refresh_upstream)
  CleanAndDetachHead(git_repo)
  if refresh_upstream:
    RunGit(git_repo, ['remote', 'update', remote_ref.remote])
  RunGit(git_repo, ['checkout', remote_ref.ref])


def GetChromiteTrackingBranch():
  """Returns the remote branch associated with chromite."""
  cwd = os.path.dirname(os.path.realpath(__file__))
  result_ref = GetTrackingBranch(cwd, for_checkout=False, fallback=False)
  if result_ref:
    branch = result_ref.ref
    if branch.startswith('refs/heads/'):
      # Normal scenario.
      return StripRefsHeads(branch)
    # Reaching here means it was refs/remotes/m/blah, or just plain invalid,
    # or that we're on a detached head in a repo not managed by chromite.

  # Manually try the manifest next.
  try:
    manifest = ManifestCheckout.Cached(cwd)
    # Ensure the manifest knows of this checkout.
    if manifest.FindCheckoutFromPath(cwd, strict=False):
      return manifest.manifest_branch
  except EnvironmentError as e:
    if e.errno != errno.ENOENT:
      raise

  # Not a manifest checkout.
  logging.notice(
      "Chromite checkout at %s isn't controlled by repo, nor is it on a "
      'branch (or if it is, the tracking configuration is missing or broken).  '
      'Falling back to assuming the chromite checkout is derived from '
      "'master'; this *may* result in breakage." % cwd)
  return 'master'


def GarbageCollection(git_repo, prune_all=False):
  """Cleanup unnecessary files and optimize the local repository.

  Args:
    git_repo: Directory of git repository.
    prune_all: If True, prune all loose objects regardless of gc.pruneExpire.
  """
  # Use --auto so it only runs if housekeeping is necessary.
  cmd = ['gc', '--auto']
  if prune_all:
    cmd.append('--prune=all')
  RunGit(git_repo, cmd)
