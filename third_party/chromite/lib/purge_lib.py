# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script scans through all canary builds in gs://chromeos-releases/.

Classify what type of build it was.
"""

from __future__ import print_function

import re
import urlparse

from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import git

RELEASE = 'RELEASE'
FIRMWARE = 'FIRMWARE'
FACTORY = 'FACTORY'


#
# This section of the library is for locating purge candidates in
#   gs://chromeos-releases.
#

class ParseException(Exception):
  """Raised when something fails to parse."""


def ParseBranchName(branch):
  """Parse a firmware/factory branch name.

  Given a branch name (possibly a remote branch) of a firmware or factory
  branch, extract the version number it was branched from.

  'origin/factory-veyron-7505.B' -> '7505'
  'factory-2723.14.B' -> '2723.14'

  By convention, this extracted version number is a partial version that
  identifies the build branched from. For example, '7505' was branched from
  '7505.0.0'. '2723.14' from '2723.14.0'.

  Any build which starts with matching version numbers (except the build
  branched from) will have been built from this branch. If multiple branches
  were taken from the same root build, then we will have collisions. That
  doesn't matter since the build artifacts would be stored in locations that
  would also collide.

  Args:
    branch: Branch name as string.

  Returns:
    version as string.

  Raises:
    ParseException if the branch isn't a conventially named firmware/factory
    branch.
  """
  PATTERN = r'(factory|firmware)-([a-zA-Z_]+-)?([0-9\.]+)\.B$'
  m = re.search(PATTERN, branch)
  if not m or not m.group(3):
    raise ParseException('Unable to parse branch name: "%s"' % branch)
  return m.group(3)


def ProtectedBranchVersions(remote_branches):
  """Get a list of all protected branch versions.

  This returns a list of all branch versions that appear to be 'protected'
  meaning they are either factory or firmware branches.

  Returns:
    List of branch versions as strings.
  """
  result = []
  for branch in remote_branches:
    try:
      result.append(ParseBranchName(branch))
    except ParseException:
      pass
  return result


def ParseChromeosReleasesBuildUri(uri):
  """Parse a build URI.

  'gs://chromeos-releases/canary-channel/duck/6652.0.0/' -> '6652.0.0'

  Args:
    uri: string describing a build URI.

  Returns:
    version as string.

  Raises:
    ParseException if the URI does not describe a build output directory.
  """
  PATTERN = (r'^gs://.*/.*/.*/([0-9\.]+)/$')
  m = re.match(PATTERN, uri)
  if not m or not m.group(1):
    raise ParseException('Unable to parse build uri: "%s"' % uri)
  return m.group(1)


def VersionBranchMatch(version, branch):
  """Does the given version match the branch description?

  Given a branch version of '1', then '1.2.0' or '1.2.3' were built from this
  branch and will match.

  However, '1.0.0' is the build the branch was built from, and so '1.0.0' was
  NOT built on the branch, and does not match.

  See ParseBranchName for a fuller explaination of branch versions.

  Args:
    version: As a string ('1.2.3').
    branch: As a string for a partial version that was branched from.

  Returns:
    boolean telling if the version is part of the branch.
  """
  version_parts = [int(n) for n in version.split('.')]
  branch_parts = [int(n) for n in branch.split('.')]
  branch_len = len(branch_parts)

  if len(version_parts) <= branch_len:
    return False

  if version_parts[:branch_len] != branch_parts:
    return False

  # If the first digit matching the branch branch is zero, we are the build
  # branched from, not on the branch.
  if version_parts[branch_len] == 0:
    return False

  return True


def InBranches(version, branch_versions):
  """Does a specific build match any of the given parse branch names?

  Args:
    board: Board name of the build.
    version: Version of the build.
    branch_versions: List of parsed branch names.

  Returns:
    boolean telling if there was a match.
  """
  return any(VersionBranchMatch(version, b_version)
             for b_version in branch_versions)


def ListRemoteBranches():
  """Get a list of all remote branches for the chromite repository.

  Returns:
    List of branch names as strings.
  """
  ret = git.RunGit(constants.CHROMITE_DIR, ['branch', '-lr'])
  return [l.strip() for l in ret.output.splitlines()]


def SafeList(ctx, url):
  """Get a GS listing with details enabled.

  Ignore most any error. This is because GS flake can trigger all sorts of
  random failures, and we don't want flake to interrupt a multi-day script run.
  It is generally safe to return [] since any files that would have been
  discovered will be presumed to not exist, and so ignored during the current
  cleanup pass.

  Also, this script is convenient for mocking out results in unittests.
  """
  try:
    return ctx.List(url, details=True)
  except Exception as e:
    # We can fail for lots of repeated random reasons.
    logging.warn('List of "%s" failed, ignoring: "%s"', url, e)
    return []


def LocateChromeosReleasesProtectedPrefixes(ctx, protected_branches):
  """Find all prefixes in gs://chromeos-releases to exclude.

  This determines locations to be preserved when considering files to remove.

  We never cleanup dev, beta, stable channel builds, or signer logs/operational
  directories. We also preverve the Attic (for now) since we don't have file
  permissions to clean it up.

  We look at all builds in the canary channel, and mark them for preservation if
  they were built on a firmware or factory branch. We determine the branch by
  using the version numbers of the branch and the build.

  We preserve the firmware/factory builds, since they are the builds which
  produced binaries which may be used for long periods of time.

  Args:
    ctx: GS context.
    protected_branches: List of branch versions as strings.

  Returns:
    Returns an iterator of URL prefixes to exclude.
  """
  result = [
      'gs://chromeos-releases/Attic',
      'gs://chromeos-releases/stable-channel',
      'gs://chromeos-releases/beta-channel',
      'gs://chromeos-releases/dev-channel',
      'gs://chromeos-releases/logs',
      'gs://chromeos-releases/tobesigned',
  ]

  # We have to examine canary channel builds one at a time.
  boards = SafeList(ctx, 'gs://chromeos-releases/canary-channel/')
  for board in boards:
    builds = SafeList(ctx, board.url)
    for build in builds:
      try:
        version = ParseChromeosReleasesBuildUri(build.url)
        if InBranches(version, protected_branches):
          result.append(build.url)

      except ParseException:
        # Files we don't understand are purge candidates.
        logging.info('Found unexpected: "%s"', build.url)

  return result

#
# This section is for location purge candidates in
#   gs://chromeos-image-archive/.
#

def LocateChromeosImageArchiveProtectedPrefixes(ctx):
  """Find all prefixes in gs://chromeos-image-archive to exclude.

  We look at all builder names, and protect any that are firmware builders (but
  not trybots), since these firmware builds are needed for manual FAFT tests run
  prior to releasing a new firmware version.
  We also protect the tryjob images generated by testbed-ap. The images were
  built and used by the wifi test lab. The images will be installed
  periodically, so we need to keep the images in gs to provide the access.

  Args:
    ctx: GS context.

  Returns:
    Returns an iterator of URL prefixes to exclude.
  """
  result = []
  top_levels = SafeList(ctx, 'gs://chromeos-image-archive/')

  for top_level in top_levels:
    # 'gs://chromeos-releases/Attic/' -> 'Attic'
    name = top_level.url.rstrip('/').split('/')[-1]
    # If non-trybot firmware builds or testbed-ap builds, skip them.
    # Note that at various points in time, tryjobs have had various
    # combinations of trybot-* prefixes and *-tryjob suffixes.
    if ((not name.startswith('trybot-') and name.endswith('-firmware')) or
        (name.endswith('-test-ap') or name.endswith('-test-ap-tryjob'))):
      result.append(top_level.url)
  return result

#
# This section is for handling purge candidates.
#

def ProduceFilteredCandidates(ctx, root_url, prefixes, search_depth):
  """Given a root URL and a list of prefixes get a list of purge candidates.

  Args:
    ctx: GS context.
    root_url: Url of base directory to consider. IE: gs://chromeos-releases/
    prefixes: Iterable list of URLs to exclude from results.
    search_depth: Minimum directory depth of a directory result.

  Returns:
    Returns an iterable of gs.GSListResult objects for files.
  """
  def depth(url):
    return len(url.split('/'))

  prefix_regex_pattern = '|'.join([re.escape(p) for p in prefixes])
  prefix_re = re.compile(prefix_regex_pattern)

  logging.info('Examining: "%s"', root_url)

  # How many directory levels down do we have to go.
  search_depth = max(depth(root_url) + search_depth,
                     max(depth(p) for p in prefixes))
  logging.debug('Using search_depth %d.', search_depth)

  def recurse(base_url):
    for result in SafeList(ctx, base_url):
      url = result.url

      if prefix_re.match(url):
        continue

      if url.endswith('/') and depth(url) < search_depth:
        # If we are not deep enough to match all possible patterns, recurse.
        for u in recurse(url):
          yield u
      else:
        # If it's a file, or we are deep enough, return.
        yield result

  return recurse(root_url)


def Expand(ctx, candidate):
  """Given a gs.GSListResult object, expand to a list of testable objects.

  Will return an iterable of gs.GSListResult of files with the creation_time
  attribute populated. If the object already represents a file with that
  field populated, it will be return as is.

  Directories will be expanded to all files inside the directory.

  Args:
    ctx: GS context.
    candidate: gs.GSListResult object representing a file or directory.

  Returns:
    Returns an iterable of gs.GSListResult objects.
  """
  if candidate.creation_time is not None:
    # If it's a details populated file, return as is.
    # A directory can't have a creation time.
    return [candidate]

  url = candidate.url
  if url.endswith('/'):
    # If it's a directory, return the full contents.
    url += '**'

  return SafeList(ctx, url)


def Expire(ctx, dryrun, url):
  """Given a url, move it to the backup buckets.

  Args:
    ctx: GS context.
    dryrun: Do we actually move the file?
    url: Address of file to move.
  """
  logging.info('Expiring: %s', url)
  # Move gs://foo/some/file -> gs://foo-backup/some/file
  parts = urlparse.urlparse(url)
  expired_parts = list(parts)
  expired_parts[1] = parts.netloc + '-backup'
  target_url = urlparse.urlunparse(expired_parts)
  if dryrun:
    logging.notice('gsutil mv %s %s', url, target_url)
  else:
    try:
      ctx.Move(url, target_url)
    except Exception as e:
      # We can fail for lots of repeated random reasons.
      logging.warn('Move of "%s" failed, ignoring: "%s"', url, e)


def ExpandAndExpire(ctx, dryrun, expired_cutoff, candidate):
  """Given a list of candidate, expand to files, and expire if needed.

  Args:
    ctx: GS context.
    dryrun: Flag to turn on/off bucket updates.
    expired_cutoff: datetime.datetime of cutoff for expiring candidates.
    candidate: gs.GSListResult object of a file or directory.
  """
  for file_candidate in Expand(ctx, candidate):
    if file_candidate.creation_time < expired_cutoff:
      Expire(ctx, dryrun, file_candidate.url)
