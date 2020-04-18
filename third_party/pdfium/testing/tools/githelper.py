# Copyright 2017 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes for dealing with git."""

import subprocess

from common import RunCommandPropagateErr


class GitHelper(object):
  """Issues git commands. Stateful."""

  def __init__(self):
    self.stashed = 0

  def Checkout(self, branch):
    """Checks out a branch."""
    RunCommandPropagateErr(['git', 'checkout', branch], exit_status_on_error=1)

  def FetchOriginMaster(self):
    """Fetches new changes on origin/master."""
    RunCommandPropagateErr(['git', 'fetch', 'origin', 'master'],
                           exit_status_on_error=1)

  def StashPush(self):
    """Stashes uncommitted changes."""
    output = RunCommandPropagateErr(['git', 'stash', '--include-untracked'],
                                    exit_status_on_error=1)
    if 'No local changes to save' in output:
      return False

    self.stashed += 1
    return True

  def StashPopAll(self):
    """Pops as many changes as this instance stashed."""
    while self.stashed > 0:
      RunCommandPropagateErr(['git', 'stash', 'pop'], exit_status_on_error=1)
      self.stashed -= 1

  def GetCurrentBranchName(self):
    """Returns a string with the current branch name."""
    return RunCommandPropagateErr(
        ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
        exit_status_on_error=1).strip()

  def GetCurrentBranchHash(self):
    return RunCommandPropagateErr(
        ['git', 'rev-parse', 'HEAD'], exit_status_on_error=1).strip()

  def IsCurrentBranchClean(self):
    output = RunCommandPropagateErr(['git', 'status', '--porcelain'],
                                    exit_status_on_error=1)
    return not output

  def BranchExists(self, branch_name):
    """Return whether a branch with the given name exists."""
    output = RunCommandPropagateErr(['git', 'rev-parse', '--verify',
                                     branch_name])
    return output is not None

  def CloneLocal(self, source_repo, new_repo):
    RunCommandPropagateErr(['git', 'clone', source_repo, new_repo],
                           exit_status_on_error=1)
