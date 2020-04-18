# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Routines and classes for working with Portage overlays and ebuilds."""

from __future__ import print_function

import collections
import errno
import fileinput
import glob
import itertools
import multiprocessing
import os
import re
import shutil
import sys

from chromite.lib import constants
from chromite.lib import failures_lib
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import parallel


# The parsed output of running `ebuild <ebuild path> info`.
RepositoryInfoTuple = collections.namedtuple('RepositoryInfoTuple',
                                             ('srcdir', 'project'))


_PRIVATE_PREFIX = '%(buildroot)s/src/private-overlays'

# Define datastructures for holding PV and CPV objects.
_PV_FIELDS = ['pv', 'package', 'version', 'version_no_rev', 'rev']
PV = collections.namedtuple('PV', _PV_FIELDS)
CPV = collections.namedtuple('CPV', ['category'] + _PV_FIELDS)

# Package matching regexp, as dictated by package manager specification:
# http://www.gentoo.org/proj/en/qa/pms.xml
_pkg = r'(?P<package>' + r'[\w+][\w+-]*)'
_ver = (r'(?P<version>'
        r'(?P<version_no_rev>(\d+)((\.\d+)*)([a-z]?)'
        r'((_(pre|p|beta|alpha|rc)\d*)*))'
        r'(-(?P<rev>r(\d+)))?)')
_pvr_re = re.compile(r'^(?P<pv>%s-%s)$' % (_pkg, _ver), re.VERBOSE)

# This regex matches a category name.
_category_re = re.compile(r'^(?P<category>[\w\+\.][\w\+\.\-]*)$', re.VERBOSE)

# This regex matches blank lines, commented lines, and the EAPI line.
_blank_or_eapi_re = re.compile(r'^\s*(?:#|EAPI=|$)')

# This regex is used to extract test names from IUSE_TESTS
_autotest_re = re.compile(r'\+tests_(\w+)', re.VERBOSE)

WORKON_EBUILD_VERSION = '9999'
WORKON_EBUILD_SUFFIX = '-%s.ebuild' % WORKON_EBUILD_VERSION

UNITTEST_PACKAGE_BLACKLIST = set((
    'sys-devel/binutils',
))

# A structure to hold computed values of CROS_WORKON_*.
CrosWorkonVars = collections.namedtuple(
    'CrosWorkonVars',
    ('localname', 'project', 'srcpath', 'always_live', 'commit', 'rev_subdirs',
     'subtrees'))

# EBuild source information computed from CrosWorkonVars.
SourceInfo = collections.namedtuple(
    'SourceInfo',
    (
        # List of project names.
        'projects',
        # List of source git directory paths. They are guaranteed to exist,
        # be a directory.
        'srcdirs',
        # List of source paths under subdirs. Their existence is ensured by
        # cros-workon.eclass. They can be directories or regular files.
        'subtrees',
    ))


class MissingOverlayException(Exception):
  """This exception indicates that a needed overlay is missing."""


def GetOverlayRoot(path):
  """Get the overlay root folder for |path|.

  For traditional portage overlays, the root folder is |path|.
  """
  return path


def _ListOverlays(board=None, buildroot=constants.SOURCE_ROOT):
  """Return the list of overlays to use for a given buildbot.

  Always returns all overlays in parent -> child order, and does not
  perform any filtering.

  Args:
    board: Board to look at.
    buildroot: Source root to find overlays.
  """
  # Load all the known overlays so we can extract the details below.
  paths = (
      'projects',
      'src/overlays',
      'src/private-overlays',
      'src/third_party',
  )
  overlays = {}
  for path in paths:
    path = os.path.join(buildroot, path, '*')
    for overlay in glob.glob(path):
      name = GetOverlayName(overlay)
      if name is None:
        continue

      # Sanity check the sets of repos.
      if name in overlays:
        raise RuntimeError('multiple repos with same name "%s": %s and %s' %
                           (name, overlays[name]['path'], overlay))

      try:
        masters = cros_build_lib.LoadKeyValueFile(
            os.path.join(GetOverlayRoot(overlay), 'metadata',
                         'layout.conf'))['masters'].split()
      except (KeyError, IOError):
        masters = []
      overlays[name] = {
          'masters': masters,
          'path': GetOverlayRoot(overlay),
      }

  # Easy enough -- dump them all.
  if board is None:
    return [x['path'] for x in overlays.values()]

  # Build up the list of repos we need.
  ret = []
  seen = set()
  def _AddRepo(repo, optional=False):
    """Recursively add |repo|'s masters from |overlays| to |ret|.

    Args:
      repo: The repo name to look up.
      optional: If |repo| does not exist, return False, else
        raise an MissingOverlayException.

    Returns:
      True if |repo| was found.
    """
    if repo not in overlays:
      if optional:
        return False
      else:
        raise MissingOverlayException('%s was not found' % repo)

    for master in overlays[repo]['masters'] + [repo]:
      if master not in seen:
        seen.add(master)
        _AddRepo(master)
        ret.append(overlays[master]['path'])
        if not master.endswith('-private'):
          _AddRepo('%s-private' % master, True)
    return True

  # Legacy: load the global configs.  In the future, this should be found
  # via the overlay's masters.
  _AddRepo('chromeos', optional=True)
  path = os.path.join(buildroot, 'src', 'private-overlays',
                      'chromeos-*-overlay')
  ret += glob.glob(path)

  # Locate the board repo by name.
  # Load the public & private versions if available.
  found_pub = _AddRepo(board, optional=True)
  found_priv = _AddRepo('%s-private' % board, optional=True)

  # If neither public nor private board was found, die.
  if not found_pub and not found_priv:
    raise MissingOverlayException('board overlay not found: %s' % board)

  return ret


def FindOverlays(overlay_type, board=None, buildroot=constants.SOURCE_ROOT):
  """Return the list of overlays to use for a given buildbot.

  The returned list of overlays will be in parent -> child order.

  Args:
    overlay_type: A string describing which overlays you want.
      'private': Just the private overlays.
      'public': Just the public overlays.
      'both': Both the public and private overlays.
    board: Board to look at.
    buildroot: Source root to find overlays.
  """
  overlays = _ListOverlays(board=board, buildroot=buildroot)
  private_prefix = _PRIVATE_PREFIX % dict(buildroot=buildroot)
  if overlay_type == constants.PRIVATE_OVERLAYS:
    return [x for x in overlays if x.startswith(private_prefix)]
  elif overlay_type == constants.PUBLIC_OVERLAYS:
    return [x for x in overlays if not x.startswith(private_prefix)]
  elif overlay_type == constants.BOTH_OVERLAYS:
    return overlays
  else:
    assert overlay_type is None
    return []


def FindOverlayFile(filename, overlay_type='both', board=None,
                    buildroot=constants.SOURCE_ROOT):
  """Attempt to find a file in the overlay directories.

  Searches through this board's overlays for the specified file. The
  overlays are searched in child -> parent order.

  Args:
    filename: Path to search for inside the overlay.
    overlay_type: A string describing which overlays you want.
      'private': Just the private overlays.
      'public': Just the public overlays.
      'both': Both the public and private overlays.
    board: Board to look at.
    buildroot: Source root to find overlays.

  Returns:
    Path to the first file found in the search. None if the file is not found.
  """
  for overlay in reversed(FindOverlays(overlay_type, board, buildroot)):
    if os.path.isfile(os.path.join(overlay, filename)):
      return os.path.join(overlay, filename)
  return None


def FindSysrootOverlays(sysroot):
  """Ask portage for a list of overlays installed in a given sysroot.

  Returns overlays in lowest to highest priority.  Note that this list
  is only partially ordered.

  Args:
    sysroot: The root directory being inspected.

  Returns:
    list of overlays used in sysroot.
  """
  cmd = (cros_build_lib.GetSysrootToolPath(sysroot, 'portageq'),
         'envvar', 'PORTDIR_OVERLAY')
  return cros_build_lib.RunCommand(cmd, print_cmd=False,
                                   capture_output=True).output.strip().split()


def ReadOverlayFile(filename, overlay_type='both', board=None,
                    buildroot=constants.SOURCE_ROOT):
  """Attempt to open a file in the overlay directories.

  Searches through this board's overlays for the specified file. The
  overlays are searched in child -> parent order.

  Args:
    filename: Path to open inside the overlay.
    overlay_type: A string describing which overlays you want.
      'private': Just the private overlays.
      'public': Just the public overlays.
      'both': Both the public and private overlays.
    board: Board to look at.
    buildroot: Source root to find overlays.

  Returns:
    The contents of the file, or None if no files could be opened.
  """
  file_found = FindOverlayFile(filename, overlay_type, board, buildroot)
  if file_found is None:
    return None
  return osutils.ReadFile(file_found)


def FindPrimaryOverlay(overlay_type, board, buildroot=constants.SOURCE_ROOT):
  """Return the primary overlay to use for a given buildbot.

  An overlay is only considered a primary overlay if it has a make.conf and a
  toolchain.conf. If multiple primary overlays are found, the first primary
  overlay is returned.

  Args:
    overlay_type: A string describing which overlays you want.
      'private': Just the private overlays.
      'public': Just the public overlays.
      'both': Both the public and private overlays.
    board: Board to look at.
    buildroot: Path to root of build directory.

  Raises:
    MissingOverlayException: No primary overlay found.
  """
  for overlay in FindOverlays(overlay_type, board, buildroot):
    if (os.path.exists(os.path.join(overlay, 'make.conf')) and
        os.path.exists(os.path.join(overlay, 'toolchain.conf'))):
      return overlay
  raise MissingOverlayException('No primary overlay found for board=%r' % board)


def GetOverlayName(overlay):
  """Get the self-declared repo name for the |overlay| path."""
  try:
    return cros_build_lib.LoadKeyValueFile(
        os.path.join(GetOverlayRoot(overlay), 'metadata',
                     'layout.conf'))['repo-name']
  except (KeyError, IOError):
    # Not all layout.conf files have a repo-name, so don't make a fuss.
    try:
      with open(os.path.join(overlay, 'profiles', 'repo_name')) as f:
        return f.readline().rstrip()
    except IOError:
      # Not all overlays have a repo_name, so don't make a fuss.
      return None


class EBuildVersionFormatException(Exception):
  """Exception for bad ebuild version string format."""

  def __init__(self, filename):
    self.filename = filename
    message = ('Ebuild file name %s '
               'does not match expected format.' % filename)
    super(EBuildVersionFormatException, self).__init__(message)


class EbuildFormatIncorrectException(Exception):
  """Exception for bad ebuild format."""

  def __init__(self, filename, message):
    message = 'Ebuild %s has invalid format: %s ' % (filename, message)
    super(EbuildFormatIncorrectException, self).__init__(message)


class EBuild(object):
  """Wrapper class for information about an ebuild."""

  VERBOSE = False
  _PACKAGE_VERSION_PATTERN = re.compile(
      r'.*-(([0-9][0-9a-z_.]*)(-r[0-9]+)?)[.]ebuild')
  _WORKON_COMMIT_PATTERN = re.compile(r'^CROS_WORKON_COMMIT=')

  @classmethod
  def _RunCommand(cls, command, **kwargs):
    kwargs.setdefault('capture_output', True)
    return cros_build_lib.RunCommand(
        command, print_cmd=cls.VERBOSE, **kwargs).output

  @classmethod
  def _RunGit(cls, cwd, command, **kwargs):
    result = git.RunGit(cwd, command, print_cmd=cls.VERBOSE, **kwargs)
    return None if result is None else result.output

  def IsSticky(self):
    """Returns True if the ebuild is sticky."""
    return self.is_stable and self.current_revision == 0

  @classmethod
  def UpdateEBuild(cls, ebuild_path, variables, redirect_file=None,
                   make_stable=True):
    """Static function that updates WORKON information in the ebuild.

    This function takes an ebuild_path and updates WORKON information.

    Note: If an exception is thrown, the |ebuild_path| is left in a corrupt
    state.  You should try to avoid causing exceptions ;).

    Args:
      ebuild_path: The path of the ebuild.
      variables: Dictionary of variables to update in ebuild.
      redirect_file: Optionally redirect output of new ebuild somewhere else.
      make_stable: Actually make the ebuild stable.
    """
    written = False
    try:
      for line in fileinput.input(ebuild_path, inplace=1):
        # Has to be done here to get changes to sys.stdout from fileinput.input.
        if not redirect_file:
          redirect_file = sys.stdout

        # Always add variables at the top of the ebuild, before the first
        # nonblank line other than the EAPI line.
        if not written and not _blank_or_eapi_re.match(line):
          for key, value in sorted(variables.items()):
            assert key is not None and value is not None
            redirect_file.write('%s=%s\n' % (key, value))
          written = True

        # Mark KEYWORDS as stable by removing ~'s.
        if line.startswith('KEYWORDS=') and make_stable:
          line = line.replace('~', '')

        varname, eq, _ = line.partition('=')
        if not (eq == '=' and varname.strip() in variables):
          # Don't write out the old value of the variable.
          redirect_file.write(line)
    finally:
      fileinput.close()

  @classmethod
  def MarkAsStable(cls, unstable_ebuild_path, new_stable_ebuild_path,
                   variables, redirect_file=None, make_stable=True):
    """Static function that creates a revved stable ebuild.

    This function assumes you have already figured out the name of the new
    stable ebuild path and then creates that file from the given unstable
    ebuild and marks it as stable.  If the commit_value is set, it also
    set the commit_keyword=commit_value pair in the ebuild.

    Args:
      unstable_ebuild_path: The path to the unstable ebuild.
      new_stable_ebuild_path: The path you want to use for the new stable
        ebuild.
      variables: Dictionary of variables to update in ebuild.
      redirect_file: Optionally redirect output of new ebuild somewhere else.
      make_stable: Actually make the ebuild stable.
    """
    shutil.copyfile(unstable_ebuild_path, new_stable_ebuild_path)
    EBuild.UpdateEBuild(new_stable_ebuild_path, variables, redirect_file,
                        make_stable)

  @classmethod
  def CommitChange(cls, message, overlay):
    """Commits current changes in git locally with given commit message.

    Args:
      message: the commit string to write when committing to git.
      overlay: directory in which to commit the changes.

    Raises:
      RunCommandError: Error occurred while committing.
    """
    logging.info('Committing changes with commit message: %s', message)
    git_commit_cmd = ['commit', '-a', '-m', message]
    cls._RunGit(overlay, git_commit_cmd)

  def __init__(self, path):
    """Sets up data about an ebuild from its path.

    Args:
      path: Path to the ebuild.
    """
    self.overlay, self.category, self.pkgname, filename = path.rsplit('/', 3)
    m = self._PACKAGE_VERSION_PATTERN.match(filename)
    if not m:
      raise EBuildVersionFormatException(filename)
    self.version, self.version_no_rev, revision = m.groups()
    if revision is not None:
      self.current_revision = int(revision.replace('-r', ''))
    else:
      self.current_revision = 0
    self.package = '%s/%s' % (self.category, self.pkgname)

    self._ebuild_path_no_version = os.path.join(
        os.path.dirname(path), self.pkgname)
    self.ebuild_path_no_revision = '%s-%s' % (
        self._ebuild_path_no_version, self.version_no_rev)
    self._unstable_ebuild_path = '%s%s' % (
        self._ebuild_path_no_version, WORKON_EBUILD_SUFFIX)
    self.ebuild_path = path

    self.is_workon = False
    self.is_stable = False
    self.is_blacklisted = False
    self.has_test = False
    self._ReadEBuild(path)

    # Grab the latest project settings.
    try:
      new_vars = EBuild.GetCrosWorkonVars(
          self._unstable_ebuild_path, self.pkgname)
    except EbuildFormatIncorrectException:
      new_vars = None

    # Grab the current project settings.
    try:
      old_vars = EBuild.GetCrosWorkonVars(self.ebuild_path, self.pkgname)
    except EbuildFormatIncorrectException:
      old_vars = None

    # Merge the two settings.
    self.cros_workon_vars = old_vars
    if new_vars is not None and old_vars is not None:
      merged_vars = new_vars._replace(commit=old_vars.commit)
      # If the project settings have changed, throw away existing vars (use
      # new_vars).
      if merged_vars != old_vars:
        self.cros_workon_vars = new_vars

  @staticmethod
  def Classify(ebuild_path):
    """Return whether this ebuild is workon, stable, and/or blacklisted

    workon is determined by whether the ebuild inherits from the
    'cros-workon' eclass. stable is determined by whether there's a '~'
    in the KEYWORDS setting in the ebuild. An ebuild is considered blacklisted
    if a line in it starts with 'CROS_WORKON_BLACKLIST='
    """
    is_workon = False
    is_stable = False
    is_blacklisted = False
    has_test = False
    for line in fileinput.input(ebuild_path):
      if line.startswith('inherit ') and 'cros-workon' in line:
        is_workon = True
      elif line.startswith('KEYWORDS='):
        for keyword in line.split('=', 1)[1].strip("\"'").split():
          if not keyword.startswith('~') and keyword != '-*':
            is_stable = True
      elif line.startswith('CROS_WORKON_BLACKLIST='):
        is_blacklisted = True
      elif (line.startswith('src_test()') or
            line.startswith('platform_pkg_test()')):
        has_test = True
    fileinput.close()
    return is_workon, is_stable, is_blacklisted, has_test

  def _ReadEBuild(self, path):
    """Determine the settings of `is_workon`, `is_stable` and is_blacklisted

    These are determined using the static Classify function.
    """
    (self.is_workon, self.is_stable,
     self.is_blacklisted, self.has_test) = EBuild.Classify(path)

  @staticmethod
  def _GetAutotestTestsFromSettings(settings):
    """Return a list of test names, when given a settings dictionary.

    Args:
      settings: A dictionary containing ebuild variables contents.

    Returns:
      A list of test name strings.
    """
    # We do a bit of string wrangling to extract directory names from test
    # names. First, get rid of special characters.
    test_list = []
    raw_tests_str = settings['IUSE_TESTS']
    if len(raw_tests_str) == 0:
      return test_list

    test_list.extend(_autotest_re.findall(raw_tests_str))
    return test_list

  @staticmethod
  def GetAutotestSubdirsToRev(ebuild_path, srcdir):
    """Return list of subdirs to be watched while deciding whether to uprev.

    This logic is specific to autotest related ebuilds, that derive from the
    autotest eclass.

    Args:
      ebuild_path: Path to the ebuild file (e.g
                   autotest-tests-graphics-9999.ebuild).
      srcdir: The path of the source for the test

    Returns:
      A list of strings mentioning directory paths.
    """
    results = []
    test_vars = ('IUSE_TESTS',)

    if not ebuild_path or not srcdir:
      return results

    # TODO(pmalani): Can we get this from get_test_list in autotest.eclass ?
    settings = osutils.SourceEnvironment(ebuild_path, test_vars, env=None,
                                         multiline=True)
    if 'IUSE_TESTS' not in settings:
      return results

    test_list = EBuild._GetAutotestTestsFromSettings(settings)

    location = ['client', 'server']
    test_type = ['tests', 'site_tests']

    # Check the existence of every directory combination of location and
    # test_type for each test.
    # This is a re-implementation of the same logic in autotest_src_prepare in
    # the chromiumos-overlay/eclass/autotest.eclass .
    for cur_test in test_list:
      for x, y in list(itertools.product(location, test_type)):
        a = os.path.join(srcdir, x, y, cur_test)
        if os.path.isdir(a):
          results.append(os.path.join(x, y, cur_test))

    return results

  @staticmethod
  def GetCrosWorkonVars(ebuild_path, pkg_name):
    """Return the finalized values of CROS_WORKON vars in an ebuild script.

    Args:
      ebuild_path: Path to the ebuild file (e.g: platform2-9999.ebuild).
      pkg_name: The package name (e.g.: platform2).

    Returns:
      A CrosWorkonVars tuple.
    """
    cros_workon_vars = EBuild._ReadCrosWorkonVars(ebuild_path, pkg_name)
    return EBuild._FinalizeCrosWorkonVars(cros_workon_vars, ebuild_path)

  @staticmethod
  def _ReadCrosWorkonVars(ebuild_path, pkg_name):
    """Return the raw values of CROS_WORKON vars in an ebuild script.

    Args:
      ebuild_path: Path to the ebuild file (e.g: platform2-9999.ebuild).
      pkg_name: The package name (e.g.: platform2).

    Returns:
      A CrosWorkonVars tuple.
    """
    workon_vars = (
        'CROS_WORKON_LOCALNAME',
        'CROS_WORKON_PROJECT',
        'CROS_WORKON_SRCPATH',
        'CROS_WORKON_ALWAYS_LIVE',
        'CROS_WORKON_COMMIT',
        'CROS_WORKON_SUBDIRS_TO_REV',
        'CROS_WORKON_SUBTREE',
    )
    env = {
        'CROS_WORKON_LOCALNAME': pkg_name,
        'CROS_WORKON_ALWAYS_LIVE': '',
    }
    settings = osutils.SourceEnvironment(ebuild_path, workon_vars, env=env)
    # Try to detect problems extracting the variables by checking whether
    # either CROS_WORKON_PROJECT or CROS_WORK_SRCPATH is set. If it isn't,
    # something went wrong, possibly because we're simplistically sourcing the
    # ebuild without most of portage being available. That still breaks this
    # script and needs to be flagged as an error. We won't catch problems
    # setting CROS_WORKON_LOCALNAME or if CROS_WORKON_{PROJECT,SRCPATH} is set
    # to the wrong thing, but at least this covers some types of failures.
    projects = []
    srcpaths = []
    rev_subdirs = []
    if 'CROS_WORKON_PROJECT' in settings:
      projects = settings['CROS_WORKON_PROJECT'].split(',')
    if 'CROS_WORKON_SRCPATH' in settings:
      srcpaths = settings['CROS_WORKON_SRCPATH'].split(',')
    if 'CROS_WORKON_SUBDIRS_TO_REV' in settings:
      rev_subdirs = settings['CROS_WORKON_SUBDIRS_TO_REV'].split(',')

    if not (projects or srcpaths):
      raise EbuildFormatIncorrectException(
          ebuild_path,
          'Unable to determine CROS_WORKON_{PROJECT,SRCPATH} values.')

    localnames = settings['CROS_WORKON_LOCALNAME'].split(',')
    live = settings['CROS_WORKON_ALWAYS_LIVE']
    commit = settings.get('CROS_WORKON_COMMIT')
    subtrees = [
        tuple(subtree.split() or [''])
        for subtree in settings.get('CROS_WORKON_SUBTREE', '').split(',')]
    if (len(projects) > 1 or len(srcpaths) > 1) and len(rev_subdirs) > 0:
      raise EbuildFormatIncorrectException(
          ebuild_path,
          'Must not define CROS_WORKON_SUBDIRS_TO_REV if defining multiple '
          'cros_workon projects or source paths.')

    return CrosWorkonVars(
        localname=localnames,
        project=projects,
        srcpath=srcpaths,
        always_live=live,
        commit=commit,
        rev_subdirs=rev_subdirs,
        subtrees=subtrees)

  @staticmethod
  def _FinalizeCrosWorkonVars(cros_workon_vars, ebuild_path):
    """Finalize CrosWorkonVars tuple.

    It is allowed to set different number of entries in CROS_WORKON array
    variables. In that case, this function completes those variable so that
    all variables have the same number of entries.

    Args:
      cros_workon_vars: A CrosWorkonVars tuple.
      ebuild_path: Path to the ebuild file (e.g: platform2-9999.ebuild).

    Returns:
      A completed CrosWorkonVars tuple.
    """
    localnames = cros_workon_vars.localname
    projects = cros_workon_vars.project
    srcpaths = cros_workon_vars.srcpath
    subtrees = cros_workon_vars.subtrees

    # Sanity checks and completion.
    num_projects = len(projects)

    # Each project specification has to have the same amount of items.
    if num_projects != len(localnames):
      raise EbuildFormatIncorrectException(
          ebuild_path,
          'Number of _PROJECT and _LOCALNAME items don\'t match.')

    # If both SRCPATH and PROJECT are defined, they must have the same number
    # of items.
    if len(srcpaths) > num_projects:
      if num_projects > 0:
        raise EbuildFormatIncorrectException(
            ebuild_path,
            '_PROJECT has fewer items than _SRCPATH.')
      num_projects = len(srcpaths)
      projects = [''] * num_projects
      localnames = [''] * num_projects
    elif len(srcpaths) < num_projects:
      if len(srcpaths) > 0:
        raise EbuildFormatIncorrectException(
            ebuild_path,
            '_SRCPATH has fewer items than _PROJECT.')
      srcpaths = [''] * num_projects

    # We better have at least one PROJECT or SRCPATH value at this point.
    if num_projects == 0:
      raise EbuildFormatIncorrectException(
          ebuild_path, 'No _PROJECT or _SRCPATH value found.')

    # Subtree must be either 1 or len(project).
    if num_projects != len(subtrees):
      if len(subtrees) > 1:
        raise EbuildFormatIncorrectException(
            ebuild_path, 'Incorrect number of _SUBTREE items.')
      # Multiply by num_projects. Note that subtrees is a list of tuples, and
      # there should be at least one element.
      subtrees *= num_projects

    return cros_workon_vars._replace(
        localname=localnames,
        project=projects,
        srcpath=srcpaths,
        subtrees=subtrees)

  def GetSourceInfo(self, srcroot, manifest):
    """Get source information for this ebuild.

    Args:
      srcroot: Full path to the "src" subdirectory in the source repository.
      manifest: git.ManifestCheckout object.

    Returns:
      EBuild.SourceInfo namedtuple.
    """
    localnames = self.cros_workon_vars.localname
    projects = self.cros_workon_vars.project
    srcpaths = self.cros_workon_vars.srcpath
    always_live = self.cros_workon_vars.always_live
    subtrees = self.cros_workon_vars.subtrees

    if always_live:
      return SourceInfo(projects=[], srcdirs=[], subtrees=[])

    # Calculate srcdir (used for core packages).
    if self.category in ('chromeos-base', 'brillo-base'):
      dir_ = ''
    else:
      dir_ = 'third_party'

    srcbase = ''
    if any(srcpaths):
      base_dir = os.path.dirname(os.path.dirname(os.path.dirname(
          os.path.dirname(self._unstable_ebuild_path))))
      srcbase = os.path.join(base_dir, 'src')
      if not os.path.isdir(srcbase):
        cros_build_lib.Die('_SRCPATH used but source path not found.')

    subdir_paths = []
    subtree_paths = []
    rows = zip(localnames, projects, srcpaths, subtrees)
    for local, project, srcpath, subtree in rows:
      if srcpath:
        subdir_path = os.path.join(srcbase, srcpath)
        if not os.path.isdir(subdir_path):
          cros_build_lib.Die('Source for package %s not found.' % self.pkgname)
      else:
        subdir_path = os.path.realpath(os.path.join(srcroot, dir_, local))
        if dir_ == '' and not os.path.isdir(subdir_path):
          subdir_path = os.path.realpath(os.path.join(srcroot, 'platform',
                                                      local))

        if not os.path.isdir(subdir_path):
          cros_build_lib.Die('Source repository %s '
                             'for project %s does not exist.' % (subdir_path,
                                                                 self.pkgname))
        # Verify that we're grabbing the commit id from the right project name.
        real_project = manifest.FindCheckoutFromPath(subdir_path)['name']
        if project != real_project:
          cros_build_lib.Die('Project name mismatch for %s '
                             '(found %s, expected %s)' % (subdir_path,
                                                          real_project,
                                                          project))

      subdir_paths.append(subdir_path)
      subtree_paths.extend(
          os.path.join(subdir_path, s) if s else subdir_path
          for s in subtree)

    return SourceInfo(
        projects=projects, srcdirs=subdir_paths, subtrees=subtree_paths)

  def GetCommitId(self, srcdir):
    """Get the commit id for this ebuild."""
    output = self._RunGit(srcdir, ['rev-parse', 'HEAD'])
    if not output:
      cros_build_lib.Die('Cannot determine HEAD commit for %s' % srcdir)
    return output.rstrip()

  def GetTreeId(self, path):
    """Get the SHA1 of the source tree for this ebuild.

    Unlike the commit hash, the SHA1 of the source tree is unaffected by the
    history of the repository, or by commit messages.

    Given path can point a regular file, not a directory. If it does not exist,
    None is returned.
    """
    if not os.path.exists(path):
      return None
    if os.path.isdir(path):
      basedir = path
      relpath = ''
    else:
      basedir = os.path.dirname(path)
      relpath = os.path.basename(path)
    output = self._RunGit(basedir, ['rev-parse', 'HEAD:./%s' % relpath])
    if not output:
      cros_build_lib.Die('Cannot determine HEAD tree hash for %s' % path)
    return output.rstrip()

  def GetVersion(self, srcroot, manifest, default):
    """Get the base version number for this ebuild.

    The version is provided by the ebuild through a specific script in
    the $FILESDIR (chromeos-version.sh).
    """
    vers_script = os.path.join(os.path.dirname(self._ebuild_path_no_version),
                               'files', 'chromeos-version.sh')

    if not os.path.exists(vers_script):
      return default

    if not self.is_workon:
      raise EbuildFormatIncorrectException(
          self._ebuild_path_no_version,
          'Package has a chromeos-version.sh script but is not workon-able.')

    srcdirs = self.GetSourceInfo(srcroot, manifest).srcdirs

    # The chromeos-version script will output a usable raw version number,
    # or nothing in case of error or no available version
    try:
      output = self._RunCommand([vers_script] + srcdirs).strip()
    except cros_build_lib.RunCommandError as e:
      cros_build_lib.Die('Package %s chromeos-version.sh failed: %s' %
                         (self.pkgname, e))

    if not output:
      cros_build_lib.Die('Package %s has a chromeos-version.sh script but '
                         'it returned no valid version for "%s"' %
                         (self.pkgname, ' '.join(srcdirs)))

    # Sanity check: disallow versions that will be larger than the 9999 ebuild
    # used by cros-workon.
    main_pv = output.split('.', 1)[0]
    try:
      main_pv = int(main_pv)
    except ValueError:
      raise ValueError('PV returned is invalid: %s' % output)
    if main_pv >= int(WORKON_EBUILD_VERSION):
      raise ValueError('cros-workon packages must have a PV < %s; not %s'
                       % (WORKON_EBUILD_VERSION, output))

    return output

  @staticmethod
  def FormatBashArray(unformatted_list):
    """Returns a python list in a bash array format.

    If the list only has one item, format as simple quoted value.
    That is both backwards-compatible and more readable.

    Args:
      unformatted_list: an iterable to format as a bash array. This variable
        has to be sanitized first, as we don't do any safeties.

    Returns:
      A text string that can be used by bash as array declaration.
    """
    if len(unformatted_list) > 1:
      return '("%s")' % '" "'.join(unformatted_list)
    else:
      return '"%s"' % unformatted_list[0]

  def RevWorkOnEBuild(self, srcroot, manifest, redirect_file=None):
    """Revs a workon ebuild given the git commit hash.

    By default this class overwrites a new ebuild given the normal
    ebuild rev'ing logic.  However, a user can specify a redirect_file
    to redirect the new stable ebuild to another file.

    Args:
      srcroot: full path to the 'src' subdirectory in the source
        repository.
      manifest: git.ManifestCheckout object.
      redirect_file: Optional file to write the new ebuild.  By default
        it is written using the standard rev'ing logic.  This file must be
        opened and closed by the caller.

    Returns:
      If the revved package is different than the old ebuild, return a tuple
      of (full revved package name (including the version number), new stable
      ebuild path to add to git, old ebuild path to remove from git (if any)).
      Otherwise, return None.

    Raises:
      OSError: Error occurred while creating a new ebuild.
      IOError: Error occurred while writing to the new revved ebuild file.
    """

    if self.is_stable:
      stable_version_no_rev = self.GetVersion(srcroot, manifest,
                                              self.version_no_rev)
    else:
      # If given unstable ebuild, use preferred version rather than 9999.
      stable_version_no_rev = self.GetVersion(srcroot, manifest, '0.0.1')

    old_version = '%s-r%d' % (
        stable_version_no_rev, self.current_revision)
    old_stable_ebuild_path = '%s-%s.ebuild' % (
        self._ebuild_path_no_version, old_version)
    new_version = '%s-r%d' % (
        stable_version_no_rev, self.current_revision + 1)
    new_stable_ebuild_path = '%s-%s.ebuild' % (
        self._ebuild_path_no_version, new_version)

    info = self.GetSourceInfo(srcroot, manifest)
    srcdirs = info.srcdirs
    subtrees = info.subtrees
    commit_ids = map(self.GetCommitId, srcdirs)
    tree_ids = map(self.GetTreeId, subtrees)
    variables = dict(CROS_WORKON_COMMIT=self.FormatBashArray(commit_ids),
                     CROS_WORKON_TREE=self.FormatBashArray(tree_ids))

    # We use |self._unstable_ebuild_path| because that will contain the newest
    # changes to the ebuild (and potentially changes to test subdirs
    # themselves).
    subdirs_to_rev = self.GetAutotestSubdirsToRev(self._unstable_ebuild_path,
                                                  srcdirs[0])
    old_subdirs_to_rev = self.GetAutotestSubdirsToRev(self.ebuild_path,
                                                      srcdirs[0])
    test_dirs_changed = False
    if sorted(subdirs_to_rev) != sorted(old_subdirs_to_rev):
      logging.info(
          'The list of subdirs in the ebuild %s has changed, upreving.',
          self.pkgname)
      test_dirs_changed = True

    old_stable_commit = self._RunGit(self.overlay,
                                     ['log', '--pretty=%H', '-n1', '--',
                                      old_stable_ebuild_path]).rstrip()
    output = self._RunGit(self.overlay,
                          ['log', '%s..HEAD' % old_stable_commit, '--',
                           self._unstable_ebuild_path,
                           os.path.join(os.path.dirname(self.ebuild_path),
                                        'files')])

    unstable_ebuild_or_files_changed = bool(output)

    # If there has been any change in the tests list, choose to uprev.
    if (not test_dirs_changed and not unstable_ebuild_or_files_changed and
        not self._ShouldRevEBuild(commit_ids, srcdirs, subdirs_to_rev)):
      logging.info('Skipping uprev of ebuild %s, none of the rev_subdirs have '
                   'been modified, no files/, nor has the -9999 ebuild.' %
                   self.pkgname)
      return

    logging.info('Creating new stable ebuild %s' % new_stable_ebuild_path)
    if not os.path.exists(self._unstable_ebuild_path):
      cros_build_lib.Die('Missing unstable ebuild: %s' %
                         self._unstable_ebuild_path)

    self.MarkAsStable(self._unstable_ebuild_path, new_stable_ebuild_path,
                      variables, redirect_file)

    old_ebuild_path = self.ebuild_path
    if (EBuild._AlmostSameEBuilds(old_ebuild_path, new_stable_ebuild_path) and
        not unstable_ebuild_or_files_changed):
      logging.info('Old and new ebuild %s are exactly identical; '
                   'skipping uprev', new_stable_ebuild_path)
      os.unlink(new_stable_ebuild_path)
      return
    else:
      ebuild_path_to_remove = old_ebuild_path if self.is_stable else None
      return ('%s-%s' % (self.package, new_version),
              new_stable_ebuild_path, ebuild_path_to_remove)

  def _ShouldRevEBuild(self, commit_ids, srcdirs, subdirs_to_rev):
    """Determine whether we should attempt to rev |ebuild|.

    If CROS_WORKON_SUBDIRS_TO_REV is not defined for |ebuild|, and
    subdirs_to_rev is empty, this function trivially returns True.

    Args:
      commit_ids: Commit ID of the tip of tree for the source dir.
      srcdirs: Source direutory where the git repo is located.
      subdirs_to_rev: Test subdirectories which have to be checked for
      modifications since the last stable commit hash.

    Returns:
      True is an Uprev is needed, False otherwise.
    """
    if not self.cros_workon_vars:
      return True
    if not self.cros_workon_vars.commit:
      return True
    if len(commit_ids) != 1:
      return True
    if len(srcdirs) != 1:
      return True
    if not subdirs_to_rev and not self.cros_workon_vars.rev_subdirs:
      return True

    current_commit_hash = commit_ids[0]
    stable_commit_hash = self.cros_workon_vars.commit
    srcdir = srcdirs[0]
    logrange = '%s..%s' % (stable_commit_hash, current_commit_hash)
    dirs = []
    dirs.extend(self.cros_workon_vars.rev_subdirs)
    dirs.extend(subdirs_to_rev)
    if dirs:
      # Any change to the unstable ebuild must generate an uprev. If there are
      # no dirs then this happens automatically (since the git log has no file
      # list). Otherwise we must ensure that it works here.
      dirs.append('*9999.ebuild')
    git_args = ['log', '--oneline', logrange, '--'] + dirs

    try:
      output = EBuild._RunGit(srcdir, git_args)
    except cros_build_lib.RunCommandError as ex:
      logging.warning(str(ex))
      return True

    if output:
      logging.info(' Rev: Determined that one+ of the ebuild %s rev_subdirs '
                   'was touched %s', self.pkgname, list(subdirs_to_rev))
      return True
    else:
      logging.info('Skip: Determined that none of the ebuild %s rev_subdirs '
                   'was touched %s', self.pkgname, list(subdirs_to_rev))
      return False

  @classmethod
  def GitRepoHasChanges(cls, directory):
    """Returns True if there are changes in the given directory."""
    # Refresh the index first. This squashes just metadata changes.
    cls._RunGit(directory, ['update-index', '-q', '--refresh'])
    output = cls._RunGit(directory, ['diff-index', '--name-only', 'HEAD'])
    return output not in [None, '']

  @classmethod
  def _AlmostSameEBuilds(cls, ebuild_path1, ebuild_path2):
    """Checks if two ebuilds are the same except for CROS_WORKON_COMMIT line.

    Even if CROS_WORKON_COMMIT is different, as long as CROS_WORKON_TREE is
    the same, we can guarantee the source tree is identical.
    """
    return (
        cls._LoadEBuildForComparison(ebuild_path1) ==
        cls._LoadEBuildForComparison(ebuild_path2))

  @classmethod
  def _LoadEBuildForComparison(cls, ebuild_path):
    """Loads an ebuild file dropping CROS_WORKON_COMMIT line."""
    lines = osutils.ReadFile(ebuild_path).splitlines()
    return '\n'.join(
        line for line in lines
        if not cls._WORKON_COMMIT_PATTERN.search(line))


class PortageDBException(Exception):
  """Generic PortageDB error."""


class PortageDB(object):
  """Wrapper class to access the portage database located in var/db/pkg."""

  def __init__(self, root='/'):
    """Initialize the internal structure for the database in the given root.

    Args:
      root: The path to the root to inspect, for example "/build/foo".
    """
    self.root = root
    self.db_path = os.path.join(root, 'var/db/pkg')
    self._ebuilds = {}

  def GetInstalledPackage(self, category, pv):
    """Get the InstalledPackage instance for the passed package.

    Args:
      category: The category of the package. For example "chromeos-base".
      pv: The package name with the version (and revision) of the
          installed package. For example "libchrome-271506-r5".

    Returns:
      An InstalledPackage instance for the requested package or None if the
      requested package is not found.
    """
    pkg_key = '%s/%s' % (category, pv)
    if pkg_key in self._ebuilds:
      return self._ebuilds[pkg_key]

    # Create a new InstalledPackage instance and cache it.
    pkgdir = os.path.join(self.db_path, category, pv)
    try:
      pkg = InstalledPackage(self, pkgdir, category, pv)
    except PortageDBException:
      return None
    self._ebuilds[pkg_key] = pkg
    return pkg

  def InstalledPackages(self):
    """Lists all portage packages in the database.

    Returns:
      A list of InstalledPackage instances for each package in the database.
    """
    ebuild_pattern = os.path.join(self.db_path, '*/*/*.ebuild')
    packages = []

    for path in glob.glob(ebuild_pattern):
      category, pf, packagecheck = SplitEbuildPath(path)
      if not _category_re.match(category):
        continue
      if pf != packagecheck:
        continue
      pkg_key = '%s/%s' % (category, pf)
      if pkg_key not in self._ebuilds:
        self._ebuilds[pkg_key] = InstalledPackage(
            self, os.path.join(self.db_path, category, pf),
            category, pf)
      packages.append(self._ebuilds[pkg_key])

    return packages


class InstalledPackage(object):
  """Wrapper class for information about an installed package.

  This class accesses the information provided by var/db/pkg for an installed
  ebuild, such as the list of files installed by this package.
  """

  # "type" constants for the ListContents() return value.
  OBJ = 'obj'
  SYM = 'sym'
  DIR = 'dir'

  def __init__(self, portage_db, pkgdir, category=None, pf=None):
    """Initialize the installed ebuild wrapper.

    Args:
      portage_db: The PortageDB instance where the ebuild is installed. This
          is used to query the database about other installed ebuilds, for
          example, the ones listed in DEPEND, but otherwise it isn't used.
      pkgdir: The directory where the installed package resides. This could be
          for example a directory like "var/db/pkg/category/pf" or the
          "build-info" directory in the portage temporary directory where
          the package is being built.
      category: The category of the package. If omitted, it will be loaded from
          the package contents.
      pf: The package and version of the package. If omitted, it will be loaded
          from the package contents. This avoids unncessary lookup when this
          value is known.

    Raises:
      PortageDBException if the pkgdir doesn't contain a valid package.
    """
    self._portage_db = portage_db
    self.pkgdir = pkgdir
    self._fields = {}
    # Prepopulate the field cache with the category and pf (if provided).
    if not category is None:
      self._fields['CATEGORY'] = category
    if not pf is None:
      self._fields['PF'] = pf

    if self.pf is None:
      raise PortageDBException("Package doesn't contain package-version value.")

    # Check that the ebuild is present.
    ebuild_path = os.path.join(self.pkgdir, '%s.ebuild' % self.pf)
    if not os.path.exists(ebuild_path):
      raise PortageDBException("Package doesn't contain an ebuild file.")

    split_pv = SplitPV(self.pf)
    if split_pv is None:
      raise PortageDBException('Package and version "%s" doesn\'t have a valid '
                               'format.' % self.pf)
    self.package = split_pv.package
    self.version = split_pv.version

  def _ReadField(self, field_name):
    """Reads the contents of the file in the installed package directory.

    Args:
      field_name: The name of the field to read, for example, 'SLOT' or
          'LICENSE'.

    Returns:
      A string with the contents of the file. The contents of the file are
      cached in _fields. If the file doesn't exists returns None.
    """
    if field_name not in self._fields:
      try:
        value = osutils.ReadFile(os.path.join(self.pkgdir, field_name))
      except IOError as e:
        if e.errno != errno.ENOENT:
          raise
        value = None
      self._fields[field_name] = value
    return self._fields[field_name]

  @property
  def category(self):
    return self._ReadField('CATEGORY')

  @property
  def pf(self):
    return self._ReadField('PF')

  def ListContents(self):
    """List of files and directories installed by this package.

    Returns:
      A list of tuples (file_type, path) where the file_type is a string
      determining the type of the installed file: InstalledPackage.OBJ (regular
      files), InstalledPackage.SYM (symlinks) or InstalledPackage.DIR
      (directory), and path is the relative path of the file to the root like
      'usr/bin/ls'.
    """
    path = os.path.join(self.pkgdir, 'CONTENTS')
    if not os.path.exists(path):
      return []

    result = []
    for line in open(path):
      line = line.strip()
      # Line format is: "type file_path [more space-separated fields]".
      # Discard any other line without at least the first two fields. The
      # remaining fields depend on the type.
      typ, data = line.split(' ', 1)
      if typ == self.OBJ:
        file_path, _file_hash, _mtime = data.rsplit(' ', 2)
      elif typ == self.DIR:
        file_path = data
      elif typ == self.SYM:
        file_path, _ = data.split(' -> ', 1)
      else:
        # Unknown type.
        continue
      result.append((typ, file_path.lstrip('/')))

    return result


def BestEBuild(ebuilds):
  """Returns the newest EBuild from a list of EBuild objects."""
  from portage.versions import vercmp  # pylint: disable=import-error
  winner = ebuilds[0]
  for ebuild in ebuilds[1:]:
    if vercmp(winner.version, ebuild.version) < 0:
      winner = ebuild
  return winner


def _FindUprevCandidates(files, allow_blacklisted=False):
  """Return the uprev candidate ebuild from a specified list of files.

  Usually an uprev candidate is a the stable ebuild in a cros_workon
  directory.  However, if no such stable ebuild exists (someone just
  checked in the 9999 ebuild), this is the unstable ebuild.

  If the package isn't a cros_workon package, return None.

  Args:
    files: List of files in a package directory.
    allow_blacklisted: If False, discard blacklisted packages.
  """
  stable_ebuilds = []
  unstable_ebuilds = []
  for path in files:
    if not path.endswith('.ebuild') or os.path.islink(path):
      continue
    ebuild = EBuild(path)
    if not ebuild.is_workon or (ebuild.is_blacklisted and
                                not allow_blacklisted):
      continue
    if ebuild.is_stable:
      if ebuild.version == WORKON_EBUILD_VERSION:
        cros_build_lib.Die('KEYWORDS in %s ebuild should not be stable %s'
                           % (WORKON_EBUILD_VERSION, path))
      stable_ebuilds.append(ebuild)
    else:
      unstable_ebuilds.append(ebuild)

  # If both ebuild lists are empty, the passed in file list was for
  # a non-workon package.
  if not unstable_ebuilds:
    if stable_ebuilds:
      path = os.path.dirname(stable_ebuilds[0].ebuild_path)
      cros_build_lib.Die(
          'Missing %s ebuild in %s' % (WORKON_EBUILD_VERSION, path))
    return None

  path = os.path.dirname(unstable_ebuilds[0].ebuild_path)
  if len(unstable_ebuilds) > 1:
    cros_build_lib.Die('Found multiple unstable ebuilds in %s' % path)

  if not stable_ebuilds:
    logging.warning('Missing stable ebuild in %s' % path)
    return unstable_ebuilds[0]

  if len(stable_ebuilds) == 1:
    return stable_ebuilds[0]

  stable_versions = set(ebuild.version_no_rev for ebuild in stable_ebuilds)
  if len(stable_versions) > 1:
    package = stable_ebuilds[0].package
    message = 'Found multiple stable ebuild versions in %s:' % path
    for version in stable_versions:
      message += '\n    %s-%s' % (package, version)
    cros_build_lib.Die(message)

  uprev_ebuild = max(stable_ebuilds, key=lambda eb: eb.current_revision)
  for ebuild in stable_ebuilds:
    if ebuild != uprev_ebuild:
      logging.warning('Ignoring stable ebuild revision %s in %s' %
                      (ebuild.version, path))
  return uprev_ebuild


def GetOverlayEBuilds(overlay, use_all, packages, allow_blacklisted=False):
  """Get ebuilds from the specified overlay.

  Args:
    overlay: The path of the overlay to get ebuilds.
    use_all: Whether to include all ebuilds in the specified directories.
      If true, then we gather all packages in the directories regardless
      of whether they are in our set of packages.
    packages: A set of the packages we want to gather.  If use_all is
      True, this argument is ignored, and should be None.
    allow_blacklisted: Whether or not to consider blacklisted ebuilds.

  Returns:
    A list of ebuilds of the overlay
  """
  ebuilds = []
  for package_dir, _dirs, files in os.walk(overlay):
    # If we were given a list of packages to uprev, only consider the files
    # whose potential CP match.
    # This allows us to uprev specific blacklisted without throwing errors on
    # every badly formatted blacklisted ebuild.
    package_name = os.path.basename(package_dir)
    category = os.path.basename(os.path.dirname(package_dir))

    # If the --all option isn't used, we only want to update packages that
    # are in packages.
    if not (use_all or os.path.join(category, package_name) in packages):
      continue

    paths = [os.path.join(package_dir, path) for path in files]
    ebuild = _FindUprevCandidates(paths, allow_blacklisted)

    # Add stable ebuild.
    if ebuild:
      ebuilds.append(ebuild)

  return ebuilds


def RegenCache(overlay):
  """Regenerate the cache of the specified overlay.

  Args:
    overlay: The tree to regenerate the cache for.
  """
  repo_name = GetOverlayName(overlay)
  if not repo_name:
    return

  layout = cros_build_lib.LoadKeyValueFile(
      os.path.join(GetOverlayRoot(overlay), 'metadata', 'layout.conf'),
      ignore_missing=True)
  if layout.get('cache-format') != 'md5-dict':
    return

  # Regen for the whole repo.
  cros_build_lib.RunCommand(['egencache', '--update', '--repo', repo_name,
                             '--jobs', str(multiprocessing.cpu_count())],
                            cwd=overlay, enter_chroot=True)
  # If there was nothing new generated, then let's just bail.
  result = git.RunGit(overlay, ['status', '-s', 'metadata/'])
  if not result.output:
    return
  # Explicitly add any new files to the index.
  git.RunGit(overlay, ['add', 'metadata/'])
  # Explicitly tell git to also include rm-ed files.
  git.RunGit(overlay, ['commit', '-m', 'regen cache', 'metadata/'])


def ParseBashArray(value):
  """Parse a valid bash array into python list."""
  # The syntax for bash arrays is nontrivial, so let's use bash to do the
  # heavy lifting for us.
  sep = ','
  # Because %s may contain bash comments (#), put a clever newline in the way.
  cmd = 'ARR=%s\nIFS=%s; echo -n "${ARR[*]}"' % (value, sep)
  return cros_build_lib.RunCommand(
      cmd, print_cmd=False, shell=True, capture_output=True).output.split(sep)


def WorkonEBuildGeneratorForDirectory(base_dir):
  """Yields cros_workon EBuilds in |base_dir|.

  Args:
    base_dir: Path to the base directory.

  Yields:
    A cros_workon EBuild instance.
  """
  for root, _, files in os.walk(base_dir):
    for filename in files:
      # Only look at *-9999.ebuild files.
      if filename.endswith(WORKON_EBUILD_SUFFIX):
        full_path = os.path.join(root, filename)
        ebuild = EBuild(full_path)
        if not ebuild.is_workon:
          continue
        yield ebuild


def WorkonEBuildGenerator(buildroot, overlay_type):
  """Scans all overlays and yields cros_workon EBuilds.

  Args:
    buildroot: Path to source root to find overlays.
    overlay_type: The type of overlay to use (one of
      constants.VALID_OVERLAYS).

  Yields:
    A cros_workon EBuild instance.
  """
  # Get the list of all overlays.
  overlays = FindOverlays(overlay_type, buildroot=buildroot)
  # Iterate through overlays and gather all workon ebuilds
  for overlay in overlays:
    for ebuild in WorkonEBuildGeneratorForDirectory(overlay):
      yield ebuild


def BuildFullWorkonPackageDictionary(buildroot, overlay_type, manifest):
  """Scans all cros_workon ebuilds and build a dictionary.

  Args:
    buildroot: Path to source root to find overlays.
    overlay_type: The type of overlay to use (one of
      constants.VALID_OVERLAYS).
    manifest: git.ManifestCheckout object.

  Returns:
    A dictionary mapping (project, branch) to a list of packages.
    E.g., {('chromiumos/third_party/kernel', 'chromeos-3.14'):
           ['sys-kernel/chromeos-kernel-3_14']}.
  """
  # we want (project, branch) -> package (CP or P?)
  directory_src = os.path.join(buildroot, 'src')

  pkg_map = dict()
  for ebuild in WorkonEBuildGenerator(buildroot, overlay_type):
    if ebuild.is_blacklisted:
      continue
    package = ebuild.package
    paths = ebuild.GetSourceInfo(directory_src, manifest).srcdirs
    for path in paths:
      checkout = manifest.FindCheckoutFromPath(path)
      project = checkout['name']
      branch = git.StripRefs(checkout['tracking_branch'])
      pkg_list = pkg_map.get((project, branch), [])
      pkg_list.append(package)
      pkg_map[(project, branch)] = pkg_list

  return pkg_map


def GetWorkonProjectMap(overlay, subdirectories):
  """Get a mapping of cros_workon ebuilds to projects and source paths.

  Args:
    overlay: Overlay to look at.
    subdirectories: List of subdirectories to look in on the overlay.

  Yields:
    Tuples containing (filename, projects, srcpaths) for cros-workon ebuilds in
    the given overlay under the given subdirectories.
  """
  # Search ebuilds for project names, ignoring non-existent directories.
  # Also filter out ebuilds which are not cros_workon.
  for subdir in subdirectories:
    base_dir = os.path.join(overlay, subdir)
    for ebuild in WorkonEBuildGeneratorForDirectory(base_dir):
      full_path = ebuild.ebuild_path
      workon_vars = EBuild.GetCrosWorkonVars(full_path, ebuild.pkgname)
      relpath = os.path.relpath(full_path, start=overlay)
      yield relpath, workon_vars.project, workon_vars.srcpath


def EbuildToCP(path):
  """Return the category/path string from an ebuild path.

  Args:
    path: Path to an ebuild.

  Returns:
    '$CATEGORY/$PN' (e.g. 'sys-apps/dbus')
  """
  return os.path.join(*SplitEbuildPath(path)[0:2])


def SplitEbuildPath(path):
  """Split an ebuild path into its components.

  Given a specified ebuild filename, returns $CATEGORY, $PN, $P. It does not
  perform any check on ebuild name elements or their validity, merely splits
  a filename, absolute or relative, and returns the last 3 components.

  Example: For /any/path/chromeos-base/power_manager/power_manager-9999.ebuild,
  returns ('chromeos-base', 'power_manager', 'power_manager-9999').

  Args:
    path: Path to the ebuild.

  Returns:
    $CATEGORY, $PN, $P
  """
  return os.path.splitext(path)[0].rsplit('/', 3)[-3:]


def SplitPV(pv, strict=True):
  """Takes a PV value and splits it into individual components.

  Args:
    pv: Package name and version.
    strict: If True, returns None if version or package name is missing.
      Otherwise, only package name is mandatory.

  Returns:
    A collection with named members:
      pv, package, version, version_no_rev, rev
  """
  m = _pvr_re.match(pv)

  if m is None and strict:
    return None

  if m is None:
    return PV(**{'pv': None, 'package': pv, 'version': None,
                 'version_no_rev': None, 'rev': None})

  return PV(**m.groupdict())


def SplitCPV(cpv, strict=True):
  """Splits a CPV value into components.

  Args:
    cpv: Category, package name, and version of a package.
    strict: If True, returns None if any of the components is missing.
      Otherwise, only package name is mandatory.

  Returns:
    A collection with named members:
      category, pv, package, version, version_no_rev, rev
  """
  chunks = cpv.split('/')
  if len(chunks) > 2:
    raise ValueError('Unexpected package format %s' % cpv)
  if len(chunks) == 1:
    category = None
  else:
    category = chunks[0]

  m = SplitPV(chunks[-1], strict=strict)
  if strict and (category is None or m is None):
    return None
  return CPV(category=category, **m._asdict())


def FindWorkonProjects(packages):
  """Find the projects associated with the specified cros_workon packages.

  Args:
    packages: List of cros_workon packages.

  Returns:
    The set of projects associated with the specified cros_workon packages.
  """
  all_projects = set()
  buildroot, both = constants.SOURCE_ROOT, constants.BOTH_OVERLAYS
  for overlay in FindOverlays(both, buildroot=buildroot):
    for _, projects, _ in GetWorkonProjectMap(overlay, packages):
      all_projects.update(projects)
  return all_projects


def ListInstalledPackages(sysroot):
  """[DEPRECATED] Lists all portage packages in a given portage-managed root.

  Assumes the existence of a /var/db/pkg package database.

  This function is DEPRECATED, please use PortageDB.InstalledPackages instead.

  Args:
    sysroot: The root directory being inspected.

  Returns:
    A list of (cp,v) tuples in the given sysroot.
  """
  return [('%s/%s' % (pkg.category, pkg.package), pkg.version)
          for pkg in PortageDB(sysroot).InstalledPackages()]


def BestVisible(atom, board=None, pkg_type='ebuild',
                buildroot=constants.SOURCE_ROOT):
  """Get the best visible ebuild CPV for the given atom.

  Args:
    atom: Portage atom.
    board: Board to look at. By default, look in chroot.
    pkg_type: Package type (ebuild, binary, or installed).
    buildroot: Directory

  Returns:
    A CPV object.
  """
  portageq = 'portageq' if board is None else 'portageq-%s' % board
  root = cros_build_lib.GetSysroot(board=board)
  cmd = [portageq, 'best_visible', root, pkg_type, atom]
  result = cros_build_lib.RunCommand(
      cmd, cwd=buildroot, enter_chroot=True, debug_level=logging.DEBUG,
      capture_output=True)
  return SplitCPV(result.output.strip())


def IsPackageInstalled(package, sysroot='/'):
  """Return whether a portage package is in a given portage-managed root.

  Args:
    package: The CP to look for.
    sysroot: The root being inspected.
  """
  for key, _version in ListInstalledPackages(sysroot):
    if key == package:
      return True

  return False


def FindPackageNameMatches(pkg_str, board=None,
                           buildroot=constants.SOURCE_ROOT):
  """Finds a list of installed packages matching |pkg_str|.

  Args:
    pkg_str: The package name with optional category, version, and slot.
    board: The board to insepct.
    buildroot: Source root to find overlays.

  Returns:
    A list of matched CPV objects.
  """
  cmd = ['equery']
  if board:
    cmd = ['equery-%s' % board]

  cmd += ['list', pkg_str]
  result = cros_build_lib.RunCommand(
      cmd, cwd=buildroot, enter_chroot=True, capture_output=True,
      error_code_ok=True)

  matches = []
  if result.returncode == 0:
    matches = [SplitCPV(x) for x in result.output.splitlines()]

  return matches


def FindEbuildForBoardPackage(pkg_str, board):
  """Returns a path to an ebuild for a particular board."""
  equery = 'equery-%s' % board
  cmd = [equery, 'which', pkg_str]
  return cros_build_lib.RunCommand(
      cmd, cwd=constants.SOURCE_ROOT, enter_chroot=True,
      capture_output=True).output.strip()


def FindEbuildForPackage(pkg_str, sysroot, include_masked=False,
                         extra_env=None, error_code_ok=True):
  """Returns a path to an ebuild responsible for package matching |pkg_str|.

  Args:
    pkg_str: The package name with optional category, version, and slot.
    sysroot: The root directory being inspected.
    include_masked: True iff we should include masked ebuilds in our query.
    extra_env: optional dictionary of extra string/string pairs to use as the
      environment of equery command.
    error_code_ok: If true, do not raise an exception when RunCommand returns
      a non-zero exit code. Instead, return the CommandResult object containing
      the exit code.

  Returns:
    Path to ebuild for this package.
  """
  cmd = [cros_build_lib.GetSysrootToolPath(sysroot, 'equery'), 'which']
  if include_masked:
    cmd += ['--include-masked']
  cmd += [pkg_str]

  result = cros_build_lib.RunCommand(cmd, extra_env=extra_env, print_cmd=False,
                                     capture_output=True,
                                     error_code_ok=error_code_ok)

  if result.returncode:
    return None
  return result.output.strip()


def GetInstalledPackageUseFlags(pkg_str, board=None):
  """Gets the list of USE flags for installed packages matching |pkg_str|.

  Args:
    pkg_str: The package name with optional category, version, and slot.
    board: The board to inspect.

  Returns:
    A dictionary with the key being a package CP and the value being the list
    of USE flags for that package.
  """
  cmd = ['qlist']
  if board:
    cmd = ['qlist-%s' % board]

  cmd += ['-CqU', pkg_str]
  result = cros_build_lib.RunCommand(
      cmd, enter_chroot=True, capture_output=True, error_code_ok=True)

  use_flags = {}
  if result.returncode == 0:
    for line in result.output.splitlines():
      tokens = line.split()
      use_flags[tokens[0]] = tokens[1:]

  return use_flags


def GetBinaryPackageDir(sysroot='/', packages_dir=None):
  """Returns the binary package directory of |sysroot|."""
  dir_name = packages_dir if packages_dir else 'packages'
  return os.path.join(sysroot, dir_name)


def GetBinaryPackagePath(c, p, v, sysroot='/', packages_dir=None):
  """Returns the path to the binary package.

  Args:
    c: category.
    p: package.
    v: version.
    sysroot: The root being inspected.
    packages_dir: Name of the packages directory in |sysroot|.

  Returns:
    The path to the binary package.
  """
  pkgdir = GetBinaryPackageDir(sysroot=sysroot, packages_dir=packages_dir)
  path = os.path.join(pkgdir, c, '%s-%s.tbz2' % (p, v))
  if not os.path.exists(path):
    raise ValueError('Cannot find the binary package %s!' % path)

  return path


def GetBoardUseFlags(board):
  """Returns a list of USE flags in effect for a board."""
  portageq = 'portageq-%s' % board
  cmd = [portageq, 'envvar', 'USE']
  return cros_build_lib.RunCommand(
      cmd, cwd=constants.SOURCE_ROOT, enter_chroot=True,
      capture_output=True).output.split()


def GetPackageDependencies(board, package):
  """Returns the depgraph list of packages for a board and package."""
  emerge = 'emerge-%s' % board
  cmd = [emerge, '-p', '--cols', '--quiet', '--root', '/var/empty', '-e',
         package]
  emerge_output = cros_build_lib.RunCommand(
      cmd, cwd=constants.SOURCE_ROOT, enter_chroot=True,
      capture_output=True).output.splitlines()
  packages = []
  for line in emerge_output:
    columns = line.split()
    try:
      package = columns[1] + '-' + columns[2]
      packages.append(package)
    except IndexError:
      logging.error('Wrong format of output: \n%r', emerge_output)
      raise

  return packages


def GetFullAndroidPortagePackageName(android_package_name):
  """Returns the full portage pacakge name for the given android package.

  Args:
    android_package_name: Android package name. E.g. android-container.

  Returns:
    Full portage package name. E.g. chromeos-base/android-container.
  """
  return '%s/%s' % (constants.CHROMEOS_BASE, android_package_name)


def GetRepositoryFromEbuildInfo(info):
  """Parse output of the result of `ebuild <ebuild_path> info`

  This command should return output that looks a lot like:
   CROS_WORKON_SRCDIR=("/mnt/host/source/src/platform2")
   CROS_WORKON_PROJECT=("chromiumos/platform2")
  """
  srcdir_match = re.search(r'^CROS_WORKON_SRCDIR=(\(".*"\))$',
                           info, re.MULTILINE)
  project_match = re.search(r'^CROS_WORKON_PROJECT=(\(".*"\))$',
                            info, re.MULTILINE)
  if not srcdir_match or not project_match:
    return None

  srcdirs = ParseBashArray(srcdir_match.group(1))
  projects = ParseBashArray(project_match.group(1))
  if len(srcdirs) != len(projects):
    return None

  return [RepositoryInfoTuple(srcdir, project)
          for srcdir, project in zip(srcdirs, projects)]


def GetRepositoryForEbuild(ebuild_path, sysroot):
  """Get parsed output of `ebuild <ebuild_path> info`

  ebuild ... info runs the pkg_info step of an ebuild.
  cros-workon.eclass defines that step and prints both variables.

  Args:
    ebuild_path: string full path to ebuild file.
    sysroot: The root directory being inspected.

  Returns:
    list of RepositoryInfoTuples.
  """
  cmd = (cros_build_lib.GetSysrootToolPath(sysroot, 'ebuild'),
         ebuild_path, 'info')
  result = cros_build_lib.RunCommand(
      cmd, capture_output=True, print_cmd=False, error_code_ok=True)
  return GetRepositoryFromEbuildInfo(result.output)


def CleanOutdatedBinaryPackages(sysroot):
  """Cleans outdated binary packages from |sysroot|."""
  return cros_build_lib.RunCommand(
      [cros_build_lib.GetSysrootToolPath(sysroot, 'eclean'), '-d', 'packages'])


def _CheckHasTest(cp, sysroot):
  """Checks if the ebuild for |cp| has tests.

  Args:
    cp: A portage package in the form category/package_name.
    sysroot: Path to the sysroot.

  Returns:
    |cp| if the ebuild for |cp| defines a test stanza, None otherwise.

  Raises:
    raise failures_lib.PackageBuildFailure if FindEbuildForPackage
    raises a RunCommandError
  """
  try:
    path = FindEbuildForPackage(cp, sysroot, error_code_ok=False)
  except cros_build_lib.RunCommandError as e:
    logging.error('FindEbuildForPackage error %s', e)
    raise failures_lib.PackageBuildFailure(e, 'equery', cp)
  ebuild = EBuild(path)
  return cp if ebuild.has_test else None


def PackagesWithTest(sysroot, packages):
  """Returns the subset of |packages| that have unit tests.

  Args:
    sysroot: Path to the sysroot.
    packages: List of packages to filter.

  Returns:
    The subset of |packages| that defines unit tests.
  """
  inputs = [(cp, sysroot) for cp in packages]
  pkg_with_test = set(parallel.RunTasksInProcessPool(_CheckHasTest, inputs))

  # CheckHasTest will return None for packages that do not have tests. We can
  # discard that value.
  pkg_with_test.discard(None)
  return pkg_with_test
