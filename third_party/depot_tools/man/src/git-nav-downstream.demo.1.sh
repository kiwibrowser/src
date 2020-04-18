#!/usr/bin/env bash
. demo_repo.sh

silent git checkout origin/master

run git map-branches
pcommand git nav-downstream
git nav-downstream --pick 0 2>&1
run git map-branches
run git nav-downstream  2>&1
run git map-branches

