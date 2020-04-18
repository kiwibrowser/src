#!/usr/bin/env bash
. git-drover.demo.common.sh

drover_c "This change needs to go to branch 9999"

echo "# Make sure we have the most up-to-date branch sources."
run git fetch
echo
echo "# Here's a commit (from some.committer) that we want to 'drover'."
run git log -n 1 --pretty=fuller
echo
echo "# Checkout the branch we want to 'drover' to."
run git checkout -b drover_9999 branch-heads/9999
echo
echo "# Now do the 'drover'."
echo "# IMPORTANT!!! Do Not leave off the '-x' flag"
run git cherry-pick -x $(git show-ref -s pick_commit)
echo
echo "# That took the code authored by some.committer and committed it to"
echo "# the branch by the person who drovered it (i.e. you)."
run git log -n 1 --pretty=fuller
echo
echo "# Looks good. Ship it!"
pcommand git cl upload
echo "# Wait for LGTM or TBR it."
run git cl land
echo "# Or skip the LGTM/TBR and just 'git cl land --bypass-hooks'"
