# -*- coding: utf-8 -*-
# Copyright (c) 2011-2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that handles the processing of patches to the source tree."""

from __future__ import print_function

import calendar
import collections
import os
import random
import re
import time

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import gob_util
from chromite.lib import metrics


# We import mock so that we can identify mock.MagicMock instances in tests
# that use mock.
try:
  import mock
except ImportError:
  mock = None


site_config = config_lib.GetConfig()


_MAXIMUM_GERRIT_NUMBER_LENGTH = 7
_GERRIT_CHANGE_ID_PREFIX = 'I'
_GERRIT_CHANGE_ID_LENGTH = 40
_GERRIT_CHANGE_ID_TOTAL_LENGTH = (_GERRIT_CHANGE_ID_LENGTH +
                                  len(_GERRIT_CHANGE_ID_PREFIX))
REPO_NAME_RE = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_\-]*(/[a-zA-Z0-9_-]+)*$')
BRANCH_NAME_RE = re.compile(r'^(refs/heads/)?[a-zA-Z0-9_][a-zA-Z0-9_\-]*$')

DEFAULT_GET_ROOT_ERROR_LIMIT = 100

# Constants for attributes names.
ATTR_REMOTE = 'remote'
ATTR_GERRIT_NUMBER = 'gerrit_number'
ATTR_PROJECT = 'project'
ATTR_BRANCH = 'branch'
ATTR_PROJECT_URL = 'project_url'
ATTR_REF = 'ref'
ATTR_CHANGE_ID = 'change_id'
ATTR_COMMIT = 'commit'
ATTR_PATCH_NUMBER = 'patch_number'
ATTR_OWNER_EMAIL = 'owner_email'
ATTR_FAIL_COUNT = 'fail_count'
ATTR_PASS_COUNT = 'pass_count'
ATTR_TOTAL_FAIL_COUNT = 'total_fail_count'
ATTR_COMMIT_MESSAGE = 'commit_message'

ALL_ATTRS = (
    ATTR_REMOTE,
    ATTR_GERRIT_NUMBER,
    ATTR_PROJECT,
    ATTR_BRANCH,
    ATTR_PROJECT_URL,
    ATTR_REF,
    ATTR_CHANGE_ID,
    ATTR_COMMIT,
    ATTR_PATCH_NUMBER,
    ATTR_OWNER_EMAIL,
    ATTR_FAIL_COUNT,
    ATTR_PASS_COUNT,
    ATTR_TOTAL_FAIL_COUNT,
    ATTR_COMMIT_MESSAGE,
)

def ParseSHA1(text, error_ok=True):
  """Checks if |text| conforms to the SHA1 format and parses it.

  Args:
    text: The string to check.
    error_ok: If set, do not raise an exception if |text| is not a
      valid SHA1.

  Returns:
    If |text| is a valid SHA1, returns |text|.  Otherwise,
    returns None when |error_ok| is set and raises an exception when
    |error_ok| is False.
  """
  valid = git.IsSHA1(text)
  if not error_ok and not valid:
    raise ValueError('%s is not a valid SHA1', text)

  return text if valid else None


def ParseGerritNumber(text, error_ok=True):
  """Checks if |text| conforms to the Gerrit number format and parses it.

  Args:
    text: The string to check.
    error_ok: If set, do not raise an exception if |text| is not a
      valid Gerrit number.

  Returns:
    If |text| is a valid Gerrit number, returns |text|.  Otherwise,
    returns None when |error_ok| is set and raises an exception when
    |error_ok| is False.
  """
  valid = text.isdigit() and len(text) <= _MAXIMUM_GERRIT_NUMBER_LENGTH
  if not error_ok and not valid:
    raise ValueError('%s is not a valid Gerrit number', text)

  return text if valid else None


def ParseChangeID(text, error_ok=True):
  """Checks if |text| conforms to the change-ID format and parses it.

  Change-ID is a string that starts with I/i. E.g.
    I47ea30385af60ae4cc2acc5d1a283a46423bc6e1

  Args:
    text: The string to check.
    error_ok: If set, do not raise an exception if |text| is not a
      valid change-ID.

  Returns:
    If |text| is a valid change-ID, returns |text|.  Otherwise,
    returns None when |error_ok| is set and raises an exception when
    |error_ok| is False.
  """
  valid = (text.startswith(_GERRIT_CHANGE_ID_PREFIX) and
           len(text) == _GERRIT_CHANGE_ID_TOTAL_LENGTH and
           git.IsSHA1(text[len(_GERRIT_CHANGE_ID_PREFIX):].lower()))

  if not error_ok and not valid:
    raise ValueError('%s is not a valid change-ID', text)

  return text if valid else None


FullChangeId = collections.namedtuple(
    'FullChangeId', ('project', 'branch', 'change_id'))


def ParseFullChangeID(text, error_ok=True):
  """Checks if |text| conforms to the full change-ID format and parses it.

  Full change-ID format: project~branch~change-id. E.g.
    chromiumos/chromite~master~I47ea30385af60ae4cc2acc5d1a283a46423bc6e1

  Args:
    text: The string to check.
    error_ok: If set, do not raise an exception if |text| is not a
      valid full change-ID.

  Returns:
    If |text| is a valid full change-ID, returns (project, branch,
    change_id).  Otherwise, returns None when |error_ok| is set and
    raises an exception when |error_ok| is False.
  """
  fields = text.split('~')
  if not len(fields) == 3:
    if not error_ok:
      raise ValueError('%s is not a valid full change-ID', text)

    return None

  project, branch, change_id = fields
  if (not REPO_NAME_RE.match(project) or
      not BRANCH_NAME_RE.match(branch) or
      not ParseChangeID(change_id)):
    if not error_ok:
      raise ValueError('%s is not a valid full change-ID', text)

    return None

  return FullChangeId(project, branch, change_id)


class PatchException(Exception):
  """Base exception class all patch exception derive from."""

  # Unless instances override it, default all exceptions to ToT.
  inflight = False

  def __init__(self, patch, message=None):
    if (not isinstance(patch, GitRepoPatch) and
        not (mock and isinstance(patch, mock.MagicMock))):
      raise TypeError(
          'Patch must be a GitRepoPatch derivative; got type %s: %r'
          % (type(patch), patch))
    Exception.__init__(self)
    self.patch = patch
    self.message = message
    self.args = (patch,)
    if message is not None:
      self.args += (message,)

  def ShortExplanation(self):
    """Print a short explanation of why the patch failed.

    Explanations here should be suitable for inclusion in a sentence
    starting with the CL number. This is useful for writing nice error
    messages about dependency errors.
    """
    return 'failed: %s' % (self.message,)

  def __str__(self):
    return '%s %s' % (self.patch.PatchLink(), self.ShortExplanation())


class ApplyPatchException(PatchException):
  """Exception thrown if we fail to apply a patch."""

  def __init__(self, patch, message=None, inflight=False, trivial=False,
               files=()):
    PatchException.__init__(self, patch, message=message)
    self.inflight = inflight
    self.trivial = trivial
    self.files = files = tuple(files)
    # Reset args; else serialization can break.
    self.args = (patch, message, inflight, trivial, files)

  def _StringifyInflight(self):
    return 'the current patch series' if self.inflight else 'ToT'

  def _StringifyFilenames(self):
    """Stringify our list of filenames for presentation in Gerrit."""
    # Prefix each filename with a hyphen so that Gerrit will format it as an
    # unordered list.
    return '\n\n'.join('- %s' % x for x in self.files)

  def ShortExplanation(self):
    s = 'conflicted with %s' % (self._StringifyInflight(),)
    if self.trivial:
      s += (' because file content merging is disabled for this '
            'project.')
    else:
      s += '.'
    if self.files:
      s += ('\n\nThe conflicting files are amongst:\n\n'
            '%s' % (self._StringifyFilenames(),))
    if self.message:
      s += '\n\n%s' % (self.message,)
    return s


class EbuildConflict(ApplyPatchException):
  """Exception thrown if two CLs delete the same ebuild."""

  def __init__(self, patch, inflight, ebuilds):
    ApplyPatchException.__init__(self, patch, inflight=inflight, files=ebuilds)
    self.args = (patch, inflight, ebuilds)

  def ShortExplanation(self):
    return ('deletes an ebuild that is not present anymore. For this reason, '
            'we refuse to submit your change.\n\n'
            'When you rebase your change, please take into account that the '
            'following ebuilds have been uprevved or deleted:\n\n'
            '%s' % (self._StringifyFilenames()))


class ForbiddenMerge(PatchException):
  """Thrown if a merge commit doesn't meet criteria to be handled."""


class NonMainlineMerge(ForbiddenMerge):
  """Thrown in a merge commit has no parents that are already in mainline."""

  def __init__(self, patch):
    msg = ('Neither parent of this merge commit is already submitted in '
           'the destination branch. The CQ can only handle merge commits '
           'that meet this criteria.')
    super(NonMainlineMerge, self).__init__(patch, message=msg)


class PatchNoParents(PatchException):
  """Thrown when attempting to handle a patch with no parents."""

  def __init__(self, patch):
    msg = 'This patch has no parents, and therefore cannot be applied.'
    super(PatchNoParents, self).__init__(patch, message=msg)


class PatchIsEmpty(ApplyPatchException):
  """Exception thrown if we try to apply an empty patch"""

  def ShortExplanation(self):
    return 'had no changes after rebasing to %s.' % (
        self._StringifyInflight(),)


class DependencyError(PatchException):
  """Thrown when a change cannot be applied due to a failure in a dependency."""

  def __init__(self, patch, error):
    """Initialize the error object.

    Args:
      patch: The GitRepoPatch instance that this exception concerns.
      error: A PatchException object that can be stringified to describe
        the error.
    """
    PatchException.__init__(self, patch)
    self.inflight = error.inflight
    self.error = error
    self.args = (patch, error)

  def ShortExplanation(self):
    link = self.error.patch.PatchLink()
    return 'depends on %s, which %s' % (link, self.error.ShortExplanation())

  def GetRootError(self, limit=DEFAULT_GET_ROOT_ERROR_LIMIT):
    """Get the root error of nested dependency errors.

    DependencyError can be nested dependency errors on a root error, this
    method returns the exception of the root change. For example, for the error
    'CL:A depends on CL:B, which depends on CL:C, which was not eligible
    (wrong manifest branch, wrong labels, or otherwise filtered from eligible
    set).', this method will return the not_eligible_error of CL:C.
    DependencyErrors won't be formed by circular dependency errors, add the
    depth check just in case dependency errors are malformed.

    Args:
      limit: The limit (int) of depth to get the root error.

    Returns:
      The root error of the nested dependency errors; None if the depth exceeds
      the given limit.
    """
    depth = 1
    key_error = self.error
    while isinstance(key_error, DependencyError) and depth < limit:
      key_error = key_error.error

      depth += 1
      if depth == limit:
        return None

    return key_error

class BrokenCQDepends(PatchException):
  """Raised if a patch has a CQ-DEPEND line that is ill formated."""

  def __init__(self, patch, text, msg=None):
    PatchException.__init__(self, patch)
    self.text = text
    self.msg = msg
    self.args = (patch, text, msg)

  def ShortExplanation(self):
    s = 'has a malformed CQ-DEPEND target: %s' % (self.text,)
    if self.msg is not None:
      s += '; %s' % (self.msg,)
    return s


class BrokenChangeID(PatchException):
  """Raised if a patch has an invalid or missing Change-ID."""

  def __init__(self, patch, message, missing=False):
    PatchException.__init__(self, patch)
    self.message = message
    self.missing = missing
    self.args = (patch, message, missing)

  def ShortExplanation(self):
    return 'has a broken ChangeId: %s' % (self.message,)


class ChangeMatchesMultipleCheckouts(PatchException):
  """Raised if the given change matches multiple checkouts."""

  def ShortExplanation(self):
    return ('matches multiple checkouts. Does the manifest check out the '
            'same project and branch to different locations?')


class ChangeNotInManifest(PatchException):
  """Raised if we try to apply a not-in-manifest change."""

  def ShortExplanation(self):
    return 'could not be found in the repo manifest.'


class PatchNotSubmittable(PatchException):
  """Raised if a patch is not submittable."""

  def __init__(self, patch, reason):
    PatchException.__init__(self, patch)
    self.reason = str(reason)
    self.args = (patch, reason)

  def ShortExplanation(self):
    return self.reason


def MakeChangeId(unusable=False):
  """Create a random Change-Id.

  Args:
    unusable: If set to True, return a Change-Id like string that gerrit
      will explicitly fail on.  This is primarily used for internal ids,
      as a fallback when a Change-Id could not be parsed.
  """
  s = '%x' % (random.randint(0, 2 ** 160),)
  s = s.rjust(_GERRIT_CHANGE_ID_LENGTH, '0')
  if unusable:
    return 'Fake-ID %s' % s
  return '%s%s' % (_GERRIT_CHANGE_ID_PREFIX, s)


class PatchCache(object):
  """Dict-like object used for tracking a group of patches.

  This is usable both for existence checks against given string
  deps, and for change querying.
  """

  def __init__(self, initial=()):
    self._dict = {}
    self.Inject(*initial)

  def Inject(self, *args):
    """Inject a sequence of changes into this cache."""
    for change in args:
      self.InjectCustomKeys(change.LookupAliases(), change)

  def InjectCustomKeys(self, keys, change):
    """Inject a change w/ a list of keys.  Generally you want Inject instead.

    Args:
      keys: A list of keys to update.
      change: The change to update the keys to.
    """
    for key in keys:
      self._dict[str(key)] = change

  def _GetAliases(self, value):
    if hasattr(value, 'LookupAliases'):
      return value.LookupAliases()
    elif not isinstance(value, basestring):
      # This isn't needed in production code; it however is
      # rather useful to flush out bugs in test code.
      raise ValueError("Value %r isn't a string" % (value,))
    return [value]

  def Remove(self, *args):
    """Remove a change from this cache."""
    for change in args:
      for alias in self._GetAliases(change):
        self._dict.pop(alias, None)

  def __iter__(self):
    return iter(set(self._dict.itervalues()))

  def __getitem__(self, key):
    """If the given key exists, return the Change, else None."""
    for alias in self._GetAliases(key):
      val = self._dict.get(alias)
      if val is not None:
        return val
    return None

  def __contains__(self, key):
    return self[key] is not None

  def copy(self):
    """Return a copy of this cache."""
    return self.__class__(list(self))


def StripPrefix(text):
  """Strips the leading '*' for internal change names.

  Args:
    text: text to examine.

  Returns:
    A tuple of the corresponding remote and the stripped text.
  """
  remote = site_config.params.EXTERNAL_REMOTE
  prefix = site_config.params.INTERNAL_CHANGE_PREFIX
  if text.startswith(prefix):
    text = text[len(prefix):]
    remote = site_config.params.INTERNAL_REMOTE

  return remote, text


def AddPrefix(patch, text):
  """Add the leading '*' to |text| if applicable.

  Examines patch.remote and adds the prefix to text if applicable.

  Args:
    patch: A PatchQuery object to examine.
    text: The text to add prefix to.

  Returns:
    |text| with an added prefix for internal patches; otherwise, returns text.
  """
  return '%s%s' % (site_config.params.CHANGE_PREFIX[patch.remote], text)


def ParsePatchDep(text, no_change_id=False, no_sha1=False,
                  no_full_change_id=False, no_gerrit_number=False):
  """Parses a given patch dependency and convert it to a PatchQuery object.

  Parse user-given dependency (e.g. from the CQ-DEPEND line in the
  commit message) and returns a PatchQuery object with the relevant
  information of the dependency.

  Args:
    text: The text to parse.
    no_change_id: Do not allow change-ID.
    no_sha1: Do not allow SHA1.
    no_full_change_id: Do not allow full change-ID.
    no_gerrit_number: Do not allow gerrit_number.

  Returns:
    A PatchQuery object.
  """
  original_text = text
  if not text:
    raise ValueError('ParsePatchDep invoked with an empty value: %r'
                     % (text,))
  # Deal w/ CL: targets.
  if text.upper().startswith('CL:'):
    if not text.startswith('CL:'):
      raise ValueError(
          "ParsePatchDep: 'CL:' must be upper case: %r"
          % (original_text,))
    text = text[3:]

  # Strip the prefix to determine the remote.
  remote, text = StripPrefix(text)

  parsed = ParseFullChangeID(text)
  if parsed:
    if no_full_change_id:
      raise ValueError(
          'ParsePatchDep: Full Change-ID is not allowed: %r.' % original_text)

    return PatchQuery(remote, project=parsed.project,
                      tracking_branch=parsed.branch, change_id=parsed.change_id)

  parsed = ParseChangeID(text)
  if parsed:
    if no_change_id:
      raise ValueError(
          'ParsePatchDep: Change-ID is not allowed: %r.' % original_text)

    return PatchQuery(remote, change_id=parsed)

  parsed = ParseGerritNumber(text)
  if parsed:
    if no_gerrit_number:
      raise ValueError(
          'ParsePatchDep: Gerrit number is not allowed: %r.' % original_text)

    return PatchQuery(remote, gerrit_number=parsed)

  parsed = ParseSHA1(text)
  if parsed:
    if no_sha1:
      raise ValueError(
          'ParsePatchDep: SHA1 is not allowed: %r.' % original_text)

    return PatchQuery(remote, sha1=parsed)

  raise ValueError('Cannot parse the dependency: %s' % original_text)


def GetOptionLinesFromCommitMessage(commit_message, option_re):
  """Finds lines in |commit_message| that start with |option_re|.

  Args:
    commit_message: (str) Text of the commit message.
    option_re: (str) regular expression to match the key identifying this
               option. Additionally, any whitespace surrounding the option
               is ignored.

  Returns:
    list of line values that matched the option (with the option stripped
    out) if at least 1 line matched the option (even if it provided no
    valuse). None if no lines of the message matched the option.
  """
  option_lines = []
  matched = False
  lines = commit_message.splitlines()[2:]
  for line in lines:
    line = line.strip()
    if re.match(option_re, line):
      matched = True
      line = re.sub(option_re, '', line, count=1).strip()
      if line:
        option_lines.append(line)

  if matched:
    return option_lines
  else:
    return None


# TODO(akeshet): Refactor CQ-DEPEND parsing logic to use general purpose
# GetOptionFromCommitMessage.
def GetPaladinDeps(commit_message):
  """Get the paladin dependencies for the given |commit_message|."""
  PALADIN_DEPENDENCY_RE = re.compile(r'^([ \t]*CQ.?DEPEND.)(.*)$',
                                     re.MULTILINE | re.IGNORECASE)
  PATCH_RE = re.compile('[^, ]+')
  EXPECTED_PREFIX = 'CQ-DEPEND='
  matches = PALADIN_DEPENDENCY_RE.findall(commit_message)
  dependencies = []
  for prefix, match in matches:
    if prefix != EXPECTED_PREFIX:
      msg = 'Expected %r, but got %r' % (EXPECTED_PREFIX, prefix)
      raise ValueError(msg)
    for chunk in PATCH_RE.findall(match):
      chunk = ParsePatchDep(chunk, no_sha1=True)
      if chunk not in dependencies:
        dependencies.append(chunk)
  return dependencies


class PatchQuery(object):
  """Store information about a patch.

  This stores information about a patch used to query Gerrit and/or
  our internal PatchCache. It is mostly used to describe a patch
  dependency.

  It is is intended to match a single patch. If a user specified a
  non-full change id then it might match multiple patches. If a user
  specified an invalid change id then it might not match any patches.
  """

  def __init__(self, remote, project=None, tracking_branch=None, change_id=None,
               sha1=None, gerrit_number=None):
    """Initializes a PatchQuery instance.

    Args:
      remote: The remote git instance path, defined in constants.CROS_REMOTES.
      project: The name of the project that the patch applies to.
      tracking_branch: The remote branch of the project the patch applies to.
      change_id: The Gerrit Change-ID representing this patch.
      sha1: The sha1 of the commit. This *must* be accurate
      gerrit_number: The Gerrit number of the patch.
    """
    self.remote = remote
    self.tracking_branch = None
    if tracking_branch:
      self.tracking_branch = os.path.basename(tracking_branch)
    self.project = project
    self.sha1 = None if sha1 is None else ParseSHA1(sha1)
    self.tree_hash = None
    self.change_id = None if change_id is None else ParseChangeID(change_id)
    self.gerrit_number = (None if gerrit_number is None else
                          ParseGerritNumber(gerrit_number))
    self.id = self.full_change_id = None
    self._SetFullChangeID()
    # self.id is the only attribute with the internal prefix (*) if
    # applicable. All other atttributes are strictly external format.
    self._SetID()

  def _SetFullChangeID(self):
    """Set the unique full Change-ID if possible."""
    if (self.project is not None and
        self.tracking_branch is not None and
        self.change_id is not None):
      self.full_change_id = '%s~%s~%s' % (
          self.project, self.tracking_branch, self.change_id)

  def _SetID(self, override_value=None):
    """Set the unique ID to be used internally, if possible."""
    if override_value is not None:
      self.id = override_value
      return

    if not self.full_change_id:
      self._SetFullChangeID()

    if self.full_change_id:
      self.id = AddPrefix(self, self.full_change_id)

    elif self.sha1:
      # We assume sha1 is unique, but in rare cases (e.g. two branches with
      # the same history) it is not. We don't handle that.
      self.id = '%s%s' % (site_config.params.CHANGE_PREFIX[self.remote],
                          self.sha1)

  def LookupAliases(self):
    """Returns the list of lookup keys to query a PatchCache.

    Each key has to be unique for the patch. If no unique key can be
    generated yet (because of incomplete patch information), we'd
    rather return None to avoid retrieving incorrect patch from the
    cache.
    """
    l = []
    if self.gerrit_number:
      l.append(self.gerrit_number)

    # Note that change-ID alone is not unique. Use full change-id here.
    if self.full_change_id:
      l.append(self.full_change_id)

    # Note that in rare cases (two branches with the same history),
    # the commit hash may not be unique. We don't handle that.
    if self.sha1:
      l.append(self.sha1)

    return ['%s%s' % (site_config.params.CHANGE_PREFIX[self.remote], x)
            for x in l if x is not None]

  def ToGerritQueryText(self):
    """Generate a text used to query Gerrit.

    This text may not be unique because the lack of information from
    user-specified dependencies (crbug.com/354734). In which cases,
    the Gerrit query would fail.
    """
    # Try to return a unique ID if possible.
    if self.gerrit_number:
      return self.gerrit_number
    elif self.full_change_id:
      return self.full_change_id
    elif self.sha1:
      # SHA1 may not not be unique, but we don't handle that here.
      return self.sha1
    elif self.change_id:
      # Fall back to use Change-Id, which is not unique.
      return self.change_id
    else:
      # We cannot query without at least one of the three fields. A
      # special case is UploadedLocalPatch which has none of the
      # above, but also is not used for query.
      raise ValueError(
          'We do not have enough information to generate a Gerrit query. '
          'At least one of the following fields needs to be set: Change-Id, '
          'Gerrit number, or sha1')

  def __hash__(self):
    """Returns a hash to be used in a set or a list."""
    if self.id:
      return hash(self.id)
    else:
      return hash((self.remote, self.project, self.tracking_branch,
                   self.gerrit_number, self.change_id, self.sha1))

  def __eq__(self, other):
    """Defines when two PatchQuery objects are considered equal."""
    # We allow comparing against a string to make testing easier.
    if isinstance(other, basestring):
      return self.id == other

    if self.id is not None:
      return self.id == getattr(other, 'id', None)

    return ((self.remote, self.project, self.tracking_branch,
             self.gerrit_number, self.change_id, self.sha1) ==
            (other.remote, other.project, other.tracking_branch,
             other.gerrit_number, other.change_id, other.sha1))


class GitRepoPatch(PatchQuery):
  """Representing a patch from a branch of a local or remote git repository."""

  # Note the selective case insensitivity; gerrit allows only this.
  # TOOD(ferringb): back VALID_CHANGE_ID_RE down to {8,40}, requires
  # ensuring CQ's internals can do the translation (almost can now,
  # but will fail in the case of a CQ-DEPEND on a change w/in the
  # same pool).
  pattern = (r'^' + re.escape(_GERRIT_CHANGE_ID_PREFIX) + r'[0-9a-fA-F]{' +
             re.escape(str(_GERRIT_CHANGE_ID_LENGTH)) + r'}$')
  _STRICT_VALID_CHANGE_ID_RE = re.compile(pattern)
  _GIT_CHANGE_ID_RE = re.compile(r'^Change-Id:[\t ]*(\w+)\s*$',
                                 re.I | re.MULTILINE)

  def __init__(self, project_url, project, ref, tracking_branch, remote,
               sha1=None, change_id=None):
    """Initialization of abstract Patch class.

    Args:
      project_url: The url of the git repo (can be local or remote) to pull the
                   patch from.
      project: See PatchQuery for documentation.
      ref: The refspec to pull from the git repo.
      tracking_branch: See PatchQuery for documentation.
      remote: See PatchQuery for documentation.
      sha1: The sha1 of the commit, if known. This *must* be accurate.  Can
        be None if not yet known- in which case Fetch will update it.
      change_id: See PatchQuery for documentation.
    """
    super(GitRepoPatch, self).__init__(remote, project=project,
                                       tracking_branch=tracking_branch,
                                       change_id=change_id,
                                       sha1=sha1, gerrit_number=None)

    # git_remote_url is the url of the remote git repo that this patch
    # belongs to. Differs from project_url as that may point to a local
    # repo or a gerrit review repo.
    self.git_remote_url = '%s/%s' % (
        site_config.params.GIT_REMOTES.get(remote), project)
    self.project_url = project_url
    self._project = project
    self._tracking_branch = tracking_branch
    self._remote = remote
    self.commit_message = None
    self._subject_line = None
    self.ref = ref
    self._is_fetched = set()
    self._committer_email = None
    self._committer_name = None
    self._commit_message = None

  @property
  def commit_message(self):
    return self._commit_message

  @commit_message.setter
  def commit_message(self, value):
    self._commit_message = self._AddFooters(value) if value else value

  @property
  def internal(self):
    """Whether patch is to an internal cros project."""
    return self.remote == site_config.params.INTERNAL_REMOTE

  def _GetFooters(self, msg):
    """Get the Git footers of the specified commit message.

    Args:
      msg: A commit message

    Returns:
      The parsed footers from the commit message.  Footers are
      lines of the form 'key: value' and are at the end of the commit
      message in a separate paragraph.  We return a list of pairs like
      ('key', 'value').
    """
    footers = []
    data = re.split(r'\n{2,}', msg.rstrip('\n'))[-1]
    for line in data.splitlines():
      m = re.match(r'([A-Za-z0-9-]+): *(.*)', line.rstrip('\n'))
      if m:
        footers.append(m.groups())
    return footers

  def _AddFooters(self, msg):
    """Ensure that commit messages have a change ID.

    Args:
      msg: The commit message.

    Returns:
      The modified commit message with necessary Gerrit footers.
    """
    if not msg:
      msg = '<no commit message provided>'

    if msg[-1] != '\n':
      msg += '\n'

    # This function is adapted from the version in Gerrit:
    # goto/createCherryPickCommitMessage
    old_footers = self._GetFooters(msg)

    if not old_footers:
      # Doesn't end in a "Signed-off-by: ..." style line? Add another line
      # break to start a new paragraph for the reviewed-by tag lines.
      msg += '\n'

    # This replicates the behavior of
    # goto/createCherryPickCommitMessage, but can result in multiple
    # Change-Id footers.  We should consider changing this behavior.
    if ('Change-Id', self.change_id) not in old_footers and self.change_id:
      msg += 'Change-Id: %s\n' % self.change_id

    return msg

  def _PullData(self, rev, git_repo):
    """Returns info about a commit object in the local repository.

    Args:
      rev: The commit to find information about
      git_repo: The path of the local git repository.

    Returns:
      A 6-tuple of (sha1, tree_hash, commit subject, commit message,
      committer email, committer name).
    """
    f = '%H%x00%T%x00%s%x00%B%x00%ce%x00%cn'
    cmd = ['log', '--pretty=format:%s' % f, '-n1', rev]
    ret = git.RunGit(git_repo, cmd, error_code_ok=True)
    # TODO(phobbs): this should probably use a namedtuple...
    if ret.returncode != 0:
      return None, None, None, None, None, None
    output = ret.output.split('\0')
    if len(output) != 6:
      return None, None, None, None, None, None
    return [unicode(x.strip(), 'ascii', 'ignore') for x in output]

  def UpdateMetadataFromRepo(self, git_repo, sha1):
    """Update this this object's metadata given a sha1.

    This updates various internal fields such as the committer name and email,
    the commit message, tree hash, etc.

    Raises a PatchException if the found sha1 differs from self.sha1.

    Args:
      git_repo: The path to the git repository that this commit exists in.
      sha1: The sha1 of the commit.  If None, assumes it was just fetched and
        uses "FETCH_HEAD".

    Returns:
      The sha1 of the commit.
    """
    sha1 = sha1 or 'FETCH_HEAD'
    sha1, tree_hash, subject, msg, email, name = self._PullData(sha1, git_repo)
    sha1 = ParseSHA1(sha1, error_ok=False)

    if self.sha1 is not None and sha1 != self.sha1:
      # Even if we know the sha1, still do a sanity check to ensure we
      # actually just fetched it.
      raise PatchException(self,
                           'Patch %s specifies sha1 %s, yet in fetching from '
                           '%s we could not find that sha1.  Internal error '
                           'most likely.' % (self, self.sha1, self.ref))

    self._committer_email = email
    self._committer_name = name
    self.sha1 = sha1
    self.tree_hash = tree_hash
    self.commit_message = msg
    self._EnsureId(self.commit_message)
    self._subject_line = subject
    self._is_fetched.add(git_repo)
    return self.sha1

  def HasBeenFetched(self, git_repo):
    """Whether this patch has already exists locally in `git_repo`

    Args:
      git_repo: The git repository to fetch this patch into.

    Returns:
      If it exists, the sha1 of this patch in `git_repo`.
    """
    git_repo = os.path.normpath(git_repo)
    if git_repo in self._is_fetched:
      return self.sha1

    # See if we've already got the object.
    if self.sha1 is not None:
      return self._PullData(self.sha1, git_repo)[0]

  def Fetch(self, git_repo):
    """Fetch this patch into the given git repository.

    FETCH_HEAD is implicitly reset by this operation.  Additionally,
    if the sha1 of the patch was not yet known, it is pulled and stored
    on this object and the git_repo is updated w/ the requested git
    object.

    While doing so, we'll load the commit message and Change-Id if not
    already known.

    Finally, if the sha1 is known and it's already available in the target
    repository, this will skip the actual fetch operation (it's unneeded).

    Args:
      git_repo: The git repository to fetch this patch into.

    Returns:
      The sha1 of the patch.
    """
    sha1 = self.HasBeenFetched(git_repo)

    if sha1 is None:
      fields = {'project_url': self.project_url}
      metrics.Counter(constants.MON_GIT_FETCH_COUNT).increment(fields=fields)

      git.RunGit(git_repo, ['fetch', '-f', self.project_url, self.ref],
                 print_cmd=True)

    return self.UpdateMetadataFromRepo(git_repo, sha1=sha1 or self.sha1)

  def GetDiffStatus(self, git_repo):
    """Isolate the paths and modifications this patch induces.

    Note that detection of file renaming is explicitly turned off.
    This is intentional since the level of rename detection can vary
    by user configuration, and trying to have our code specify the
    minimum level is fairly messy from an API perspective.

    Args:
      git_repo: Git repository to operate upon.

    Returns:
      A dictionary of path -> modification_type tuples.  See
      `git log --help`, specifically the --diff-filter section for details.
    """

    self.Fetch(git_repo)

    try:
      lines = git.RunGit(git_repo, ['diff', '--no-renames', '--name-status',
                                    '%s^..%s' % (self.sha1, self.sha1)])
    except cros_build_lib.RunCommandError as e:
      # If we get a 128, that means git couldn't find the the parent of our
      # sha1- meaning we're the first commit in the repository (there is no
      # parent).
      if e.result.returncode != 128:
        raise
      return {}
    lines = lines.output.splitlines()
    return dict(line.split('\t', 1)[::-1] for line in lines)

  def _AmendCommitMessage(self, git_repo):
    """"Amend the commit and update our sha1 with the new commit."""
    git.RunGit(git_repo, ['commit', '--amend', '-m', self.commit_message])
    self.sha1 = ParseSHA1(self._PullData('HEAD', git_repo)[0], error_ok=False)

  # pylint: disable=unused-argument
  def Merge(self, git_repo, trivial=False, inflight=False, leave_dirty=False):
    """Attempts to merge the given rev into branch.

    Note: This method is intended to present the same interface as CherryPick.
    However, it's behavior is simpler and not all of the arguments actually
    do anything.

    Args:
      git_repo: The git repository to operate upon.
      trivial: [ignored]
      inflight: [ignored]
      leave_dirty: [ignored]

    Raises:
      A ApplyPatchException if the request couldn't be handled.
    """
    cmd = ['merge', self.sha1]

    # TODO(akeshet): Amend the original merge's commit message before merging
    # it.

    reset_target = 'HEAD'
    try:
      git.RunGit(git_repo, cmd, capture_output=False)
      reset_target = None
      return
    except cros_build_lib.RunCommandError:
      # TODO(akeshet): Add more specialized or grandular error handling.
      # TODO(akeshet): Use an exception class other than ApplyPatchException,
      # for merge commits.
      raise ApplyPatchException(self, 'Unable to merge this CL.')
    finally:
      if reset_target:
        git.RunGit(git_repo, ['reset', '--hard', reset_target])

  def CherryPick(self, git_repo, trivial=False, inflight=False,
                 leave_dirty=False):
    """Attempts to cherry-pick the given rev into branch.

    Args:
      git_repo: The git repository to operate upon.
      trivial: Only allow trivial merges when applying change.
      inflight: If true, changes are already applied in this branch.
      leave_dirty: If True, if a CherryPick fails leaves partial commit behind.

    Raises:
      A ApplyPatchException if the request couldn't be handled.
    """
    # Note the --ff; we do *not* want the sha1 to change unless it
    # has to.
    cmd = ['cherry-pick', '--strategy', 'resolve', '--ff']
    if trivial:
      cmd += ['-X', 'trivial']
    cmd.append(self.sha1)

    reset_target = None if leave_dirty else 'HEAD'
    try:
      git.RunGit(git_repo, cmd, capture_output=False)
      self._AmendCommitMessage(git_repo)
      reset_target = None
      return
    except cros_build_lib.RunCommandError as error:
      ret = error.result.returncode
      if ret not in (1, 2):
        logging.error('Unknown cherry-pick exit code %s; %s', ret, error)
        raise ApplyPatchException(
            self, inflight=inflight,
            message=('Unknown exit code %s returned from cherry-pick '
                     'command: %s' % (ret, error)))
      elif ret == 1:
        # This means merge resolution was fine, but there was content conflicts.
        # If there are no conflicts, then this is caused by the change already
        # being merged.
        result = git.RunGit(git_repo,
                            ['diff', '--name-only', '--diff-filter=U'])

        # Output is one line per filename.
        conflicts = result.output.splitlines()
        if not conflicts:
          # No conflicts means the git repo is in a pristine state.
          reset_target = None
          raise PatchIsEmpty(self, inflight=inflight)

        # Making it here means that it wasn't trivial, nor was it already
        # applied.
        assert not trivial
        raise ApplyPatchException(self, inflight=inflight, files=conflicts)

      # ret=2 handling, this deals w/ trivial conflicts; including figuring
      # out if it was trivial induced or not.
      if not trivial:
        logging.error('The git tree may be corrupted.')
        logging.error('If the git error is "unable to read tree", '
                      'please clean up this repo.')
        raise

      # Here's the kicker; trivial conflicts can mask content conflicts.
      # We would rather state if it's a content conflict since in solving the
      # content conflict, the trivial conflict is solved.  Thus this
      # second run, where we let the exception fly through if one occurs.
      # Note that a trivial conflict means the tree is unmodified; thus
      # no need for cleanup prior to this invocation.
      reset_target = None
      self.CherryPick(git_repo, trivial=False, inflight=inflight)
      # Since it succeeded, we need to rewind.
      reset_target = 'HEAD^'

      raise ApplyPatchException(self, trivial=True, inflight=inflight)
    finally:
      if reset_target is not None:
        git.RunGit(git_repo, ['reset', '--hard', reset_target],
                   error_code_ok=True)

  def Apply(self, git_repo, upstream, revision=None, trivial=False):
    """Apply patch into a standalone git repo.

    The git repo does not need to be part of a repo checkout.

    Args:
      git_repo: The git repository to operate upon.
      revision: Revision to attach the tracking branch to.
      upstream: The branch to base the patch on.
      trivial: Only allow trivial merges when applying change.
    """

    self.Fetch(git_repo)

    logging.info('Attempting to apply change %s', self)

    # If the patch branch exists use it, otherwise create it and switch to it.
    if git.DoesCommitExistInRepo(git_repo, constants.PATCH_BRANCH):
      git.RunGit(git_repo, ['checkout', '-f', constants.PATCH_BRANCH])
    else:
      git.RunGit(git_repo,
                 ['checkout', '-b', constants.PATCH_BRANCH, '-t', upstream])
      if revision:
        git.RunGit(git_repo, ['reset', '--hard', revision])

    # Figure out if we're inflight.  At this point, we assume that the branch
    # is checked out and rebased onto upstream.  If HEAD differs from upstream,
    # then there are already other patches that have been applied.
    upstream, head = [
        git.RunGit(git_repo, ['rev-list', '-n1', x]).output.strip()
        for x in (upstream, 'HEAD')]
    inflight = (head != upstream)

    self._FindEbuildConflicts(git_repo, upstream, inflight=inflight)

    parents = self._GetParents(git_repo)
    if len(parents) == 1:
      use_merge = False
    elif len(parents) == 2:
      self._ValidateMergeCommit(git_repo, upstream, parents)
      use_merge = True
    else:
      raise PatchNoParents(self)

    return self._ApplyHelper(git_repo, upstream, trivial, inflight, use_merge)

  def _ApplyHelper(self, git_repo, upstream, trivial, inflight,
                   use_merge=False):
    via = 'merge' if use_merge else 'cherry-pick'
    logging.info('Applying via %s.', via)

    do_apply = self.Merge if use_merge else self.CherryPick

    do_checkout = True
    try:
      do_apply(git_repo, trivial=trivial, inflight=inflight)
      do_checkout = False
      return
    except ApplyPatchException:
      if not inflight:
        raise
      git.RunGit(git_repo, ['checkout', '-f', '--detach', upstream])

      do_apply(git_repo, trivial=trivial, inflight=False)
      # Making it here means that it was an inflight issue; throw the original.
      raise
    finally:
      # Ensure we're on the correct branch on the way out.
      if do_checkout:
        git.RunGit(git_repo, ['checkout', '-f', constants.PATCH_BRANCH],
                   error_code_ok=True)

  # pylint: disable=protected-access
  def _ValidateMergeCommit(self, git_repo, upstream, parents):
    """If this patch is a merge commit, validate that it meets restrictions.

    Args:
      git_repo: The git repo to work in.
      upstream: Current sha1 of upstream branch.
      parents: List (length 2) of the two parents of this patch.

    Raises:
      ForbiddenMerge if the merge does not meet criteria.
    """
    # We do not support patches with a history like this:
    #
    # *   E [new merge patch being handled]
    # |\
    # | * D [branchline commit]
    # * | C [mainline commit, not yet submitted]
    # |/
    # | * B CURRENT_UPSTREAM, [mainline commit, submitted]
    # |/
    # *   A [common ancestor on mainline]
    #
    # because of potential complications if C is cherry-picked into mainline in
    # the same CQ run as we are attempting to merge E.
    #
    # We prevent this by limiting ourselves to handle E only if at least 1
    # parent is already in the history of CURRENT_UPSTREAM.

    upstream_as_patch = self._FromSha1(upstream)
    parent1 = self._FromSha1(parents[0])
    parent2 = self._FromSha1(parents[1])

    parent_is_ancestor = (parent1._IsAncestorOf(git_repo, upstream_as_patch) or
                          parent2._IsAncestorOf(git_repo, upstream_as_patch))

    if not parent_is_ancestor:
      raise NonMainlineMerge(self)

    # TODO(akeshet): We should also validate that the "branchline" commits are
    # not in the current validation pool, otherwise we could end up with CLs.
    # being duplicated if the CQ both cherry-picks them and brings them in via
    # merge.
    #
    # The user instructions for the merge feature will make it clear that
    # "branchline" CLs should not be reviewed or CQ'd in the same way, but it
    # shouldn't be too hard to add a check here.

  def _FromSha1(self, sha1):
    """Return a new GitRepoPatch instance with same upstream, for other sha1.

    This is a useful helper method to convert sha1 values into GitRepoPatch
    objects if needed, to make use of the GitRepoPatch methods.
    """
    return GitRepoPatch(self.project_url, self._project, self.ref,
                        self._tracking_branch, self._remote, sha1=sha1)

  def ApplyAgainstManifest(self, manifest, trivial=False):
    """Applies the patch against the specified manifest.

      manifest: A ManifestCheckout object which is used to discern which
        git repo to patch, what the upstream branch should be, etc.
      trivial:  Only allow trivial merges when applying change.

    Raises:
      ApplyPatchException: If the patch failed to apply.
    """
    checkout = self.GetCheckout(manifest)
    revision = checkout.get('revision')
    # revision might be a branch which is written as it would appear on the
    # remote. If so, rewrite it as a local reference to the remote branch.
    # For example, refs/heads/master might become refs/remotes/cros/master.
    if revision and not git.IsSHA1(revision):
      revision = 'refs/remotes/%s/%s' % \
          (checkout['remote'], git.StripRefs(revision))
    upstream = checkout['tracking_branch']
    self.Apply(checkout.GetPath(absolute=True), upstream, revision=revision,
               trivial=trivial)

  def GerritDependencies(self):
    """Returns a list of Gerrit change numbers that this patch depends on.

    Ordinary patches have no Gerrit-style dependencies since they're not
    from Gerrit at all. See GerritPatch.GerritDependencies instead.
    """
    return []

  def _EnsureId(self, commit_message):
    """Ensure we have a usable Change-Id.

    This will parse the Change-Id out of the given commit message;
    if it cannot find one, it logs a warning and creates a fake ID.

    By its nature, that fake ID is useless- it's created to simplify
    API usage for patch consumers. If CQ were to see and try operating
    on one of these, it would fail for example.
    """
    if self.id is not None:
      return self.id

    try:
      self.change_id = self._ParseChangeId(commit_message)
    except BrokenChangeID:
      logging.warning(
          'Change %s, sha1 %s lacks a change-id in its commit '
          'message.  CQ-DEPEND against this rev may not work, nor '
          'will any gerrit querying.  Please add the appropriate '
          'Change-Id into the commit message to resolve this.',
          self, self.sha1)
      self._SetID(self.sha1)
    else:
      self._SetID()

  def _ParseChangeId(self, data):
    """Parse a Change-Id out of a block of text.

    Note that the returned content is *not* ran through FormatChangeId;
    this is left up to the invoker.
    """
    # Grab just the last pararaph.
    git_metadata = re.split(r'\n{2,}', data.rstrip())[-1]
    change_id_match = self._GIT_CHANGE_ID_RE.findall(git_metadata)
    if not change_id_match:
      raise BrokenChangeID(self, 'Missing Change-Id in %s' % (data,),
                           missing=True)

    # Now, validate it.  This has no real effect on actual gerrit patches,
    # but for local patches the validation is useful for general sanity
    # enforcement.
    change_id_match = change_id_match[-1]
    # Note that since we're parsing it from basically a commit message,
    # the gerrit standard format is required- no internal markings.
    if not self._STRICT_VALID_CHANGE_ID_RE.match(change_id_match):
      raise BrokenChangeID(self, change_id_match)

    return ParseChangeID(change_id_match)

  def PaladinDependencies(self, git_repo):
    """Returns an ordered list of dependencies based on the Commit Message.

    Parses the Commit message for this change looking for lines that follow
    the format:

    CQ-DEPEND=change_num+ e.g.

    A commit which depends on a couple others.

    BUG=blah
    TEST=blah
    CQ-DEPEND=10001,10002
    """
    dependencies = []
    logging.debug('Checking for CQ-DEPEND dependencies for change %s', self)

    # Only fetch the commit message if needed.
    if self.commit_message is None:
      self.Fetch(git_repo)

    try:
      dependencies = GetPaladinDeps(self.commit_message)
    except ValueError as e:
      raise BrokenCQDepends(self, str(e))

    if dependencies:
      logging.debug('Found %s Paladin dependencies for change %s',
                    dependencies, self)
    return dependencies

  def _FindEbuildConflicts(self, git_repo, upstream, inflight=False):
    """Verify that there are no ebuild conflicts in the given |git_repo|.

    When an ebuild is uprevved, git treats the uprev as a "delete" and an "add".
    If a developer writes a CL to delete an ebuild, and the CQ uprevs the ebuild
    in the mean time, the ebuild deletion is silently lost, because git does
    not flag the double-delete as a conflict. Instead the CQ attempts to test
    the CL and it ends up breaking the CQ.

    Args:
      git_repo: The directory to examine.
      upstream: The upstream git revision.
      inflight: Whether we currently have patches applied to this repository.
    """
    ebuilds = [path for (path, mtype) in
               self.GetDiffStatus(git_repo).iteritems()
               if mtype == 'D' and path.endswith('.ebuild')]

    conflicts = self._FindMissingFiles(git_repo, 'HEAD', ebuilds)
    if not conflicts:
      return

    if inflight:
      # If we're inflight, test against ToT for an accurate error message.
      tot_conflicts = self._FindMissingFiles(git_repo, upstream, ebuilds)
      if tot_conflicts:
        inflight = False
        conflicts = tot_conflicts

    raise EbuildConflict(self, inflight=inflight, ebuilds=conflicts)

  def _FindMissingFiles(self, git_repo, tree_revision, files):
    """Return a list of the |files| that are missing in |tree_revision|.

    Args:
      git_repo: Git repository to work in.
      tree_revision: Revision of the tree to use.
      files: Files to look for.

    Returns:
      A list of the |files| that are missing in |tree_revision|.
    """
    if not files:
      return []

    cmd = ['ls-tree', '--full-name', '--name-only', '-z', tree_revision, '--']
    output = git.RunGit(git_repo, cmd + files, error_code_ok=True).output
    existing_filenames = output.split('\0')[:-1]
    return [x for x in files if x not in existing_filenames]

  def GetCheckout(self, manifest, strict=True):
    """Get the ProjectCheckout associated with this patch.

    Args:
      manifest: A ManifestCheckout object.
      strict: If the change refers to a project/branch that is not in the
        manifest, raise a ChangeNotInManifest error.

    Raises:
      ChangeMatchesMultipleCheckouts if there are multiple checkouts that
      match this change.
    """
    checkouts = manifest.FindCheckouts(self.project, self.tracking_branch)
    if len(checkouts) != 1:
      if len(checkouts) > 1:
        raise ChangeMatchesMultipleCheckouts(self)
      elif strict:
        raise ChangeNotInManifest(self)
      return None

    return checkouts[0]

  def PatchLink(self):
    """Return a CL link for this patch."""
    # GitRepoPatch instances don't have a CL link, so just return the string
    # representation.
    return str(self)

  def __str__(self):
    """Returns custom string to identify this patch."""
    s = '%s:%s' % (self.project, self.ref)
    if self.sha1 is not None:
      s = '%s:%s%s' % (s, site_config.params.CHANGE_PREFIX[self.remote],
                       self.sha1[:8])
    # TODO(ferringb,build): This gets a bit long in output; should likely
    # do some form of truncation to it.
    if self._subject_line:
      s += ' "%s"' % (self._subject_line,)
    return s

  def GetLocalSHA1(self, git_repo, revision):
    """Get the local SHA1 for this patch in the given |manifest|.

    Args:
      git_repo: The path to the repo.
      revision: The tracking branch.

    Returns:
      The local SHA1 for this patch, if it is present in the given |manifest|.
      If this patch is not present, returns None.
    """
    query = 'Change-Id: %s' % self.change_id
    cmd = ['log', '-F', '--all-match', '--grep', query,
           '--format=%H', '%s..HEAD' % revision]
    output = git.RunGit(git_repo, cmd).output.split()
    if len(output) == 1:
      return output[0]
    elif len(output) > 1:
      raise BrokenChangeID(self, 'Duplicate change ID')

  def _GetParents(self, git_repo):
    """Get the parent sha1s of this patch.

    Args:
      git_repo: The path to the repo.

    Returns:
      A list of sha1s. For normal commits, this will be a length=1 list. For
      merge commits, this will be a length=2 list. For commits with no parent
      (i.e. the initial commit of a repo) this will be an empty list.
    """
    self.Fetch(git_repo)

    cmd = ['show', '--format=%P', '-s', self.sha1]
    parents = git.RunGit(git_repo, cmd).output.split()
    return parents

  def IsMerge(self, git_repo):
    """Determine if this patch is a merge commit.

    Args:
      git_repo: The path to the repo.

    Returns:
      True if this is a merge commit, false otherwise.
    """
    return len(self._GetParents(git_repo)) == 2

  def _IsAncestorOf(self, git_repo, other_patch):
    """Determine whether this patch is ancestor of |other_patch|.

    Args:
      git_repo: The git repository to fetch into.
      other_patch: A GitRepoPatch representing the other patch.

    Returns:
      True if this patch is ancestor of |other_patch|. False otherwise.
    """
    self.Fetch(git_repo)
    other_patch.Fetch(git_repo)

    cmd = ['merge-base', '--is-ancestor', self.sha1, other_patch.sha1]
    try:
      git.RunGit(git_repo, cmd)
      # Exit code 0 means yes.
      return True
    except cros_build_lib.RunCommandError as e:
      if e.result.returncode == 1:
        # Exit code 1 means no.
        return False
      # Other return codes are exceptions
      raise


class LocalPatch(GitRepoPatch):
  """Represents patch coming from an on-disk git repo."""

  def __init__(self, project_url, project, ref, tracking_branch, remote,
               sha1):
    GitRepoPatch.__init__(self, project_url, project, ref, tracking_branch,
                          remote, sha1=sha1)
    # Initialize our commit message/ChangeId now, since we know we have
    # access to the data right now.
    self.Fetch(project_url)

  def _GetCarbonCopy(self):
    """Returns a copy of this commit object, with a different sha1.

    This is used to work around a Gerrit bug, where a commit object cannot be
    uploaded for review if an existing branch (in refs/tryjobs/*) points to
    that same sha1.  So instead we create a copy of the commit object and upload
    that to refs/tryjobs/*.

    Returns:
      The sha1 of the new commit object.
    """
    hash_fields = [('tree_hash', '%T'), ('parent_hash', '%P')]
    transfer_fields = [('GIT_AUTHOR_NAME', '%an'),
                       ('GIT_AUTHOR_EMAIL', '%ae'),
                       ('GIT_AUTHOR_DATE', '%aD'),
                       ('GIT_COMMITTER_NAME', '%cn'),
                       ('GIT_COMMITTER_EMAIL', '%ce'),
                       ('GIT_COMMITER_DATE', '%ct')]
    fields = hash_fields + transfer_fields

    format_string = '%n'.join([code for _, code in fields] + ['%B'])
    result = git.RunGit(self.project_url,
                        ['log', '--format=%s' % format_string, '-n1',
                         self.sha1])
    lines = result.output.splitlines()
    field_value = dict(zip([name for name, _ in fields],
                           [line.strip() for line in lines]))
    commit_body = '\n'.join(lines[len(fields):])

    if len(field_value['parent_hash'].split()) != 1:
      raise PatchException(self,
                           'Branch %s:%s contains merge result %s!'
                           % (self.project, self.ref, self.sha1))

    extra_env = dict([(field, field_value[field]) for field, _ in
                      transfer_fields])

    # Reset the commit date to a value that can't conflict; if we
    # leave this to git, it's possible for a fast moving set of commit/uploads
    # to all occur within the same second (thus the same commit date),
    # resulting in the same sha1.
    extra_env['GIT_COMMITTER_DATE'] = str(
        int(extra_env['GIT_COMMITER_DATE']) - 1)

    result = git.RunGit(
        self.project_url,
        ['commit-tree', field_value['tree_hash'], '-p',
         field_value['parent_hash']],
        extra_env=extra_env, input=commit_body)

    new_sha1 = result.output.strip()
    if new_sha1 == self.sha1:
      raise PatchException(
          self,
          'Internal error!  Carbon copy of %s is the same as original!'
          % self.sha1)

    return new_sha1

  def Upload(self, push_url, remote_ref, carbon_copy=True, dryrun=False,
             reviewers=(), cc=()):
    """Upload the patch to a remote git branch.

    Args:
      push_url: Which url to push to.
      remote_ref: The ref on the remote host to push to.
      carbon_copy: Use a carbon_copy of the local commit.
      dryrun: Do the git push with --dry-run
      reviewers: Iterable of reviewers to add.
      cc: Iterable of people to add to cc.

    Returns:
      A list of gerrit URLs found in the output
    """
    if carbon_copy:
      ref_to_upload = self._GetCarbonCopy()
    else:
      ref_to_upload = self.sha1

    cmd = ['push']

    # This matches repo's project.py:Project.UploadForReview logic.
    if reviewers or cc:
      if push_url.startswith('ssh://'):
        rp = (['gerrit receive-pack'] +
              ['--reviewer=%s' % x for x in reviewers] +
              ['--cc=%s' % x for x in cc])
        cmd.append('--receive-pack=%s' % ' '.join(rp))
      else:
        rp = ['r=%s' % x for x in reviewers] + ['cc=%s' % x for x in cc]
        remote_ref += '%' + ','.join(rp)

    cmd += [push_url, '%s:%s' % (ref_to_upload, remote_ref)]
    if dryrun:
      cmd.append('--dry-run')

    # Depending on git/gerrit/weather, the URL might be written to stdout or
    # stderr.  Just combine them so we don't have to worry about it.
    result = git.RunGit(self.project_url, cmd, capture_output=True,
                        combine_stdout_stderr=True)
    lines = result.output.splitlines()
    urls = []
    for num, line in enumerate(lines):
      # Look for output like:
      # remote: New Changes:
      # remote:   https://chromium-review.googlesource.com/36756 Enforce a ...
      if 'New Changes:' in line:
        urls = []
        for line in lines[num + 1:]:
          line = line.split()
          if len(line) < 2 or not line[1].startswith('http'):
            break
          urls.append(line[1])
        break
    return urls


class UploadedLocalPatch(GitRepoPatch):
  """Represents an uploaded local patch passed in using --remote-patch."""

  def __init__(self, project_url, project, ref, tracking_branch,
               original_branch, original_sha1, remote, carbon_copy_sha1=None):
    """Initializes an UploadedLocalPatch instance.

    Args:
      project_url: See GitRepoPatch for documentation.
      project: See GitRepoPatch for documentation.
      ref: See GitRepoPatch for documentation.
      tracking_branch: See GitRepoPatch for documentation.
      original_branch: The tracking branch of the local patch.
      original_sha1: The sha1 of the local commit.
      remote: See GitRepoPatch for documentation.
      carbon_copy_sha1: The alternative commit hash to use.
    """
    GitRepoPatch.__init__(self, project_url, project, ref, tracking_branch,
                          remote, sha1=carbon_copy_sha1)
    self.original_branch = original_branch
    self.original_sha1 = ParseSHA1(original_sha1)
    self._original_sha1_valid = False if self.original_sha1 is None else True
    if self._original_sha1_valid and not self.id:
      self.id = AddPrefix(self, self.original_sha1)

  def LookupAliases(self):
    """Return the list of lookup keys this change is known by."""
    l = GitRepoPatch.LookupAliases(self)
    if self._original_sha1_valid:
      l.append(AddPrefix(self, self.original_sha1))

    return l

  def __str__(self):
    """Returns custom string to identify this patch."""
    s = '%s:%s:%s' % (self.project, self.original_branch,
                      self.original_sha1[:8])
    # TODO(ferringb,build): This gets a bit long in output; should likely
    # do some form of truncation to it.
    if self._subject_line:
      s += ':"%s"' % (self._subject_line,)
    return s


class GerritFetchOnlyPatch(GitRepoPatch):
  """Object that contains information to cherry-pick a Gerrit CL."""

  def __init__(self, project_url, project, ref, tracking_branch, remote,
               sha1, change_id, gerrit_number, patch_number, owner_email=None,
               fail_count=0, pass_count=0, total_fail_count=0,
               commit_message=None):
    """Initializes a GerritFetchOnlyPatch object."""
    super(GerritFetchOnlyPatch, self).__init__(
        project_url, project, ref, tracking_branch, remote,
        change_id=change_id, sha1=sha1)
    self.gerrit_number = gerrit_number
    self.patch_number = patch_number
    # TODO: Do we need three variables for the commit hash?
    self.revision = self.commit = self.sha1

    # Variables below are required to print the CL link.
    self.owner_email = owner_email
    self.owner = None
    if self.owner_email:
      self.owner = self.owner_email.split('@', 1)[0]

    self.url = gob_util.GetChangePageUrl(
        site_config.params.GERRIT_HOSTS[self.remote], int(self.gerrit_number))
    self.fail_count = fail_count
    self.pass_count = pass_count
    self.total_fail_count = total_fail_count
    # commit_message is herited from GitRepoPatch, only override it when passed
    # in value is not None.
    if commit_message:
      self.commit_message = commit_message

  @classmethod
  def FromAttrDict(cls, attr_dict):
    """Get a GerritFetchOnlyPatch instance from a dict.

    Args:
      attr_dict: A dictionary with the keys given in ALL_ATTRS.
    """
    return GerritFetchOnlyPatch(attr_dict[ATTR_PROJECT_URL],
                                attr_dict[ATTR_PROJECT],
                                attr_dict[ATTR_REF],
                                attr_dict[ATTR_BRANCH],
                                attr_dict[ATTR_REMOTE],
                                attr_dict[ATTR_COMMIT],
                                attr_dict[ATTR_CHANGE_ID],
                                attr_dict[ATTR_GERRIT_NUMBER],
                                attr_dict[ATTR_PATCH_NUMBER],
                                owner_email=attr_dict[ATTR_OWNER_EMAIL],
                                fail_count=int(attr_dict[ATTR_FAIL_COUNT]),
                                pass_count=int(attr_dict[ATTR_PASS_COUNT]),
                                total_fail_count=int(
                                    attr_dict[ATTR_TOTAL_FAIL_COUNT]),
                                commit_message=attr_dict.get(
                                    ATTR_COMMIT_MESSAGE))


  def _EnsureId(self, commit_message):
    """Ensure we have a usable Change-Id

    Validate what we received from gerrit against what the commit message
    states.
    """
    # GerritPatch instances get their Change-Id from gerrit
    # directly; for this to fail, there is an internal bug.
    assert self.id is not None

    # For GerritPatches, we still parse the ID- this is
    # primarily so we can throw an appropriate warning,
    # and also validate our parsing against gerrit's in
    # the process.
    try:
      parsed_id = self._ParseChangeId(commit_message)
      if parsed_id != self.change_id:
        raise AssertionError(
            'For Change-Id %s, sha %s, our parsing of the Change-Id did not '
            'match what gerrit told us.  This is an internal bug: either our '
            "parsing no longer matches gerrit's, or somehow this instance's "
            'stored change_id was invalidly modified.  Our parsing of the '
            'Change-Id yielded: %s'
            % (self.change_id, self.sha1, parsed_id))

    except BrokenChangeID:
      logging.warning(
          'Change %s, Change-Id %s, sha1 %s lacks a change-id in its commit '
          'message.  This can break the ability for any children to depend on '
          'this Change as a parent.  Please add the appropriate '
          'Change-Id into the commit message to resolve this.',
          self, self.change_id, self.sha1)

  def GetAttributeDict(self):
    """Get a dictionary of attribute used for manifest.

    Returns:
      A dictionary with the keys given in ALL_ATTRS.
    """
    attr_dict = {
        ATTR_REMOTE: self.remote,
        ATTR_GERRIT_NUMBER: self.gerrit_number,
        ATTR_PROJECT: self.project,
        ATTR_PROJECT_URL: self.project_url,
        ATTR_REF: self.ref,
        ATTR_BRANCH: self.tracking_branch,
        ATTR_CHANGE_ID: self.change_id,
        ATTR_COMMIT: self.commit,
        ATTR_PATCH_NUMBER: self.patch_number,
        ATTR_OWNER_EMAIL: self.owner_email,
        ATTR_FAIL_COUNT: str(self.fail_count),
        ATTR_PASS_COUNT: str(self.pass_count),
        ATTR_TOTAL_FAIL_COUNT: str(self.total_fail_count),
        ATTR_COMMIT_MESSAGE: self.commit_message,
    }

    return attr_dict

class GerritPatch(GerritFetchOnlyPatch):
  """Object that represents a Gerrit CL."""

  def __init__(self, patch_dict, remote, url_prefix):
    """Construct a GerritPatch object from Gerrit query results.

    Gerrit query JSON fields are documented at:
    http://gerrit-documentation.googlecode.com/svn/Documentation/2.2.1/json.html

    Args:
      patch_dict: A dictionary containing the parsed JSON gerrit query results.
      remote: The manifest remote the patched project uses.
      url_prefix: The project name will be appended to this to get the full
                  repository URL.
    """
    self.patch_dict = patch_dict
    self.url_prefix = url_prefix
    current_patch_set = patch_dict.get('currentPatchSet', {})
    # id - The CL's ChangeId
    # revision - The CL's SHA1 hash.
    # number - The CL's gerrit number.
    super(GerritPatch, self).__init__(
        os.path.join(url_prefix, patch_dict['project']),
        patch_dict['project'],
        current_patch_set.get('ref'),
        patch_dict['branch'],
        remote,
        current_patch_set.get('revision'),
        patch_dict['id'],
        ParseGerritNumber(str(patch_dict['number'])),
        current_patch_set.get('number'),
        owner_email=patch_dict['owner']['email'])

    prefix_str = site_config.params.CHANGE_PREFIX[self.remote]
    self.gerrit_number_str = '%s%s' % (prefix_str, self.gerrit_number)
    self.url = patch_dict['url']
    # status - Current state of this change.  Can be one of
    # ['NEW', 'SUBMITTED', 'MERGED', 'ABANDONED'].
    self.status = patch_dict['status']
    self._approvals = []
    if 'currentPatchSet' in self.patch_dict:
      self._approvals = self.patch_dict['currentPatchSet'].get('approvals', [])
    self.commit_timestamp = current_patch_set.get('date', 0)
    self.approval_timestamp = max(
        self.commit_timestamp,
        max(x['grantedOn'] for x in self._approvals) if self._approvals else 0)
    self._commit_message = None
    self.commit_message = patch_dict.get('commitMessage')

  @staticmethod
  def ConvertQueryResults(change, host):
    """Converts HTTP query results to the old SQL format.

    The HTTP interface to gerrit uses a different json schema from the old SQL
    interface.  This method converts data from the new schema to the old one,
    typically before passing it to the GerritPatch constructor.

    Old interface:
      http://gerrit-documentation.googlecode.com/svn/Documentation/2.6/json.html

    New interface:
      https://gerrit-review.googlesource.com/Documentation/rest-api-changes.html#json-entities
    """
    try:
      _convert_tm = lambda tm: calendar.timegm(
          time.strptime(tm.partition('.')[0], '%Y-%m-%d %H:%M:%S'))
      _convert_user = lambda u: {
          'name': u.get('name'),
          'email': u.get('email'),
          'username': u.get('name'),
      }
      change_id = change['change_id'].split('~')[-1]
      patch_dict = {
          'project': change['project'],
          'branch': change['branch'],
          'createdOn': _convert_tm(change['created']),
          'lastUpdated': _convert_tm(change['updated']),
          'id': change_id,
          'owner': _convert_user(change['owner']),
          'number': str(change['_number']),
          'url': gob_util.GetChangePageUrl(host, change['_number']),
          'status': change['status'],
          'subject': change.get('subject'),
      }
      current_revision = change.get('current_revision', '')
      current_revision_info = change.get('revisions', {}).get(current_revision)
      if current_revision_info:
        approvals = []
        for label, label_data in change['labels'].iteritems():
          # Skip unknown labels.
          if label not in constants.GERRIT_ON_BORG_LABELS:
            continue
          for review_data in label_data.get('all', []):
            granted_on = review_data.get('date', change['created'])
            approvals.append({
                'type': constants.GERRIT_ON_BORG_LABELS[label],
                'description': label,
                'value': str(review_data.get('value', '0')),
                'grantedOn': _convert_tm(granted_on),
                'by': _convert_user(review_data),
            })

        date = current_revision_info['commit']['committer']['date']
        for proto in ('http', 'https', 'repo', 'sso'):
          if proto in current_revision_info['fetch']:
            ref = current_revision_info['fetch'][proto]['ref']
            break
        else:
          raise ValueError('Missing ref info')
        patch_dict['currentPatchSet'] = {
            'approvals': approvals,
            'ref': ref,
            'revision': current_revision,
            'number': str(current_revision_info['_number']),
            'date': _convert_tm(date),
            'draft': current_revision_info.get('draft', False),
        }

        current_commit = current_revision_info.get('commit')
        if current_commit:
          patch_dict['commitMessage'] = current_commit['message']
          parents = current_commit.get('parents', [])
          patch_dict['dependsOn'] = [{'revision': p['commit']} for p in parents]

      return patch_dict
    except:
      logging.error('Error while converting:\n%s', change, exc_info=True)
      raise

  def __reduce__(self):
    """Used for pickling to re-create patch object."""
    return self.__class__, (self.patch_dict.copy(), self.remote,
                            self.url_prefix)

  def GerritDependencies(self):
    """Returns the list of PatchQuery objects that this patch depends on."""
    results = []
    for d in self.patch_dict.get('dependsOn', []):
      gerrit_number = d.get('number')
      if gerrit_number is not None:
        gerrit_number = ParseGerritNumber(gerrit_number, error_ok=False)

      change_id = d.get('id')
      if change_id is not None:
        change_id = ParseChangeID(change_id, error_ok=False)

      sha1 = d.get('revision')
      if sha1 is not None:
        sha1 = ParseSHA1(sha1, error_ok=False)

      if not gerrit_number and not change_id and not sha1:
        raise AssertionError(
            'While processing the dependencies of change %s, no "number", "id",'
            ' or "revision" key found in: %r' % (self.gerrit_number, d))

      results.append(
          PatchQuery(self.remote, project=self.project,
                     tracking_branch=self.tracking_branch,
                     gerrit_number=gerrit_number,
                     change_id=change_id, sha1=sha1))
    return results

  def IsAlreadyMerged(self):
    """Returns whether the patch has already been merged in Gerrit."""
    return self.status == 'MERGED'

  def IsBeingMerged(self):
    """Whether the patch is merged or in the progress of being merged."""
    return self.status in ('SUBMITTED', 'MERGED')

  def IsDraft(self):
    """Return true if the latest patchset is a draft."""
    return self.patch_dict['currentPatchSet']['draft']

  def HasApproval(self, field, value):
    """Return whether the current patchset has the specified approval.

    Args:
      field: Which field to check.
        'VRIF': Whether patch was verified.
        'CRVW': Whether patch was approved.
        'COMR': Whether patch was marked commit ready.
        'TRY':  Whether patch was marked ready for trybot.
      value: The expected value of the specified field (as string, or as list
             of accepted strings).
    """
    # All approvals default to '0', so use that if there's no matches.
    type_approvals = [x['value'] for x in self._approvals if x['type'] == field]
    type_approvals = type_approvals or ['0']
    if isinstance(value, (tuple, list)):
      return bool(set(value) & set(type_approvals))
    else:
      return value in type_approvals

  def HasApprovals(self, flags):
    """Return whether the current patchset has the specified approval.

    Args:
      flags: A dictionary of flag -> value mappings in
        GerritPatch.HasApproval format.
        ex: { 'CRVW': '2', 'VRIF': '1', 'COMR': ('1', '2') }

    returns boolean telling if all flag requirements are met.
    """
    return all(self.HasApproval(field, value)
               for field, value in flags.iteritems())

  def WasVetoed(self):
    """Return whether this CL was vetoed with VRIF=-1 or CRVW=-2."""
    return self.HasApproval('VRIF', '-1') or self.HasApproval('CRVW', '-2')

  def IsMergeable(self):
    """Return true if all Gerrit approvals required for submission are set."""
    return not self.GetMergeException()

  def HasReadyFlag(self):
    """Return true if the trybot-ready or commit-ready flag is set."""
    return self.HasApproval('COMR', ('1', '2')) or self.HasApproval('TRY', '1')

  def GetMergeException(self):
    """Return the reason why this change is not mergeable.

    If the change is in fact mergeable, return None.
    """
    if self.IsDraft():
      return PatchNotSubmittable(self, 'is a draft.')

    if self.status != 'NEW':
      statuses = {
          'MERGED': 'is already merged.',
          'SUBMITTED': 'is being merged.',
          'ABANDONED': 'is abandoned.',
      }
      message = statuses.get(self.status, 'has status %s.' % self.status)
      return PatchNotSubmittable(self, message)

    if self.HasApproval('VRIF', '-1'):
      return PatchNotSubmittable(self, 'is marked as Verified=-1.')
    elif self.HasApproval('CRVW', '-2'):
      return PatchNotSubmittable(self, 'is marked as Code-Review=-2.')
    elif not self.HasApproval('CRVW', '2'):
      return PatchNotSubmittable(self, 'is not marked Code-Review=+2.')
    elif not self.HasApproval('VRIF', '1'):
      return PatchNotSubmittable(self, 'is not marked Verified=+1.')
    elif not self.HasApproval('COMR', ('1', '2')):
      return PatchNotSubmittable(self, 'is not marked Commit-Queue>=+1.')

  def GetLatestApproval(self, field):
    """Return most recent value of specific field on the current patchset.

    Args:
      field: Which field to check ('VRIF', 'CRVW', ...).

    Returns:
      Most recent field value (as str) or '0' if no such field.
    """
    # All approvals default to '0', so use that if there's no matches.
    type_approvals = [x['value'] for x in self._approvals if x['type'] == field]
    return type_approvals[-1] if type_approvals else '0'

  def PatchLink(self):
    """Return a CL link for this patch."""
    return 'CL:%s' % (self.gerrit_number_str,)

  def _AddFooters(self, msg):
    """Ensure that commit messages have necessary Gerrit footers on the end.

    Args:
      msg: The commit message.

    Returns:
      The modified commit message with necessary Gerrit footers.
    """
    msg = super(GerritPatch, self)._AddFooters(msg)

    # This function is adapted from the version in Gerrit:
    # goto/createCherryPickCommitMessage
    old_footers = self._GetFooters(msg)

    gerrit_host = site_config.params.GERRIT_HOSTS[self.remote]
    reviewed_on = 'https://%s/%s' % (gerrit_host, self.gerrit_number)
    if ('Reviewed-on', reviewed_on) not in old_footers:
      msg += 'Reviewed-on: %s\n' % reviewed_on

    for approval in self._approvals:
      footer = FooterForApproval(approval, old_footers)
      if footer and footer not in old_footers:
        msg += '%s: %s\n' % footer

    return msg

  def __str__(self):
    """Returns custom string to identify this patch."""
    s = '%s:%s' % (self.owner, self.gerrit_number_str)
    if self.sha1 is not None:
      s = '%s:%s%s' % (s, site_config.params.CHANGE_PREFIX[self.remote],
                       self.sha1[:8])
    if self._subject_line:
      s += ':"%s"' % (self._subject_line,)
    return s


FOOTER_TAGS_BY_APPROVAL_TYPE = {
    'CRVW': 'Reviewed-by',
    'VRIF': 'Tested-by',
    'COMR': 'Commit-Ready',
    'TRY': None,
    'SUBM': 'Submitted-by',
}


def FooterForApproval(approval, footers):
  """Return a commit-message footer for a given approver.

  Args:
    approval: A dict containing the information about an approver
    footers: A sequence of existing footers in the commit message.

  Returns:
    A 'footer', which is a tuple (tag, id).
  """
  if int(approval.get('value', 0)) <= 0:
    # Negative votes aren't counted.
    return

  name = approval.get('by', {}).get('name')
  email = approval.get('by', {}).get('email')
  ident = ' '.join(x for x in [name, email and '<%s>' % email] if x)

  # Nothing reasonable to describe them by? Ignore them.
  if not ident:
    return

  # Don't bother adding additional footers if the CL has already been
  # signed off.
  if ('Signed-off-by', ident) in footers:
    return

  # If the tag is unknown, don't return anything at all.
  if approval['type'] not in FOOTER_TAGS_BY_APPROVAL_TYPE:
    logging.warning('unknown gerrit type %s (%r)', approval['type'], approval)
    return

  # We don't care about certain gerrit flags as they aren't approval related.
  tag = FOOTER_TAGS_BY_APPROVAL_TYPE[approval['type']]
  if not tag:
    return

  return tag, ident


def GeneratePatchesFromRepo(git_repo, project, tracking_branch, branch, remote,
                            allow_empty=False):
  """Create a list of LocalPatch objects from a repo on disk.

  Args:
    git_repo: The path to the repo.
    project: The name of the associated project.
    tracking_branch: The remote tracking branch we want to test against.
    branch: The name of our local branch, where we will look for patches.
    remote: The name of the remote to use. E.g. 'cros'
    allow_empty: Whether to allow the case where no patches were specified.
  """

  result = git.RunGit(
      git_repo,
      ['rev-list', '--reverse', '%s..%s' % (tracking_branch, branch)])

  sha1s = result.output.splitlines()
  if not sha1s:
    if not allow_empty:
      cros_build_lib.Die('No changes found in %s:%s' % (project, branch))
    return

  for sha1 in sha1s:
    yield LocalPatch(os.path.join(git_repo, '.git'),
                     project, branch, tracking_branch,
                     remote, sha1)

# Parser related functions
def _CheckLocalPatches(manifest, local_patches):
  """Do an early quick check of the passed-in patches.

  If the branch of a project is not specified we append the current branch the
  project is on.

  TODO(davidjames): The project:branch format isn't unique, so this means that
  we can't differentiate what directory the user intended to apply patches to.
  We should references by directory instead.

  Args:
    manifest: The manifest object for the checkout in question.
    local_patches: List of patches to check in project:branch format.

  Returns:
    A list of patches that have been verified, in project:branch format.
  """
  verified_patches = []
  for patch in local_patches:
    project, _, branch = patch.partition(':')

    checkouts = manifest.FindCheckouts(project)
    if not checkouts:
      cros_build_lib.Die('Project %s does not exist.' % (project,))
    if len(checkouts) > 1:
      cros_build_lib.Die(
          'We do not yet support local patching for projects that are checked '
          'out to multiple directories. Try uploading your patch to gerrit '
          'and referencing it via the -g option instead.'
      )
    checkout = checkouts[0]

    project_dir = checkout.GetPath(absolute=True)

    # If no branch was specified, we use the project's current branch.
    if not branch:
      local_branch = git.GetCurrentBranch(project_dir)
    else:
      local_branch = branch

    if local_branch and git.DoesCommitExistInRepo(project_dir, local_branch):
      verified_patches.append('%s:%s' % (project, local_branch))
    else:
      if branch:
        cros_build_lib.Die('Project %s (checked out at %s) has no branch %s'
                           % (checkout['name'], checkout['path'], branch))
      else:
        cros_build_lib.Die('Project %s is not on a branch!' % (project,))

  return verified_patches


def PrepareLocalPatches(manifest, patches):
  """Finish validation of parameters, and save patches to a temp folder.

  Args:
    manifest: The manifest object for the checkout in question.
    patches: A list of user-specified patches, in project[:branch] form.
  """
  patch_info = []
  for patch in _CheckLocalPatches(manifest, patches):
    project, branch = patch.split(':')
    project_patch_info = []
    for checkout in manifest.FindCheckouts(project):
      tracking_branch = checkout['tracking_branch']
      project_dir = checkout.GetPath(absolute=True)
      remote = checkout['remote']
      project_patch_info.extend(GeneratePatchesFromRepo(
          project_dir, project, tracking_branch, branch, remote))

    if not project_patch_info:
      cros_build_lib.Die('No changes found in %s:%s' % (project, branch))
    patch_info.extend(project_patch_info)

  return patch_info


def PrepareRemotePatches(patches):
  """Generate patch objects from list of --remote-patch parameters.

  Args:
    patches: A list of --remote-patches strings that the user specified on
             the commandline.  Patch strings are colon-delimited.  Patches come
             in the format
             <project>:<original_branch>:<ref>:<tracking_branch>:<tag>.
             A description of each element:
             project: The manifest project name that the patch is for.
             original_branch: The name of the development branch that the local
                              patch came from.
             ref: The remote ref that points to the patch.
             tracking_branch: The upstream branch that the original_branch was
                              tracking.  Should be a manifest branch.
             tag: Denotes whether the project is an internal or external
                  project.
  """
  patch_info = []
  for patch in patches:
    try:
      project, original_branch, ref, tracking_branch, tag = patch.split(':')
    except ValueError as e:
      raise ValueError(
          'Unexpected tryjob format.  You may be running an '
          "older version of chromite.  Run 'repo sync "
          "chromiumos/chromite'.  Error was %s" % e)

    if tag not in constants.PATCH_TAGS:
      raise ValueError('Bad remote patch format.  Unknown tag %s' % tag)

    remote = site_config.params.EXTERNAL_REMOTE
    if tag == constants.INTERNAL_PATCH_TAG:
      remote = site_config.params.INTERNAL_REMOTE

    push_url = site_config.params.GIT_REMOTES[remote]
    patch_info.append(UploadedLocalPatch(os.path.join(push_url, project),
                                         project, ref, tracking_branch,
                                         original_branch,
                                         os.path.basename(ref), remote))

  return patch_info


def GetChangesAsString(changes, prefix='CL:', delimiter=' '):
  """Gets a human readable string listing |changes| in CL:1234 form.

  Args:
    changes: A list of GerritPatch objects.
    prefix: Prefix to use. Defaults to 'CL:'
    delimiter: Delimiter to use. Defaults to a space.
  """
  formatted_changes = ['%s%s' % (prefix, AddPrefix(x, x.gerrit_number))
                       for x in changes]
  return delimiter.join(sorted(formatted_changes))
