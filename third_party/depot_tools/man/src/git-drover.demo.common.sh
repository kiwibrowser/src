#!/usr/bin/env bash
. demo_repo.sh

# Add and tag a dummy commit to refer to later.
drover_c() {
  add modified_file
  set_user some.committer
  c "$1"
  silent git tag -f pick_commit
  set_user you
  tick 1000
}

silent git push origin refs/remotes/origin/master:refs/branch-heads/9999
silent git config --add remote.origin.fetch \
  +refs/branch-heads/*:refs/remotes/branch-heads/*
silent git fetch origin

silent git checkout -B master origin/master
