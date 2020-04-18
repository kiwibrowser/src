# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module uprevs Chrome for cbuildbot.

After calling, it prints outs CHROME_VERSION_ATOM=(version atom string).  A
caller could then use this atom with emerge to build the newly uprevved version
of Chrome e.g.

./cros_mark_chrome_as_stable tot
Returns chrome-base/chromeos-chrome-8.0.552.0_alpha_r1

emerge-x86-generic =chrome-base/chromeos-chrome-8.0.552.0_alpha_r1
"""

from __future__ import print_function

import base64
import distutils.version
import filecmp
import os
import re
import urlparse

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import gob_util
from chromite.lib import portage_util
from chromite.lib import timeout_util
from chromite.scripts import cros_mark_as_stable


# Helper regex's for finding ebuilds.
_CHROME_VERSION_REGEX = r'\d+\.\d+\.\d+\.\d+'
_NON_STICKY_REGEX = r'%s[(_rc.*)|(_alpha.*)]+' % _CHROME_VERSION_REGEX

# Dir where all the action happens.
_OVERLAY_DIR = '%(srcroot)s/third_party/chromiumos-overlay/'

_GIT_COMMIT_MESSAGE = ('Marking %(chrome_rev)s for %(chrome_pn)s ebuild '
                       'with version %(chrome_version)s as stable.')

# URLs that print lists of chrome revisions between two versions of the browser.
_CHROME_VERSION_URL = ('http://omahaproxy.appspot.com/changelog?'
                       'old_version=%(old)s&new_version=%(new)s')

# Only print links when we rev these types.
_REV_TYPES_FOR_LINKS = [constants.CHROME_REV_LATEST,
                        constants.CHROME_REV_STICKY]

# TODO(szager): This is inaccurate, but is it safe to change?  I have no idea.
_CHROME_SVN_TAG = 'CROS_SVN_COMMIT'


def _GetVersionContents(chrome_version_info):
  """Returns the current Chromium version, from the contents of a VERSION file.

  Args:
    chrome_version_info: The contents of a chromium VERSION file.
  """
  chrome_version_array = []
  for line in chrome_version_info.splitlines():
    chrome_version_array.append(line.rpartition('=')[2])

  return '.'.join(chrome_version_array)


def _GetSpecificVersionUrl(git_url, revision, time_to_wait=600):
  """Returns the Chromium version, from a repository URL and version.

  Args:
    git_url: Repository URL for chromium.
    revision: the git revision we want to use.
    time_to_wait: the minimum period before abandoning our wait for the
      desired revision to be present.
  """
  parsed_url = urlparse.urlparse(git_url)
  host = parsed_url[1]
  path = parsed_url[2].rstrip('/') + (
      '/+/%s/chrome/VERSION?format=text' % revision)

  # Allow for git repository replication lag with sleep/retry loop.
  def _fetch():
    fh = gob_util.FetchUrl(host, path, ignore_404=True)
    return fh.read() if fh else None

  def _wait_msg(_remaining):
    logging.info('Repository does not yet have revision %s.  Sleeping...',
                 revision)

  content = timeout_util.WaitForSuccess(
      retry_check=lambda x: not bool(x),
      func=_fetch,
      timeout=time_to_wait,
      period=30,
      side_effect_func=_wait_msg)
  return _GetVersionContents(base64.b64decode(content))


def _GetTipOfTrunkVersionFile(root):
  """Returns the current Chromium version, from a file in a checkout.

  Args:
    root: path to the root of the chromium checkout.
  """
  version_file = os.path.join(root, 'src', 'chrome', 'VERSION')
  chrome_version_info = cros_build_lib.RunCommand(
      ['cat', version_file],
      redirect_stdout=True,
      error_message='Could not read version file at %s.' % version_file).output

  return _GetVersionContents(chrome_version_info)


def CheckIfChromeRightForOS(deps_content):
  """Checks if DEPS is right for Chrome OS.

  This function checks for a variable called 'buildspec_platforms' to
  find out if its 'chromeos' or 'all'. If any of those values,
  then it chooses that DEPS.

  Args:
    deps_content: Content of release buildspec DEPS file.

  Returns:
    True if DEPS is the right Chrome for Chrome OS.
  """
  platforms_search = re.search(r'buildspec_platforms.*\s.*\s', deps_content)

  if platforms_search:
    platforms = platforms_search.group()
    if 'chromeos' in platforms or 'all' in platforms:
      return True

  return False


def GetLatestRelease(git_url, branch=None):
  """Gets the latest release version from the release tags in the repository.

  Args:
    git_url: URL of git repository.
    branch: If set, gets the latest release for branch, otherwise latest
      release.

  Returns:
    Latest version string.
  """
  # TODO(szager): This only works for public release buildspecs in the chromium
  # src repository.  Internal buildspecs are tracked differently.  At the time
  # of writing, I can't find any callers that use this method to scan for
  # internal buildspecs.  But there may be something lurking...

  parsed_url = urlparse.urlparse(git_url)
  path = parsed_url[2].rstrip('/') + '/+refs/tags?format=JSON'
  j = gob_util.FetchUrlJson(parsed_url[1], path, ignore_404=False)
  if branch:
    chrome_version_re = re.compile(r'^%s\.\d+.*' % branch)
  else:
    chrome_version_re = re.compile(r'^[0-9]+\..*')
  matching_versions = [key for key in j.keys() if chrome_version_re.match(key)]
  matching_versions.sort(key=distutils.version.LooseVersion)
  for chrome_version in reversed(matching_versions):
    path = parsed_url[2].rstrip() + (
        '/+/refs/tags/%s/DEPS?format=text' % chrome_version)
    fh = gob_util.FetchUrl(parsed_url[1], path, ignore_404=False)
    content = fh.read() if fh else None
    if content:
      deps_content = base64.b64decode(content)
      if CheckIfChromeRightForOS(deps_content):
        return chrome_version

  return None


def _GetStickyEBuild(stable_ebuilds):
  """Returns the sticky ebuild."""
  sticky_ebuilds = []
  non_sticky_re = re.compile(_NON_STICKY_REGEX)
  for ebuild in stable_ebuilds:
    if not non_sticky_re.match(ebuild.version):
      sticky_ebuilds.append(ebuild)

  if not sticky_ebuilds:
    raise Exception('No sticky ebuilds found')
  elif len(sticky_ebuilds) > 1:
    logging.warning('More than one sticky ebuild found')

  return portage_util.BestEBuild(sticky_ebuilds)


class ChromeEBuild(portage_util.EBuild):
  """Thin sub-class of EBuild that adds a chrome_version field."""
  chrome_version_re = re.compile(r'.*-(%s|9999).*' % (
      _CHROME_VERSION_REGEX))
  chrome_version = ''

  def __init__(self, path):
    portage_util.EBuild.__init__(self, path)
    re_match = self.chrome_version_re.match(self.ebuild_path_no_revision)
    if re_match:
      self.chrome_version = re_match.group(1)

  def __str__(self):
    return self.ebuild_path


def FindChromeCandidates(package_dir):
  """Return a tuple of chrome's unstable ebuild and stable ebuilds.

  Args:
    package_dir: The path to where the package ebuild is stored.

  Returns:
    Tuple [unstable_ebuild, stable_ebuilds].

  Raises:
    Exception: if no unstable ebuild exists for Chrome.
  """
  stable_ebuilds = []
  unstable_ebuilds = []
  for path in [
      os.path.join(package_dir, entry) for entry in os.listdir(package_dir)]:
    if path.endswith('.ebuild'):
      ebuild = ChromeEBuild(path)
      if not ebuild.chrome_version:
        logging.warning('Poorly formatted ebuild found at %s' % path)
      else:
        if '9999' in ebuild.version:
          unstable_ebuilds.append(ebuild)
        else:
          stable_ebuilds.append(ebuild)

  # Apply some sanity checks.
  if not unstable_ebuilds:
    raise Exception('Missing 9999 ebuild for %s' % package_dir)
  if not stable_ebuilds:
    logging.warning('Missing stable ebuild for %s' % package_dir)

  return portage_util.BestEBuild(unstable_ebuilds), stable_ebuilds


def FindChromeUprevCandidate(stable_ebuilds, chrome_rev, sticky_branch):
  """Finds the Chrome uprev candidate for the given chrome_rev.

  Using the pre-flight logic, this means the stable ebuild you are uprevving
  from.  The difference here is that the version could be different and in
  that case we want to find it to delete it.

  Args:
    stable_ebuilds: A list of stable ebuilds.
    chrome_rev: The chrome_rev designating which candidate to find.
    sticky_branch: The the branch that is currently sticky with Major/Minor
      components.  For example: 9.0.553. Can be None but not if chrome_rev
      is CHROME_REV_STICKY.

  Returns:
    The EBuild, otherwise None if none found.
  """
  candidates = []
  if chrome_rev in [constants.CHROME_REV_LOCAL, constants.CHROME_REV_TOT,
                    constants.CHROME_REV_SPEC]:
    # These are labelled alpha, for historic reasons,
    # not just for the fun of confusion.
    chrome_branch_re = re.compile(r'%s.*_alpha.*' % _CHROME_VERSION_REGEX)
    for ebuild in stable_ebuilds:
      if chrome_branch_re.search(ebuild.version):
        candidates.append(ebuild)

  elif chrome_rev == constants.CHROME_REV_STICKY:
    assert sticky_branch is not None
    chrome_branch_re = re.compile(r'%s\..*' % sticky_branch)
    for ebuild in stable_ebuilds:
      if chrome_branch_re.search(ebuild.version):
        candidates.append(ebuild)

  else:
    chrome_branch_re = re.compile(r'%s.*_rc.*' % _CHROME_VERSION_REGEX)
    for ebuild in stable_ebuilds:
      if chrome_branch_re.search(ebuild.version):
        candidates.append(ebuild)

  if candidates:
    return portage_util.BestEBuild(candidates)
  else:
    return None


def GetChromeRevisionLinkFromVersions(old_chrome_version, chrome_version):
  """Return appropriately formatted link to revision info, given versions

  Given two chrome version strings (e.g. 9.0.533.0), generate a link to a
  page that prints the Chromium revisions between those two versions.

  Args:
    old_chrome_version: version to diff from
    chrome_version: version to which to diff

  Returns:
    The desired URL.
  """
  return _CHROME_VERSION_URL % {'old': old_chrome_version,
                                'new': chrome_version}


def GetChromeRevisionListLink(old_chrome, new_chrome, chrome_rev):
  """Returns a link to the list of revisions between two Chromium versions

  Given two ChromeEBuilds and the kind of rev we're doing, generate a
  link to a page that prints the Chromium changes between those two
  revisions, inclusive.

  Args:
    old_chrome: ebuild for the version to diff from
    new_chrome: ebuild for the version to which to diff
    chrome_rev: one of constants.VALID_CHROME_REVISIONS

  Returns:
    The desired URL.
  """
  assert chrome_rev in _REV_TYPES_FOR_LINKS
  return GetChromeRevisionLinkFromVersions(old_chrome.chrome_version,
                                           new_chrome.chrome_version)


def MarkChromeEBuildAsStable(stable_candidate, unstable_ebuild, chrome_pn,
                             chrome_rev, chrome_version, commit, package_dir):
  r"""Uprevs the chrome ebuild specified by chrome_rev.

  This is the main function that uprevs the chrome_rev from a stable candidate
  to its new version.

  Args:
    stable_candidate: ebuild that corresponds to the stable ebuild we are
      revving from.  If None, builds the a new ebuild given the version
      and logic for chrome_rev type with revision set to 1.
    unstable_ebuild: ebuild corresponding to the unstable ebuild for chrome.
    chrome_pn: package name.
    chrome_rev: one of constants.VALID_CHROME_REVISIONS or LOCAL
      constants.CHROME_REV_SPEC -  Requires commit value.  Revs the ebuild for
        the specified version and uses the portage suffix of _alpha.
      constants.CHROME_REV_TOT -  Requires commit value.  Revs the ebuild for
        the TOT version and uses the portage suffix of _alpha.
      constants.CHROME_REV_LOCAL - Requires a chrome_root. Revs the ebuild for
        the local version and uses the portage suffix of _alpha.
      constants.CHROME_REV_LATEST - This uses the portage suffix of _rc as they
        are release candidates for the next sticky version.
      constants.CHROME_REV_STICKY -  Revs the sticky version.
    chrome_version: The \d.\d.\d.\d version of Chrome.
    commit: Used with constants.CHROME_REV_TOT.  The git revision of chrome.
    package_dir: Path to the chromeos-chrome package dir.

  Returns:
    Full portage version atom (including rc's, etc) that was revved.
  """
  def IsTheNewEBuildRedundant(new_ebuild, stable_ebuild):
    """Returns True if the new ebuild is redundant.

    This is True if there if the current stable ebuild is the exact same copy
    of the new one.
    """
    if not stable_ebuild:
      return False

    if stable_candidate.chrome_version == new_ebuild.chrome_version:
      return filecmp.cmp(
          new_ebuild.ebuild_path, stable_ebuild.ebuild_path, shallow=False)

  # Mark latest release and sticky branches as stable.
  mark_stable = chrome_rev not in [constants.CHROME_REV_TOT,
                                   constants.CHROME_REV_SPEC,
                                   constants.CHROME_REV_LOCAL]

  # Case where we have the last stable candidate with same version just rev.
  if stable_candidate and stable_candidate.chrome_version == chrome_version:
    new_ebuild_path = '%s-r%d.ebuild' % (
        stable_candidate.ebuild_path_no_revision,
        stable_candidate.current_revision + 1)
  else:
    suffix = 'rc' if mark_stable else 'alpha'
    pf = '%s-%s_%s-r1' % (chrome_pn, chrome_version, suffix)
    new_ebuild_path = os.path.join(package_dir, '%s.ebuild' % pf)

  chrome_variables = dict()
  if commit:
    chrome_variables[_CHROME_SVN_TAG] = commit

  portage_util.EBuild.MarkAsStable(
      unstable_ebuild.ebuild_path, new_ebuild_path,
      chrome_variables, make_stable=mark_stable)
  new_ebuild = ChromeEBuild(new_ebuild_path)

  # Determine whether this is ebuild is redundant.
  if IsTheNewEBuildRedundant(new_ebuild, stable_candidate):
    msg = 'Previous ebuild with same version found and ebuild is redundant.'
    logging.info(msg)
    os.unlink(new_ebuild_path)
    return None

  if stable_candidate and chrome_rev in _REV_TYPES_FOR_LINKS:
    logging.PrintBuildbotLink('Chromium revisions',
                              GetChromeRevisionListLink(stable_candidate,
                                                        new_ebuild, chrome_rev))

  git.RunGit(package_dir, ['add', new_ebuild_path])
  if stable_candidate and not stable_candidate.IsSticky():
    git.RunGit(package_dir, ['rm', stable_candidate.ebuild_path])

  portage_util.EBuild.CommitChange(
      _GIT_COMMIT_MESSAGE % {'chrome_pn': chrome_pn,
                             'chrome_rev': chrome_rev,
                             'chrome_version': chrome_version},
      package_dir)

  return '%s-%s' % (new_ebuild.package, new_ebuild.version)


def GetParser():
  """Return a command line parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('-b', '--boards')
  parser.add_argument('-c', '--chrome_url',
                      default=constants.CHROMIUM_GOB_URL)
  parser.add_argument('-f', '--force_version',
                      help='Chrome version or git revision hash to use')
  parser.add_argument('-s', '--srcroot',
                      default=os.path.join(os.environ['HOME'], 'trunk', 'src'),
                      help='Path to the src directory')
  parser.add_argument('-t', '--tracking_branch', default='cros/master',
                      help='Branch we are tracking changes against')
  parser.add_argument('revision', choices=constants.VALID_CHROME_REVISIONS)
  return parser


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)
  options.Freeze()
  chrome_rev = options.revision

  if options.force_version and chrome_rev not in (constants.CHROME_REV_SPEC,
                                                  constants.CHROME_REV_LATEST):
    parser.error('--force_version is not compatible with the %r '
                 'option.' % (chrome_rev,))

  overlay_dir = os.path.abspath(_OVERLAY_DIR % {'srcroot': options.srcroot})
  chrome_package_dir = os.path.join(overlay_dir, constants.CHROME_CP)
  version_to_uprev = None
  commit_to_use = None
  sticky_branch = None

  (unstable_ebuild, stable_ebuilds) = FindChromeCandidates(chrome_package_dir)

  if chrome_rev == constants.CHROME_REV_LOCAL:
    if 'CHROME_ROOT' in os.environ:
      chrome_root = os.environ['CHROME_ROOT']
    else:
      chrome_root = os.path.join(os.environ['HOME'], 'chrome_root')

    version_to_uprev = _GetTipOfTrunkVersionFile(chrome_root)
    commit_to_use = 'Unknown'
    logging.info('Using local source, versioning is untrustworthy.')
  elif chrome_rev == constants.CHROME_REV_SPEC:
    if '.' in options.force_version:
      version_to_uprev = options.force_version
    else:
      commit_to_use = options.force_version
      if '@' in commit_to_use:
        commit_to_use = commit_to_use.rpartition('@')[2]
      version_to_uprev = _GetSpecificVersionUrl(options.chrome_url,
                                                commit_to_use)
  elif chrome_rev == constants.CHROME_REV_TOT:
    commit_to_use = gob_util.GetTipOfTrunkRevision(options.chrome_url)
    version_to_uprev = _GetSpecificVersionUrl(options.chrome_url,
                                              commit_to_use)
  elif chrome_rev == constants.CHROME_REV_LATEST:
    if options.force_version:
      if '.' not in options.force_version:
        parser.error('%s only accepts released Chrome versions, not SVN or '
                     'Git revisions.' % (chrome_rev,))
      version_to_uprev = options.force_version
    else:
      version_to_uprev = GetLatestRelease(options.chrome_url)
  else:
    sticky_ebuild = _GetStickyEBuild(stable_ebuilds)
    sticky_version = sticky_ebuild.chrome_version
    sticky_branch = sticky_version.rpartition('.')[0]
    version_to_uprev = GetLatestRelease(options.chrome_url, sticky_branch)

  stable_candidate = FindChromeUprevCandidate(stable_ebuilds, chrome_rev,
                                              sticky_branch)

  if stable_candidate:
    logging.info('Stable candidate found %s' % stable_candidate)
  else:
    logging.info('No stable candidate found.')

  tracking_branch = 'remotes/m/%s' % os.path.basename(options.tracking_branch)
  existing_branch = git.GetCurrentBranch(chrome_package_dir)
  work_branch = cros_mark_as_stable.GitBranch(constants.STABLE_EBUILD_BRANCH,
                                              tracking_branch,
                                              chrome_package_dir)
  work_branch.CreateBranch()

  # In the case of uprevving overlays that have patches applied to them,
  # include the patched changes in the stabilizing branch.
  if existing_branch:
    git.RunGit(chrome_package_dir, ['rebase', existing_branch])

  chrome_version_atom = MarkChromeEBuildAsStable(
      stable_candidate, unstable_ebuild, 'chromeos-chrome', chrome_rev,
      version_to_uprev, commit_to_use, chrome_package_dir)
  if chrome_version_atom:
    if options.boards:
      cros_mark_as_stable.CleanStalePackages(options.srcroot,
                                             options.boards.split(':'),
                                             [chrome_version_atom])

    # If we did rev Chrome, now is a good time to uprev other packages.
    for other_ebuild in constants.OTHER_CHROME_PACKAGES:
      other_ebuild_name = os.path.basename(other_ebuild)
      other_package_dir = os.path.join(overlay_dir, other_ebuild)
      (other_unstable_ebuild, other_stable_ebuilds) = FindChromeCandidates(
          other_package_dir)
      other_stable_candidate = FindChromeUprevCandidate(other_stable_ebuilds,
                                                        chrome_rev,
                                                        sticky_branch)
      revved_atom = MarkChromeEBuildAsStable(other_stable_candidate,
                                             other_unstable_ebuild,
                                             other_ebuild_name,
                                             chrome_rev, version_to_uprev,
                                             commit_to_use, other_package_dir)
      if revved_atom and options.boards:
        cros_mark_as_stable.CleanStalePackages(options.srcroot,
                                               options.boards.split(':'),
                                               [revved_atom])

  # Explicit print to communicate to caller.
  if chrome_version_atom:
    print('CHROME_VERSION_ATOM=%s' % chrome_version_atom)
