#!/bin/bash
# Copyright 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -Ceu


GiveUp() {
  echo -n "*** "

  if [[ -n "$1" ]]; then
    # Print repo, do not output the trailing newline.
    echo -n "$1: "
  fi

  if [[ -n "$2" ]]; then
    # Print reason, do not output the trailing newline.
    echo -n "$2 "
  fi

  echo "Please update manually! ***"
  exit 0
}


# Go to repository.

my_repo="$@"

if [[ -n "$my_repo" ]]; then
  cd "$my_repo"
fi


# Determine working branch.
# We can't use 'git symbolic-ref HEAD' as it fails in detached HEAD state.

status=`git status --branch --short --untracked-files=no`

if [[ "$status" != "## HEAD (no branch)" ]]; then

  my_branch_ref=`git symbolic-ref HEAD`
  my_branch="${my_branch_ref#refs/heads/}"
  my_commit=`git rev-parse $my_branch_ref`

else

  # Detached HEAD state - checkout branch that fits best.
  # In general, we can walk all suitable branches and pick one that is closer
  # to the commit we are at, or give up if ambiguity.
  # However, what we actually need most of the times is to change from the
  # pinned revision to master - try it.

  my_branch_ref="refs/heads/master"
  my_branch="master"
  my_commit=`git rev-parse $my_branch_ref`

  if [[ -z "$my_commit" ]]; then
    # No branch master?
    GiveUp "$my_repo" "Failed to pick a branch to change from detached HEAD."
  fi

  prev_commit=`git rev-parse HEAD`
  merge_base=`git merge-base $prev_commit $my_commit`

  if [[ "$merge_base" != "$prev_commit" ]]; then
    # HEAD and master have diverged?
    GiveUp "$my_repo" "Failed to pick a branch to change from detached HEAD."
  fi

  # Branch seems to fit, check it out.
  git checkout "$my_branch"

fi


# Now determine upstream branch and check tracking.

upstream_branch_ref=`git for-each-ref --format='%(upstream)' $my_branch_ref`

if [[ -z "$upstream_branch_ref" ]]; then
  GiveUp "$my_repo" "Failed to determine upstream branch."
fi

upstream_branch="${upstream_branch_ref#refs/remotes/}"
upstream_commit=`git rev-parse $upstream_branch_ref`
merge_base=`git merge-base $my_commit $upstream_commit`

if [[ "$merge_base" = "$upstream_commit" ]]; then
  # We are on the tip of the remote branch already.
  exit 0
fi

if [[ "$merge_base" = "$my_commit" ]]; then
  # We can fast-forward - do it.
  git rebase "$upstream_branch" "$my_branch"
  exit 0
fi

# Need to resolve manually.
GiveUp "$my_repo" "Working branch and upstream branch have diverged."
