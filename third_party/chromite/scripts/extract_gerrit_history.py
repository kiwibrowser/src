# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build database associating Gerrit change # with commit metadata."""

from __future__ import print_function

import collections
import pickle
import re
import subprocess

import git

from chromite.lib import clactions
from chromite.lib import commandline


_GERRIT_MESSAGE_REXP = (
    "Reviewed-on: https://(.*?)(?:/gerrit)?(?:/r)?/([0-9]*)\n")

# These are gerrit hosts that we know have been used for some CLs in our
# history. We do not need to log them as anomalies.
_SILENTLY_IGNORED_GERRIT_HOSTS = (
    '10.10.10.29',
    'android.intel.com',
    'gerrit.rds.intel.com',
    'review.coreboot.org',
    'weave-review.googlesource.com',
)


def _GetParser():
  """Create the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)

  parser.add_argument('repos', action='store', nargs='*',
                      help='Paths to git repository to examine. If none '
                           'provided, iterate over entire repo checkout.')
  parser.add_argument('--since', action='store', default='2014-01-01',
                      help='Date of earliest commit to examine. '
                           'Default: 2014-01-01')
  parser.add_argument('--verbose', '-v', action='store_true', default=False,
                      help='Print more logging.')
  parser.add_argument('--output', '-o', action='store', default=None,
                      help='Path to file to which to write pickled database.')

  return parser


CommitInfo = collections.namedtuple('CommitInfo',
                                    ['gerrit_host', 'change_number',
                                     'hexsha'])

ChangeDatabase = collections.namedtuple('ChangeDatabase',
                                        ['unique_changes',
                                         'duplicate_changes'])

def _ParseCommitMessage(commit_message):
  """Extract gerrit_host and change_number from commit message.

  Args:
    commit_message: String commit message.

  Returns:
    gerrit_host, change_number tuple.

  Raises:
    ValueError if commit message does not match.
  """
  m = re.findall(_GERRIT_MESSAGE_REXP, commit_message)
  if not m:
    raise ValueError(
        'Commit message does not conform to Gerrit-reviewed pattern.')
  m = m[-1]
  return (m[0], m[1])


def _ProcessCommit(commit):
  """Extract info from a given commit.

  Args:
    commit: a git.Commit instance to process.

  Returns:
    CommitInfo instance if the given commit is a Gerrit-reviewed commit.
    None otherwise.
  """
  try:
    gerrit_host, change_number = _ParseCommitMessage(commit.message)
    return CommitInfo(gerrit_host, change_number, commit.hexsha)
  except ValueError:
    pass
  return None


def _ProcessRepo(repo, since):
  """Extracts gerrit information and associates with commit info.

  Args:
    repo: Path to git repository.
    since: date in YYYY-MM-DD format to process from.

  Returns:
    A list of (GerritChangeTuple, CommitInfo) for Gerrit-reviewed
    commits found in this |repo| after time |since|. Note: this list may
    contain duplicates, as historically some commits were cherry-picked
    forcefully or landed outside of the CQ, and may have innaccurate
    gerrit info extracted from their commit message.
  """
  r = git.Repo(repo)
  print('Examining git repository at %s' % repo)
  commits = r.iter_commits(since=since)
  commit_infos = []

  for c in commits:
    ci = _ProcessCommit(c)
    if ci:
      try:
        change_tuple = clactions.GerritChangeTuple.FromHostAndNumber(
            ci.gerrit_host, ci.change_number)
        commit_infos.append((change_tuple, ci))
      except clactions.UnknownGerritHostError as e:
        if e.gerrit_host not in _SILENTLY_IGNORED_GERRIT_HOSTS:
          print('Unknown gerrit host %s for commit %s'
                % (e.gerrit_host, ci.hexsha))

  return commit_infos


def main(argv):

  parser = _GetParser()
  options = parser.parse_args(argv)

  repo_list = options.repos
  if not repo_list:
    repo_list = subprocess.check_output(['repo', 'list', '-fp']).splitlines()


  duplicates = {}
  changes_dict = {}

  for repo in repo_list:

    changes_list = _ProcessRepo(repo, options.since)
    print('Found %s Gerrit-reviewed commits since %s.' %
          (len(changes_list), options.since))

    # Merge this repo info into database
    for item in changes_list:
      change, commit_info = item
      record = (repo, commit_info)
      if change in duplicates:
        duplicates[change].append(record)
      elif change in changes_dict:
        duplicates[change] = [changes_dict.pop(change)]
        duplicates[change].append(record)
      else:
        changes_dict[change] = record

    print('Up to %s uniques, %s duplicates' %
          (len(changes_dict), len(duplicates)))

  print('There are %s unique changes and %s duplicates.'
        % (len(changes_dict), len(duplicates)))

  if options.output:
    db = ChangeDatabase(changes_dict, duplicates)
    print('Writing database as pickle to %s' % options.output)
    with open(options.output, 'w') as f:
      pickle.dump(db, f)
